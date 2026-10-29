#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Separate CRT generator and seeded rounds; optional private original x86 proof."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from test_combat_rounds import expected as rounds_model
HERE=Path(__file__).resolve().parent
DEMO_SHA='28608395dc4184b0ac972ea0886a8becd57d91a07bbabaef6f798fc5ca94e6b6'

def draws(seed,n):
 values=[]
 for _ in range(n):seed=(seed*214013+2531011)&0xffffffff;values.append((seed>>16)&32767)
 return seed,values

def expected(row):
 seed,count,*duel=row
 if count:
  after,values=draws(seed,count);h=2166136261
  for v in values:h=((h^v)*16777619)&0xffffffff
  return [0,after,values[0],values[-1],h,count,0,0,0,0,0]
 _,values=draws(seed,160);out=rounds_model([*duel,160,0],values)
 return [out[0],draws(seed,out[9])[0]if out[0]==0 else seed,*out[2:]]

def cases():
 rng=random.Random(0x63724d);seeds=[0,1,2,32767,32768,65535,65536,0x7fffffff,0x80000000,0xffffffff]
 rows=[[seed,n,0,0,0,0,0,0]for seed in seeds for n in[1,2,3,16,79,160,1024]]
 rows += [[rng.getrandbits(32),rng.randrange(1,1025),0,0,0,0,0,0]for _ in range(2000)]
 for _ in range(6000):
  ar=rng.randrange(1,5);dr=rng.randrange(1,5);v=[rng.getrandbits(32),0,rng.randrange(200000),rng.randrange(200000),ar,dr,rng.randrange(10*ar),rng.randrange(10*dr)]
  if rng.randrange(5)==0:v[rng.randrange(2,8)]=rng.choice([-1,0,1,4,5,9,10,39,40,2147483647])
  rows.append(v)
 for o,d in [(0,0),(1,1),(1,256),(256,1)]:rows.append([0xffffffff,0,o,d,4,4,0,0])
 return rows

def original_x86(path,out):
 import unicorn
 from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
 from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EDI,UC_X86_REG_EDX
 data=path.read_bytes();assert hashlib.sha256(data).hexdigest()==DEMO_SHA
 # PE .text has VMA-fileoff=0x400000 for this hash-pinned original demo.
 start=0x63724d;helper=0x63a3e3
 assert data[start-0x400000]==0xe8
 assert start+5+struct.unpack_from('<i',data,start-0x400000+1)[0]==helper
 mu=Uc(UC_ARCH_X86,UC_MODE_32);mu.mem_map(0x637000,0x1000);mu.mem_write(0x637000,data[0x237000:0x238000]);mu.mem_map(0x63a000,0x1000)
 mu.mem_map(0x4ff000,0x2000);mu.mem_write(0x4ff000,data[0xff000:0x101000])
 stack=0x20000000;ptd=0x30000000;end=0x10000000
 mu.mem_map(stack,0x10000);mu.mem_map(ptd,0x1000);mu.mem_map(end,0x1000)
 decision=[]
 def hook(uc,address,size,user):
  if address==helper:
   sp=uc.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',bytes(uc.mem_read(sp,4)))[0]
   uc.reg_write(UC_X86_REG_EAX,ptd);uc.reg_write(UC_X86_REG_ESP,sp+4);uc.reg_write(UC_X86_REG_EIP,ret)
  elif address in[0x4ffdc9,0x50020b]:
   decision.append(1 if address==0x4ffdc9 else 0);uc.emu_stop()
 mu.hook_add(UC_HOOK_CODE,hook)
 seeds=[0,1,2,32767,32768,65535,65536,0x7fffffff,0x80000000,0xffffffff];rng=random.Random(0x343fd);seeds += [rng.getrandbits(32)for _ in range(256)]
 actual_bytes=b'';calls=0
 for seed in seeds:
  mu.mem_write(ptd+0x14,struct.pack('<I',seed));expected_state=seed
  for _ in range(32):
   sp=stack+0xff00;mu.mem_write(sp,struct.pack('<I',end));mu.reg_write(UC_X86_REG_ESP,sp);mu.emu_start(start,end,count=100)
   state=struct.unpack('<I',bytes(mu.mem_read(ptd+0x14,4)))[0];value=mu.reg_read(UC_X86_REG_EAX)
   expected_state,values=draws(expected_state,1);assert (state,value)==(expected_state,values[0])
   actual_bytes+=struct.pack('<II',state,value);calls+=1
 (out/'original-crt-x86-results.bin').write_bytes(actual_bytes)
 decision_bytes=b'';round_calls=0
 for seed in seeds[:32]:
  for offense in[0,1,2,256,32768,2147483647]:
   for defense in[0,1,2,256,32768,2147483647]:
    mu.mem_write(ptd+0x14,struct.pack('<I',seed));frame=stack+0x8000;mu.reg_write(UC_X86_REG_EBP,frame);mu.reg_write(UC_X86_REG_ESP,frame-0x100)
    mu.mem_write(frame-0x14,struct.pack('<I',offense));mu.mem_write(frame-0x1c,struct.pack('<I',defense));mu.mem_write(frame-0x10,struct.pack('<I',0))
    decision.clear();mu.emu_start(0x4ffd8d,0,count=200);assert len(decision)==1
    wanted_state=seed;rolls=[]
    for strength in[offense,defense]:
     if strength<=1:rolls.append(0)
     else:wanted_state,values=draws(wanted_state,1);rolls.append(values[0]%strength)
    state=struct.unpack('<I',bytes(mu.mem_read(ptd+0x14,4)))[0];off=mu.reg_read(UC_X86_REG_EDI);deff=mu.reg_read(UC_X86_REG_EDX)
    assert(state,off,deff,decision[0])==(wanted_state,*rolls,int(rolls[0]>rolls[1]))
    decision_bytes+=struct.pack('<4I',state,off,deff,decision[0]);round_calls+=1
 (out/'original-round-decision-x86-results.bin').write_bytes(decision_bytes)
 return dict(executableSHA256=DEMO_SHA,entry='0x63724d',threadStateOffset='PTD+0x14',calls=calls,bytes=len(actual_bytes),outputSHA256=hashlib.sha256(actual_bytes).hexdigest(),roundDecision=dict(entry='0x4ffd8d',attackerDraw='0x4ffd9b',defenderDraw='0x4ffdb6',compare='0x4ffdc1',cases=round_calls,bytes=len(decision_bytes),outputSHA256=hashlib.sha256(decision_bytes).hexdigest()),emulator='Unicorn '+unicorn.__version__,scope='Actual private original demo x86 rand arithmetic and round decision instructions executed; only getptd TLS lookup supplies controlled thread-state pointer. Round strength locals are controlled, execution stops before damage branches. No full game/combat loop execution claim; no original bytes committed.')

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='c++');p.add_argument('--original-demo',type=Path);a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 exe=a.out/'combat-crt-probe';cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',str(HERE/'combat_rounds.cpp'),str(HERE/'combat_crt.cpp'),str(HERE/'combat_crt_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 rows=cases();controls=b''.join(struct.pack('<8I',*(v&0xffffffff for v in row))for row in rows);(a.out/'controls.bin').write_bytes(controls)
 run=subprocess.run([str(exe)],input=controls,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),check=True,timeout=60)
 want=b''.join(struct.pack('<11I',*expected(row))for row in rows);assert run.stdout==want
 (a.out/'expected.bin').write_bytes(want)
 report=dict(cases=len(rows),generatorCases=sum(row[1]>0 for row in rows),seededDuelCases=sum(row[1]==0 for row in rows),bytes=len(want),outputSHA256=hashlib.sha256(want).hexdigest(),native='UNRUN',originalCRT=original_x86(a.original_demo,a.out)if a.original_demo else'UNRUN',fullOriginalCombat='UNRUN')
 (a.out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
