# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,itertools,struct,json,hashlib,subprocess,os
HERE=Path(__file__).resolve().parent
DEF=[(320,3,0),(8,-1,0),(63,-128,1),(64,-1,2),(511,127,2)]
ATK=[(0,0,0,0,0),(1,320,3,0,0),(1,320,3,0,1),(1,8,-1,0,2),(1,64,-1,1,1),(1,63,-128,2,2),(1,511,127,2,7)]
def cases():
 for alt,rough,item,owner,dp,ap,ph in itertools.product([0,0x40,0x60,255],[0,0x40,0x80,255],[0,1,0x20,0x200000,0x200020,0x200021],[0,7,15],DEF,ATK,[-1,0,1,2]):
  yield [alt,rough,item,owner,1,*dp,*ap,ph]
 base=[0x60,0,0,15,1,320,3,0,1,320,3,0,2,-1]
 for i,vs in [(0,[-1,256]),(1,[-1,256]),(3,[-1,16]),(4,[-1,8]),(5,[-1,512]),(6,[-129,128]),(7,[-1,3]),(9,[-1,512]),(10,[-129,128]),(11,[-1,3]),(12,[-1,8]),(13,[-2,8])]:
  for v in vs:c=base.copy();c[i]=v;yield c
 # Absent attacker fields must be irrelevant, including invalid sentinels.
 c=base.copy();c[8]=0;c[9:13]=[-1,-999,-1,-1];yield c
 for mask in [0x80000000,0xffffffff]:c=base.copy();c[2]=mask;yield c

def model(c):
 a,r,items,o,f,p,w,t,present,ap,aw,at,af,ph=c
 invalid=not(0<=a<=255 and 0<=r<=255 and 0<=o<=15 and 0<=f<8 and 0<=p<512 and -128<=w<=127 and 0<=t<3 and -1<=ph<8)
 if present:invalid|=not(0<=ap<512 and -128<=aw<=127 and 0<=at<3 and 0<=af<8)
 if invalid:return [1,0,0,3]
 # Independent literal decision tree mirroring the two complete originals.
 if a&224<96 or items&1 and o<8:return [0,2,0,2]
 if items&32 and a&224>63 and present:
  if af==0 or ph==af or aw<0 and ap<64:return [0,2,0,2]
 label=91;bonus=int(r&192>64)
 if items&32 and a&224>63 and bonus==0 and t!=2:
  if not present or not(aw<0 and ap<64 or ph==af):
   bonus=2 if ph==f or p<64 and w<0 else 1;label=338
 if items&0x200000 and bonus==0 and(not present or at==0):bonus=1;label=291
 return [0,bonus+2,label,2]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,str(HERE/'terrain_defense.cpp'),str(HERE/'terrain_defense_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 cs=list(cases());models=[model(c)for c in cs];data=b''.join(struct.pack('<14I',*(x&0xffffffff for x in c))for c in cs);want=b''.join(struct.pack('<4I',*m)for m in models);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(want);run=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];r=subprocess.run([*run,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr,next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));receipt={'cases':len(cs),'accepted':sum(m[0]==0 for m in models),'invalid':sum(m[0]!=0 for m in models),'guards':sum(m[3]for m in models),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
