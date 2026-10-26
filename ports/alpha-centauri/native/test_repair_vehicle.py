#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
SRC=Path(__file__).resolve().parent

def model(row):
 h=list(struct.unpack('<12I',row[:48]));v=bytearray(row[48:]);variant,phase,local,turn,index,triad,reactor,offense,items,climate,val2,mask=h
 signed=lambda n:n if n<2**31 else n-2**32
 triad,reactor,offense=map(signed,[triad,reactor,offense]);uid=struct.unpack_from('<h',v,10)[0]
 bad=lambda:struct.pack('<I',1)+bytes([90])*72
 if variant>1 or phase>7 or local>7 or index>0x7fffffff:return bad()
 if v[14]!=phase:return struct.pack('<I',0)+v+bytes(20)
 if not 0<=uid<512 or not 0<=triad<=2 or not 0<=reactor<=255 or not -128<=offense<=127:return bad()
 state=struct.unpack_from('<I',v,4)[0]&0xfbffdffd;flags=struct.unpack_from('<H',v,8)[0]&0xefff
 v[42]=v[40]=0
 if (turn+index)%4==0:
  state&=~0x800;flags&=0xfffd if flags&2 else 0xfffe
 timer=struct.unpack_from('<H',v,28)[0]
 if v[17] in [1,2] and timer:
  timer=(timer-1)%65536;struct.pack_into('<H',v,28,timer)
  if timer==0:v[17]=0
 physical=bool(items&1 and val2%16<8)
 if physical or triad==0 and items&0x800 or triad==2 and items&0x40000:state&=~8
 struct.pack_into('<IH',v,4,state,flags)
 tower=variant==1 and uid==19
 if tower:v[14]=0
 healing=floor=0;attempt=False;old=v[16]
 if not (variant==1 and uid in [16,17,18]) and (tower or not state&4) and old:
  attempt=True;native=offense<0 and uid<64;fungus=bool(items&32 and climate&224>63);altitude=climate&224;factor=1
  if not tower:
   factor=2 if native and fungus else 1
   factor+=bool(mask&1)
   factor+=bool(triad==2 and items&0x40000 or triad==0 and items&0x800)
  if mask&2 and not mask&4:
   factor+=1
   if (mask&8 if native else mask&16 or mask&32 and (variant or triad!=2)):factor=old
  bay=bool(triad==0 and mask&64 and (v[17]==1 or altitude<96) and mask&128)
  if bay:factor*=2
  if mask&256:factor=old
  healing=reactor*factor
  if phase and not bay and not mask&2 and not mask&256:
   floor=reactor*2
   if reactor and fungus:
    if native:floor=0
    if mask&512:floor=0;healing+=reactor
  v[16]=min(old,max(floor,min(999,old-healing)))
  if v[16]<=floor and mask&1024 and v[17]==1 and v[16]!=old and (triad or altitude>=96):v[17]=0
 draw=bool(v[14]==phase and not physical and (v[14]==local or v[39]&(1<<local)))
 return struct.pack('<I',0)+v+struct.pack('<5I',1,attempt,draw,healing,floor)

def contexts():
 rng=random.Random(2026102611);rows=[]
 def row(h,v):return struct.pack('<12I',*[x&0xffffffff for x in h])+bytes(v)
 for variant in [0,1]:
  # Whole fact-mask, all triads and important edition-specific native designs.
  for i in range(12288):
   v=bytearray(rng.randbytes(52));uid=[0,15,16,17,18,19,63,64,320,511][i%10]
   struct.pack_into('<h',v,10,uid);phase=i%8;v[14]=phase if i%17 else (phase+1)%8
   state=rng.getrandbits(32);state=state&~4 if i%3 else state|4;struct.pack_into('<I',v,4,state)
   v[16]=[0,1,2,3,10,99,255][i%7];v[17]=i%5
   struct.pack_into('<h',v,28,[0,1,2,-1,-32768,32767][i%6])
   h=[variant,phase,(i//8)%8,rng.getrandbits(32),i%64,i%3,[0,1,2,4,255][i%5],[-128,-1,0,1,127][i%5],[0,1,32,0x800,0x40000,0x40821][i%6],[0,32,64,96,128,224][i%6],i%16,i%2048]
   rows.append(row(h,v))
 # Exhaust each order timer/sign and every damage for both editions/special IDs.
 for variant in [0,1]:
  for uid in [0,16,17,18,19,64]:
   for damage in range(256):
    v=bytearray(52);v[14]=1;v[16]=damage;v[17]=1;struct.pack_into('<h',v,10,uid);struct.pack_into('<I',v,4,12)
    rows.append(row([variant,1,1,0,0,damage%3,1,-1,32,96,0,1025],v))
 v=bytearray(52);v[14]=1;good=[0,1,1,0,0,0,1,1,0,96,0,0]
 for field,values in [(0,[2,0xffffffff]),(1,[8,0xffffffff]),(2,[8,0xffffffff]),(4,[0x80000000]),(5,[-1,3]),(6,[-1,256]),(7,[-129,128])]:
  for value in values:
   h=good.copy();h[field]=value;rows.append(row(h,v))
 for uid in [-32768,-1,512,32767]:
  w=v.copy();struct.pack_into('<h',w,10,uid);rows.append(row(good,w))
 return rows

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cxx',default='c++');ap.add_argument('--runner',nargs=argparse.REMAINDER,default=[]);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 rows=contexts();inputs=b''.join(rows);expected=b''.join(map(model,rows));ip=a.out/'inputs.bin';ip.write_bytes(inputs);binary=a.out/'probe'
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.runner else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])
 command=[a.cxx,*flags,str(SRC/'repair_vehicle.cpp'),str(SRC/'repair_vehicle_probe.cpp'),'-o',str(binary)];subprocess.run(command,check=True)
 env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0';r=subprocess.run([*a.runner,str(binary),str(ip)],env=env,capture_output=True,timeout=120);(a.out/'outputs.bin').write_bytes(r.stdout);(a.out/'stderr.txt').write_bytes(r.stderr)
 assert r.returncode==0,(r.returncode,r.stderr[:1000]);assert r.stdout==expected,next((i for i,(x,y) in enumerate(zip(r.stdout,expected)) if x!=y),('length',len(r.stdout),len(expected)))
 errors=[struct.unpack_from('<I',r.stdout,i*76)[0] for i in range(len(rows))];receipt=dict(cases=len(rows),accepted=errors.count(0),invalid=errors.count(1),inputSHA256=hashlib.sha256(inputs).hexdigest(),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),binarySHA256=hashlib.sha256(binary.read_bytes()).hexdigest(),command=command)
 (a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
