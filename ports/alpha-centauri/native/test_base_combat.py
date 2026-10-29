# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,itertools,struct,json,hashlib,subprocess,os
HERE=Path(__file__).resolve().parent
SOURCES=['base_combat.cpp','base_combat_probe.cpp']
def original_cases():
 for v,tr,fac,blink,bp,ip in itertools.product(range(2),range(3),range(8),range(2),[-100,-25,0,25,75,100],[0,25,50]):
  yield [v,864,320 if fac&4 else 288,1,320,3,tr,1,1,fac&1,fac&1,fac&1,fac&2,0,0,0,blink,bp,ip]
def original_project_cases():
 for v,tr,fac,blink,bp,ip in itertools.product(range(2),range(3),[8,16,32],range(2),[-25,0,25],[0,25]):
  yield [v,864,288,1,320,3,tr,1,1,0,0,0,0,fac&8,fac&16,fac&32,blink,bp,ip]
def cases():
 yield from original_cases()
 yield from original_project_cases()
 for v,tr,flags,bp,ip,power in itertools.product(range(2),range(3),range(256),[-100,0,33],[-100,0,25],[0,1,2147483647]):
  yield [v,power,-power,1,320,3,tr,1,1,*[(flags>>i)&1 for i in range(8)],bp,ip]
 b=[0,864,288,1,320,3,0,1,1,0,0,0,0,0,0,0,0,25,25]
 for uid,weapon,speed,faction in itertools.product([0,15,63,64,511],[-1,0,127],[0,1,2],[0,7]):
  c=b.copy();c[4:9]=[uid,weapon,0,speed,faction];yield c
 for i,vs in [(0,[-1,2]),(4,[-1,512]),(5,[-129,128]),(6,[-1,3]),(7,[-1,256]),(8,[-1,8])]:
  for value in vs:c=b.copy();c[i]=value;yield c
 for v,flags in itertools.product(range(2),range(256)):
  c=b.copy();c[0]=v;c[3]=0;c[4:9]=[-999]*5;c[9:17]=[(flags>>i)&1 for i in range(8)];yield c
 for i in [17,18]:
  for value in [-2147483648,2147483647]:c=b.copy();c[i]=value;yield c
def model(c):
 v,off,deff,present,uid,w,tr,speed,f,perim,naval,aero,tach,cit,mar,cloud,blink,bp,ip=c
 if v not in [0,1] or present and not(0<=uid<512 and -128<=w<=127 and 0<=tr<3 and 0<=speed<=255 and 0<=f<8):return [1,0,0,0,0,0,3]
 multiplier=2
 if present:
  if tr==0 and(perim or cit)or tr==1 and(naval or mar)or tr==2 and(aero or v and cloud):multiplier=4
  if tach:multiplier+=2
 baseap=multiplier==2 and(not present or not blink)and bp!=0
 infap=present and f!=0 and(uid>=64 or w>=0 and uid!=15)and tr==0 and speed==1 and ip!=0
 def pct(value,pct):
  factor=pct+100
  if not -2**31<=factor<2**31:raise OverflowError()
  value*=factor
  if not -2**31<=value<2**31:raise OverflowError()
  return (abs(value)//100)*(-1 if value<0 else 1)
 try:
  if baseap:deff=pct(deff,bp)
  if present and blink:multiplier=min(multiplier,2)
  if infap:off=pct(off,ip)
  deff*=multiplier
  if not -2**31<=deff<2**31:raise OverflowError()
 except OverflowError:return [2,0,0,0,0,0,3]
 return [0,off,deff,multiplier,int(baseap),int(infap),2]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x)for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cs=list(cases());ms=[model(c)for c in cs];data=b''.join(struct.pack('<19I',*(x&0xffffffff for x in c))for c in cs);want=b''.join(struct.pack('<7I',*(x&0xffffffff for x in m))for m in ms);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(want);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr,next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));receipt={'cases':len(cs),'accepted':sum(m[0]==0 for m in ms),'invalid':sum(m[0]==1 for m in ms),'overflow':sum(m[0]==2 for m in ms),'guards':sum(m[6]for m in ms),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
