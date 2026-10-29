# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,itertools
HERE=Path(__file__).resolve().parent
SOURCES=['combat_factions.cpp','combat_factions_probe.cpp']
def cases():
 for v,psi,drop,pod,fan,planet,faction,prof in itertools.product(range(2),range(4),range(2),range(2),range(2),[-2,0,3],[0,1],range(4)):
  factors=[(100,100),(150,75),(-50,125),(127,73)][prof];yield[v,864,144,psi,0,drop,pod,fan,faction,planet,25,20,10,2,2,*factors,*([0]*6),*factors,*([0]*6)]
 for i in range(10000):
  factors=[0,1,-1,75,150,2147483647,-2147483648];yield[i%2,[0,1,-1,864,2147483647,-2147483648][i%6],[0,1,-1,144,2147483647,-2147483648][(i//6)%6],i%4,0,1,1,1,(i//7)%8,[-2,0,3,2147483647,-2147483648][(i//11)%5],[25,-25,2147483647,-2147483648][(i//13)%4],[20,-20,2147483647,-2147483648][(i//17)%4],[10,-10,2147483647,-2147483648][(i//19)%4],i%9,(i//23)%9,*[factors[(i+j)%7]for j in range(8)],*[factors[(i+2*j)%7]for j in range(8)]]
 b=[1,864,144,3,0,1,1,1,1,3,25,20,10,2,2,150,75,*([0]*6),150,75,*([0]*6)]
 for idx,values in[(0,[-1,2]),(3,[-1,4]),(4,[-1,11,256]),(8,[-1,8]),(13,[-1,9]),(14,[-1,9])]:
  for value in values:c=b.copy();c[idx]=value;yield c
def model(c):
 v,off,deff,psi,plan,drop,pod,fan,faction,planet,dp,fp,pp,no,nd,*factors=c
 if v not in[0,1]or not(0<=psi<=3 and 0<=plan<=255 and plan!=11 and 0<=faction<=7 and 0<=no<=8 and 0<=nd<=8):return[1,0,0,0,3]
 mask=0;steps=[]
 if drop and pod and dp:steps.append((0,100-dp,1))
 if fan and fp and not psi:steps.append((0,100+fp,2))
 if v:steps.extend((0,p,4)for p in factors[:no])
 if psi and faction and planet and pp:
  if not -2**31<=planet*pp<2**31:return[2,0,0,0,3]
  steps.append((0,100+planet*pp,8))
 if v:steps.extend((1,p,16)for p in factors[8:8+nd])
 for side,f,bit in steps:
  n=deff if side else off
  if not -2**31<=f<2**31 or not -2**31<=n*f<2**31:return[2,0,0,0,3]
  n=n*f;n=abs(n)//100*(-1 if n<0 else 1)
  if side:deff=n
  else:off=n
  mask|=bit
 return[0,off,deff,mask,2]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x)for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cs=list(cases());ms=[model(c)for c in cs];data=b''.join(struct.pack('<31I',*(x&0xffffffff for x in c))for c in cs);want=b''.join(struct.pack('<5I',*(x&0xffffffff for x in m))for m in ms);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(want);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr,next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));d={'cases':len(cs),'accepted':sum(m[0]==0 for m in ms),'invalid':sum(m[0]==1 for m in ms),'overflow':sum(m[0]==2 for m in ms),'guards':sum(m[4]for m in ms),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d))
if __name__=='__main__':main()
