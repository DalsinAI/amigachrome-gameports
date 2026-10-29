#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Two real bounded steps, three facility orders and coherent worker accounting/reload."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_local_workers as workers
from test_units import sections,material
from test_production_retool import model as retool_model
from test_original_faction import SCHEMA
HERE=Path(__file__).resolve().parent
SOURCES=list(dict.fromkeys(workers.SOURCES[:-1]+['world_movement_price.cpp','move_budget.cpp','world_actions.cpp','local_session_actions.cpp','local_worker_actions_probe.cpp']))
def fixture(version,case):
 b=bytearray(workers.fixture(version,case));info,o,_=workers.e.population.decode(b);count=int(info['vehicles']);at=o['bases'];rover=bytearray(52);struct.pack_into('<hh',rover,0,2,2);struct.pack_into('<h',rover,10,320);rover[14]=1;rover[36]=2;struct.pack_into('<hhh',rover,46,-1,-1,-1);b[at:at]=rover;pf=o['legacy'];struct.pack_into('<i',b,pf+0x38,count+1);_,o,_=workers.e.population.decode(b)
 p=o['prototypes']+320*52;b[p:p+52]=bytes(52);b[p+36]=1;b[p+39]=1;b[p+42]=3
 for y in range(0,6):
  for x in range(y&1,7,2):
   p=o['tiles']+((y*16+x)//2)*44;b[p]=0x60;b[p+2]=0xf0;b[p+4]=255;b[p+5]=0;b[p+7]=1;struct.pack_into('<I',b,p+8,0x100000)
 p=o['tiles']+((2*16+2)//2)*44;b[p+2]=0xf1;struct.pack_into('<I',b,p+8,0x100002)
 for i in range(count+1):
  x,y=struct.unpack_from('<hh',b,o['vehicles']+52*i);p=o['tiles']+((y*16+x)//2)*44;struct.pack_into('<I',b,p+8,struct.unpack_from('<I',b,p+8)[0]|2);b[p+2]=(b[p+2]&0xf0)|1
 morale=next(f['offset'] for f in SCHEMA['LegacyFactionV10']['fields'] if f['name']=='SE_morale');struct.pack_into('<i',b,o['factions']+0x20bc+morale,0);struct.pack_into('<i',b,o['faction_definitions']+0x59c+0x4d8,0)
 p=o['bases'];b[p+6]=[1,3,5][case%3];struct.pack_into('<IiiII',b,p+0x78,1,0,0,0x01234560,0x65432106);b[p+0x8c:p+0x98]=bytes(12)
 struct.pack_into('<i',b,p+0x30,0);struct.pack_into('<i',b,p+0x4c,0);struct.pack_into('<i',b,p+0x50,320);struct.pack_into('<i',b,p+0x44,0);struct.pack_into('<i',b,p+0x40,400);struct.pack_into('<i',b,p+0x98,111)
 return bytes(b)
def rules():
 t=sections(workers.rules('base'));t['CHASSIS'][1][9]='0'
 for row in t['FACILITIES']:row[3]='None'
 for key,value in [('retool_strictness',2),('retool_pct_pen_prod_chg',50),('retool_exemption',10)]:t['RULES'][workers.prod.ROW[key]][0]=str(value)
 return material(t)
def order(text,data,facility):
 b=bytearray(data);info,o,_=workers.e.population.decode(b);p=o['bases'];old=struct.unpack_from('<i',b,p+0x50)[0];prior=struct.unpack_from('<i',b,p+0x44)[0];baseline=struct.unpack_from('<i',b,p+0x98)[0];fac=bytes(b[p+0x8c:p+0x94])+bytes(4);result=retool_model((0,2,50,10,1,prior,-facility,baseline,*struct.unpack('<3I',fac)));assert result[0]==0
 # expected() returns raw retool result words, not a source-generated patch.
 loss=result[3];baseline=struct.unpack_from('<i',b,p+0x98)[0];struct.pack_into('<i',b,p+0x40,baseline-loss);struct.pack_into('<i',b,p+0x50,-facility)
 if old>=0:
  off=next(f['offset'] for f in SCHEMA['LegacyFactionV10']['fields'] if f['name']=='units_queue');q=o['factions']+0x20bc+off+old;b[q]=(b[q]-1)%256
 return bytes(b)
def expected(text,data):
 b=bytearray(data);info,o,_=workers.e.population.decode(b);p=o['vehicles']+(int(info['vehicles'])-1)*52;struct.pack_into('<I',b,p+4,4);b[p+40]=6;b[p+41]=6;target=o['tiles']+((3*16+3)//2)*44;b[target+2]=0xf1
 state=order(text,bytes(b),1);state=order(text,state,2);state,error=workers.patched(text,state,'base');assert not error
 state=order(text,state,3);state,error=workers.patched(text,state,'base');assert not error;return state

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--binary',type=Path);ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=a.binary.resolve() if a.binary else out/'worker-actions';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);command=[] if a.binary else [a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];
 if command:subprocess.run(command,check=True)
 runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];text=rules();digest=hashlib.sha256();records=[]
 for version in range(7,11):
  for case in [1,2,3]:
   data=fixture(version,case);wanted=expected(text,data);i=len(records);rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);p=subprocess.run([*runner,str(exe),str(rp),str(sp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(p.stdout);(out/f'{i}.stderr').write_bytes(p.stderr);(out/f'{i}.expected').write_bytes(wanted);assert p.returncode==0 and p.stdout[4:]==wanted,(version,case,p.returncode,p.stderr,next((j for j,(x,y) in enumerate(zip(p.stdout[4:],wanted)) if x!=y),None));assert rp.read_bytes()==text and sp.read_bytes()==data;digest.update(p.stdout);records.append(dict(version=version,case=case,guards=struct.unpack_from('<I',p.stdout)[0],outputSHA256=hashlib.sha256(p.stdout).hexdigest()))
 receipt=dict(worlds=len(records),moves=2*len(records),orders=3*len(records),workerCommits=2*len(records),reloads=len(records),outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=command,records=records,native='UNRUN');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
