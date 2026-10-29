# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,struct,json,subprocess,os,hashlib,random,itertools

def i32(v):v&=0xffffffff;return v if v<0x80000000 else v-0x100000000

def div(a,b):return(abs(a)//abs(b))*(1 if(a<0)==(b<0)else-1)

def model(c):
 won,ap,ar,ad,dp,dr,dd,sa,sd,ia,id_,morale,base=c
 if won not in [0,1]or any(x>255 or x<0 for x in [ap,ar,ad,dp,dr,dd])or not 1<=ia<=9999 or not 1<=id_<=9999 or not 0<=morale<=6:return(1,0,0xa5a5a5a5,0x31415926,0x31415926)
 pa=1 if ap==12 else 10*max(1,min(100,ar));pd=1 if dp==12 else 10*max(1,min(100,dr));ha=max(0,pa-ad);hd=max(0,pd-dd)
 if won:a=div(i32(ar*ha*sa),ia);d=div(i32(dr*id_*sd),pd);loser=dp;threshold=d
 else:a=div(i32(ar*ia*sa),pa);d=div(i32(dr*hd*sd),id_);loser=ap;threshold=a
 kind=1
 if loser==12:kind=0
 elif morale>1:
  if i32(i32(a+d)-1)>0:kind=2
  elif 0>threshold and(won or base<0 or morale>=4):kind=0
 return(0,kind,(0 if won else 1)if kind==1 else 0xffffffff,a&0xffffffff,d&0xffffffff)
def cases():
 rng=random.Random(0x506af0)
 for i in range(4096):
  won=i%2;ap=rng.choice([0,7,12,255]);dp=rng.choice([0,7,12,255]);ar=rng.choice([0,1,2,4,100,255]);dr=rng.choice([0,1,2,4,100,255]);pa=1 if ap==12 else 10*max(1,min(100,ar));pd=1 if dp==12 else 10*max(1,min(100,dr));ad=min(255,max(0,pa-1))if won else min(255,pa);dd=min(255,pd)if won else min(255,max(0,pd-1));sa=rng.choice([0,1,2,17,1000,0x7fffffff,0x40000000]);sd=rng.choice([0,1,2,31,1000,0x7fffffff,0x40000000]);ia=rng.choice([1,10,40,1000,9999]);id_=rng.choice([1,10,40,1000,9999]);morale=i%7;base=-1 if i%3 else 0
  yield[won,ap,ar,ad,dp,dr,dd,sa,sd,ia,id_,morale,base]
 for won,sa,sd,morale,base in itertools.product([0,1],[0,1,-1,-2147483648,2147483647],[0,1,-1,-2147483648,2147483647],range(7),[-1,0]):yield[won,0,1,0,0,1,0,sa,sd,10,10,morale,base]
 c=[1,0,1,0,0,1,10,17,31,10,10,1,-1]
 for at,values in [(0,[2,0xffffffff]),(1,[256]),(2,[256]),(3,[256]),(4,[256]),(5,[256]),(6,[256]),(9,[0,10000]),(10,[0,10000]),(11,[7,0xffffffff])]:
  for v in values:d=c.copy();d[at]=v;yield d

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();D=a.out.resolve();D.mkdir(parents=True,exist_ok=False);S=Path(__file__).resolve().parent;cs=list(cases());inputs=b''.join(struct.pack('<13I',*(v&0xffffffff for v in c))for c in cs);expected=b''.join(struct.pack('<5I',*model(c))for c in cs);(D/'inputs.bin').write_bytes(inputs);(D/'expected.bin').write_bytes(expected);(D/'cases.json').write_text(json.dumps(cs)+'\n')
 flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);subprocess.run([a.cc,*flags,*[str(S/s)for s in ['promotion_selection.cpp','combat_round.cpp','promotion_selection_probe.cpp']],'-o',str(D/'probe')],check=True);r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(D/'probe'),str(D/'inputs.bin')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(D/'actual.bin').write_bytes(r.stdout);(D/'stderr').write_bytes(r.stderr);assert not r.returncode,(r.returncode,r.stderr[:1200]);assert r.stdout==expected,(len(r.stdout),next((i for i,(x,y)in enumerate(zip(r.stdout,expected))if x!=y),None));counts={str(k):sum(model(c)[1]==k and model(c)[0]==0 for c in cs)for k in range(3)};receipt={'cases':len(cs),'accepted':sum(model(c)[0]==0 for c in cs),'invalid':sum(model(c)[0]!=0 for c in cs),'intentCounts':counts,'inputAliasNoWriteChecks':len(cs),'hostPoisonedInputNoRead':not a.m68k_linux,'outputSHA256':hashlib.sha256(expected).hexdigest(),'originalPrefixCases':4096,'originalPrefixOutputSHA256':hashlib.sha256(b''.join(struct.pack('<4I',*model(c)[1:])for c in cs[:4096])).hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest(),'native':'UNRUN'};(D/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
