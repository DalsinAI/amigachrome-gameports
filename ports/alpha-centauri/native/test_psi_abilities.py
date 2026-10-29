# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,itertools
HERE=Path(__file__).resolve().parent
SOURCES=['psi_abilities.cpp','psi_abilities_probe.cpp']
PROFILES=[(0,0),(35,60),(-25,-40)]
def cases():
 for v in range(2):
  for psi,w,a,plan,ab,di,p in itertools.product(range(4),[0,12,13,25]if v else[0,12,13,20],[0,10,11,13]if v else[0,9],[0,12],range(4),range(4),range(3)):
   yield [v,864,576,psi,w,a,plan,di&1,di&2,ab&1,ab&2,*PROFILES[p]]
 for i in range(9000):
  yield [i%2,[0,1,-1,2147483647,-2147483648][i%5],[0,1,-1,2147483647,-2147483648][(i//5)%5],i%4,(i//7)%(26 if i%2 else 21),(i//11)%(14 if i%2 else 10),[0,12,255][(i//13)%3],(i//17)%2,(i//19)%2,(i//23)%2,(i//29)%2,*PROFILES[(i//31)%3]]
 b=[1,864,576,3,12,11,0,0,0,1,1,35,60]
 for idx,values in [(0,[-1,2]),(3,[-1,4]),(4,[-1,26]),(5,[-1,14]),(6,[-1,11,256])]:
  for value in values:c=b.copy();c[idx]=value;yield c
 for idx in [11,12]:
  for value in [-2147483648,2147483647]:c=b.copy();c[idx]=value;yield c
def model(c):
 v,off,deff,psi,w,a,plan,ad,dd,em,tr,ep,tp=c
 if v not in[0,1]or not(0<=psi<=3 and 0<=w<(26 if v else 21)and 0<=a<(14 if v else 10)and 0<=plan<=255 and plan!=11):return[1,0,0,0,3]
 mask=0
 for i,(use,p)in enumerate([(v and psi&2 and w in[12,13],25),(psi&2 and em and(not v or not dd),ep),(psi&1 and tr and(not v or not ad),tp),(v and psi&1 and a in[11,13],25)]):
  if not(use and p):continue
  n=off if i<2 else deff;f=100+p
  if not -2**31<=f<2**31 or not -2**31<=f*n<2**31:return[2,0,0,0,3]
  n=f*n;n=(abs(n)//100)*(-1 if n<0 else 1)
  if i<2:off=n
  else:deff=n
  mask|=1<<i
 return[0,off,deff,mask,2]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x)for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cs=list(cases());ms=[model(c)for c in cs];data=b''.join(struct.pack('<13I',*(x&0xffffffff for x in c))for c in cs);want=b''.join(struct.pack('<5I',*(x&0xffffffff for x in m))for m in ms);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(want);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr,next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));d={'cases':len(cs),'accepted':sum(m[0]==0 for m in ms),'invalid':sum(m[0]==1 for m in ms),'overflow':sum(m[0]==2 for m in ms),'guards':sum(m[4]for m in ms),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d))
if __name__=='__main__':main()
