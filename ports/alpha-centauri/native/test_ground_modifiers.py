# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,itertools
HERE=Path(__file__).resolve().parent
SOURCES=['ground_modifiers.cpp','ground_modifiers_probe.cpp']
PROFILES=[(0,0,0,0,0),(33,25,17,40,30),(-25,-20,-10,-15,-30)]
def original_cases():
 for a,d,base,t,alt,g,p in itertools.product([1,2,3],[1,2,3],range(2),range(4),range(3),range(4),range(3)):
  yield [864,144,2 if base or t==0 else 3,128 if t==1 else 0,base,(a+int(bool(g&1)))*3,(d+int(bool(g&2)))*3,3,160 if alt==2 else 96,160 if alt==1 else 96,g&1,g&2,*PROFILES[p]]
def cases():
 yield from original_cases()
 for i in range(12000):
  yield [[0,1,-1,2147483647,-2147483648][i%5],[0,-1,1,2147483647,-2147483648][(i//5)%5],2+i%3,[0,64,128,255][(i//3)%4],(i//7)%2,[0,3,6,9900][(i//11)%4],[0,3,6,9900][(i//17)%4],[1,3,100][(i//23)%3],i%256,(i*19)%256,(i//29)%2,(i//31)%2,*PROFILES[(i//37)%3]]
 b=[864,144,2,0,0,6,6,3,96,96,0,0,33,25,17,40,30]
 for idx,vs in [(2,[1,5]),(3,[-1,256]),(5,[-1,9901]),(6,[-1,9901]),(7,[0,101]),(8,[-1,256]),(9,[-1,256])]:
  for v in vs:c=b.copy();c[idx]=v;yield c
 for idx in range(12,17):
  for v in [-2147483648,2147483647]:c=b.copy();c[4]=1;c[8]=160 if idx==16 else 96;c[9]=160 if idx==15 else 96;c[idx]=v;yield c
def model(c):
 off,deff,t,rocky,base,a,d,road,aa,da,ag,dg,*pc=c
 if not(2<=t<=4 and 0<=rocky<=255 and 0<=aa<=255 and 0<=da<=255 and 0<=a<=9900 and 0<=d<=9900 and 1<=road<=100):return[1,0,0,0,3]
 tests=[not base and t==2 and rocky&192<128 and a>road and d<=a,(t>2 or base)and a>road,True,da&224>aa&224 and not ag,aa&224>da&224 and not dg];mask=0
 for i,(use,p)in enumerate(zip(tests,pc)):
  if not(use and p):continue
  n=deff if i in[1,3]else off;factor=100+p
  if not -2**31<=factor<2**31 or not -2**31<=factor*n<2**31:return[2,0,0,0,3]
  n=(abs(n*factor)//100)*(-1 if n*factor<0 else 1)
  if i in[1,3]:deff=n
  else:off=n
  mask|=1<<i
 return[0,off,deff,mask,2]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x)for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cs=list(cases());ms=[model(c)for c in cs];data=b''.join(struct.pack('<17I',*(x&0xffffffff for x in c))for c in cs);want=b''.join(struct.pack('<5I',*(x&0xffffffff for x in m))for m in ms);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(want);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr,next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));d={'cases':len(cs),'accepted':sum(m[0]==0 for m in ms),'invalid':sum(m[0]==1 for m in ms),'overflow':sum(m[0]==2 for m in ms),'guards':sum(m[4]for m in ms),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d))
if __name__=='__main__':main()
