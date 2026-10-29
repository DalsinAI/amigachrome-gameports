# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,itertools
HERE=Path(__file__).resolve().parent
SOURCES=['defense_abilities.cpp','defense_abilities_probe.cpp']
PROFILES=[(0,0),(35,60),(-25,-40)]
def original_cases():
 for v in range(2):
  for triad,speed,diss,abilities,armor,p in itertools.product(range(3),[1,2,3],range(2),range(4),[0,10,12]if v else[0],range(3)):
   yield [v,576,triad,speed,armor,diss,abilities&1,abilities&2,*PROFILES[p]]
def cases():
 yield from original_cases()
 for i in range(8000):
  yield [i%2,[0,1,-1,2147483647,-2147483648][(i//2)%5],(i//7)%3,[0,1,2,255][(i//11)%4],(i//13)%(14 if i%2 else 10),(i//17)%2,(i//19)%2,(i//23)%2,*PROFILES[(i//29)%3]]
 b=[1,576,0,2,10,0,1,1,35,60]
 for idx,values in [(0,[-1,2]),(2,[-1,3]),(3,[-1,256]),(4,[-1,14])]:
  for value in values:c=b.copy();c[idx]=value;yield c
 yield [0,576,0,2,10,0,1,1,35,60]
 for index in [8,9]:
  for value in [-2147483648,2147483647]:
   c=b.copy();c[index]=value;c[2]=0 if index==8 else 2;yield c
def model(c):
 v,deff,triad,speed,armor,diss,jammer,tracking,jp,tp=c
 if v not in[0,1]or not 0<=triad<3 or not 0<=speed<=255 or not 0<=armor<(14 if v else 10):return[1,0,0,3]
 mask=0;mobile=triad==0 and speed>1
 for i,(use,p)in enumerate([(v==1 and armor in[10,12]and mobile,25),(jammer and mobile and(v==0 or not diss),jp),(tracking and triad==2 and(v==0 or not diss),tp)]):
  if not(use and p):continue
  factor=100+p
  if not -2**31<=factor<2**31 or not -2**31<=factor*deff<2**31:return[2,0,0,3]
  n=factor*deff;deff=(abs(n)//100)*(-1 if n<0 else 1);mask|=1<<i
 return[0,deff,mask,2]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x)for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cs=list(cases());ms=[model(c)for c in cs];data=b''.join(struct.pack('<10I',*(x&0xffffffff for x in c))for c in cs);want=b''.join(struct.pack('<4I',*(x&0xffffffff for x in m))for m in ms);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(want);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr,next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));d={'cases':len(cs),'accepted':sum(m[0]==0 for m in ms),'invalid':sum(m[0]==1 for m in ms),'overflow':sum(m[0]==2 for m in ms),'guards':sum(m[3]for m in ms),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d))
if __name__=='__main__':main()
