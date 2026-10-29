# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,subprocess,struct,hashlib,json,os
U=0xffffffff
def signed(v):return ((v+0x80000000)&U)-0x80000000
def cases():
 for edition in range(2):
  for i in range(4096):
   a=i%8;b=(a+1+i//8%7)%8;owner=i//56%8;contour=i//448%8;sd=[0,0x10,0x11,U][i//7%4];sa=[0,1,2,4,7,8,U][i//13%7];gift=[0,1,-1,2147483647][i//19%4];old=[-2147483648,-1,0,1,20,21,2147483647][i//23%7];mode=i%3;runtime=[-2147483648,-1,0,20,2147483647][i//29%5]
   if i>=2048:contour=3+i%5;owner=next(v for v in range(8)if v not in[a,b]);sd=0x10;sa=[1,2,4,7][i%4]
   yield [edition,contour<<5,owner,a,b,sd,sa,gift,old,owner if mode!=1 else(owner+1)%8,a if mode!=2 else(a+1)%8,runtime,1]
 seed=[0,96,2,0,1,16,7,0,-2147483648,2,0,-2147483648,1]
 for j,v in [(0,2),(1,256),(2,-129),(2,128),(3,-1),(3,8),(4,-1),(4,8),(4,0),(12,0),(12,2),(2,-1),(2,8)]:
  c=seed[:];c[j]=v;yield c
 for edition in range(2):
  for climate in [0,31,32,63,64,95,96,127,128,159,160,191,192,223,224,255]:
   for owner in [-128,-1,0,1,2,7,8,127]:
    c=seed[:];c[0]=edition;c[1]=climate;c[2]=owner;yield c
def model(c):
 edition,climate,owner,a,b,sd,sa,gift,old,na,nb,nv,obs=c
 if obs!=1:return [2]+[0]*7
 if edition not in[0,1]or not 0<=climate<=255 or not -128<=owner<=127 or not 0<=a<=7 or not 0<=b<=7 or a==b:return [1]+[0]*7
 if climate>=96 and not 0<=owner<=7:return [3]+[0]*7
 hit=climate>=96 and owner not in[a,b]and sd&16 and sa&7
 return [0,int(bool(hit)),gift,1 if hit and gift==0 else gift,old,max(0,min(20,signed(old-1)))if hit else old,nv,(nv-1)&U if hit and na==owner and nb==a else nv]
def pack(values):return struct.pack('<'+'I'*len(values),*[v&U for v in values])
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();D=a.out.resolve();D.mkdir(parents=True,exist_ok=False);S=Path(__file__).resolve().parent
 flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);subprocess.run([a.cc,*flags,str(S/'territorial_combat.cpp'),str(S/'territorial_combat_probe.cpp'),'-o',str(D/'probe')],check=True)
 cs=list(cases());rows=[model(c)for c in cs];inputs=b''.join(pack(c)for c in cs);expected=b''.join(pack(c)for c in rows);(D/'inputs.bin').write_bytes(inputs);(D/'expected.bin').write_bytes(expected)
 r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(D/'probe')],input=inputs,capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(D/'actual.bin').write_bytes(r.stdout);(D/'stderr').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==expected,next((i for i,(a,b)in enumerate(zip(expected,r.stdout))if a!=b),None);assert r.stderr==f'ALIASES {len(cs)}\n'.encode()
 counts={str(k):sum(r[0]==k for r in rows)for k in range(4)};j={'cases':len(cs),'statusCounts':counts,'applied':sum(r[1]for r in rows),'poisonedAliasNoReadHostOrNoWriteBE':len(cs),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest(),'native':'UNRUN'};(D/'receipt.json').write_text(json.dumps(j,indent=2)+'\n');print(json.dumps(j))
if __name__=='__main__':main()
