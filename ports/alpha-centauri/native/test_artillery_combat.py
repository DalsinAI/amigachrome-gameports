# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,itertools
HERE=Path(__file__).resolve().parent
SOURCES=['artillery_combat.cpp','artillery_combat_probe.cpp']
PROFILES=[(0,0),(35,25),(-25,-15)]
def cases():
 for at,dt,a,d,flags,p in itertools.product(range(3),range(3),range(8),range(8),[f for f in range(32)if f&2],range(3)):
  yield [864,576,at,dt,a*32,d*32,flags,*PROFILES[p]]
 for i in range(9000):
  yield [[0,1,-1,2147483647,-2147483648][i%5],[0,1,-1,2147483647,-2147483648][(i//5)%5],i%3,(i//3)%3,i%256,(i*23)%256,2|((i//7)%32),*PROFILES[(i//11)%3]]
 b=[864,576,0,0,224,0,2,35,25]
 for index,values in [(2,[-1,3]),(3,[-1,3]),(4,[-1,256]),(5,[-1,256]),(6,[-1,0,1,32])]:
  for value in values:c=b.copy();c[index]=value;yield c
 for idx in [7,8]:
  for value in [-2147483648,2147483647]:c=b.copy();c[idx]=value;c[3]=1 if idx==7 else 0;yield c
def model(c):
 off,deff,at,dt,a,d,flags,gp,ap=c
 if not(0<=at<=2 and 0<=dt<=2 and 0<=a<=255 and 0<=d<=255 and 0<=flags<=31 and flags&2):return[1,0,0,0,3]
 mask=0;p=0;defend=False
 if not flags&24:
  if at==0 and dt==1:p=gp;mask=1
  elif at==1 and dt==0:p=gp;mask=2;defend=True
  elif a>>5>d>>5:p=((a>>5)-(d>>5))*ap;mask=4
  elif d>>5>a>>5:p=((d>>5)-(a>>5))*ap;mask=8;defend=True
 if not p:return[0,off,deff,0,2]
 n=deff if defend else off;factor=100+p
 if not -2**31<=p<2**31 or not -2**31<=factor<2**31 or not -2**31<=factor*n<2**31:return[2,0,0,0,3]
 n=factor*n;n=(abs(n)//100)*(-1 if n<0 else 1)
 if defend:deff=n
 else:off=n
 return[0,off,deff,mask,2]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x)for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cs=list(cases());ms=[model(c)for c in cs];data=b''.join(struct.pack('<9I',*(x&0xffffffff for x in c))for c in cs);want=b''.join(struct.pack('<5I',*(x&0xffffffff for x in m))for m in ms);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(want);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr,next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));d={'cases':len(cs),'accepted':sum(m[0]==0 for m in ms),'invalid':sum(m[0]==1 for m in ms),'overflow':sum(m[0]==2 for m in ms),'guards':sum(m[4]for m in ms),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d))
if __name__=='__main__':main()
