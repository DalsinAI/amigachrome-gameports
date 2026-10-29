# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,itertools
HERE=Path(__file__).resolve().parent
SOURCES=['air_port_combat.cpp','air_port_combat_probe.cpp']
PROFILES=[(0,0,0,0),(35,60,25,40),(-25,-40,-20,-30)]
def cases():
 for at,dt,sup,miss,base,nc,native,p in itertools.product(range(3),range(3),range(4),range(4),range(2),range(2),range(2),range(3)):
  yield [864,576,at,dt,sup&1,sup&2,miss&1,miss&2,0 if nc else 3,1 if nc else 2,0 if native else 1,base,*PROFILES[p]]
 for i in range(9000):
  yield [[0,1,-1,2147483647,-2147483648][i%5],[0,1,-1,2147483647,-2147483648][(i//5)%5],i%3,(i//3)%3,(i//7)%2,(i//11)%2,(i//13)%2,(i//17)%2,[0,1,3,-1,-128,127][(i//19)%6],[0,1,2,-1,-128,127][(i//23)%6],(i//29)%8,(i//31)%2,*PROFILES[(i//37)%3]]
 b=[864,576,2,1,1,0,0,0,0,1,1,1,35,60,25,40]
 for idx,values in [(2,[-1,3]),(3,[-1,3]),(8,[-129,128]),(9,[-129,128]),(10,[-1,8])]:
  for value in values:c=b.copy();c[idx]=value;yield c
 for idx in range(12,16):
  for value in [-2147483648,2147483647]:c=b.copy();c[idx]=value;c[3]=2 if idx==13 else 1;yield c
def model(c):
 off,deff,at,dt,asu,dsu,ami,dmi,weapon,armor,faction,base,gp,ap,np,pp=c
 if not(0<=at<=2 and 0<=dt<=2 and -128<=weapon<=127 and -128<=armor<=127 and 0<=faction<=7):return[1,0,0,0,3]
 tests=[at==2 and asu and dt!=2,at==2 and asu and dt==2 and not dsu,at==2 and dt==2 and dsu and not asu and not ami and not dmi,weapon==0 and armor==1 and(faction!=0 or not base),at!=1 and dt==1 and base];mask=0
 for i,(use,p,minus)in enumerate(zip(tests,[gp,ap,ap,np,pp],[True,False,False,True,False])):
  if not(use and p):continue
  factor=100-p if minus else 100+p;n=deff if i in[2,3]else off
  if not -2**31<=factor<2**31 or not -2**31<=factor*n<2**31:return[2,0,0,0,3]
  n=factor*n;n=(abs(n)//100)*(-1 if n<0 else 1)
  if i in[2,3]:deff=n
  else:off=n
  mask|=1<<i
 return[0,off,deff,mask,2]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x)for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cs=list(cases());ms=[model(c)for c in cs];data=b''.join(struct.pack('<16I',*(x&0xffffffff for x in c))for c in cs);want=b''.join(struct.pack('<5I',*(x&0xffffffff for x in m))for m in ms);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(want);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr,next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));d={'cases':len(cs),'accepted':sum(m[0]==0 for m in ms),'invalid':sum(m[0]==1 for m in ms),'overflow':sum(m[0]==2 for m in ms),'guards':sum(m[4]for m in ms),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d))
if __name__=='__main__':main()
