# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,itertools
HERE=Path(__file__).resolve().parent
SOURCES=['basic_combat.cpp','basic_combat_probe.cpp']
def model(c):
 v,uid,off,armor,plan,opplan,morale,active,pending,base,creche,brood,native,disso,sop,mode,attack,defense=c
 bad=v not in [0,1] or not 0<=uid<512 or not -128<=off<=127 or not -128<=armor<=127 or not 0<=plan<=255 or not 0<=opplan<=255 or not 0<=morale<=6
 if bad:return [1,0,0,0,0,0,3]
 a=d=morale
 brood=bool(v and brood and uid<64 and (off<0 or uid==15))
 gas=bool(v and not native and (uid>=64 or off>=0 and uid!=15) and not disso and sop)
 def modifier(high):
  x=max(-4,min(high,active));return x+int(x<=-2)
 if base:
  if creche:a+=1-modifier(4)+int(brood);d+=1-modifier(0)+int(brood)
  elif brood:a+=1-modifier(4);d+=1-modifier(4)
  extra=int(2<=pending<=3)+int(bool(native));d+=extra
  if mode:a+=extra-2*int(gas)
 if mode:a=max(1,min(6,a))
 d=max(1,min(6,d-2*int(gas)))
 fixed=plan==12 or plan==11 and armor==1 and opplan!=11
 first=attack*(a+6);second=1 if fixed else defense*(d+6)
 if any(not -2**31<=x<2**31 for x in [first,first*4,second]):return [2,0,0,0,0,0,3]
 return [0,first*4,second,a,d,int(fixed),2]
def cases():
 for i,(v,uid,morale,active,base,fac,mode,abilities) in enumerate(itertools.product(range(2),[320,15,8],range(7),[-9,-4,-3,-2,0,2,3,4,9],range(2),range(4),range(2),range(8))):
  yield [v,uid,-1 if uid==8 else 3,[-1,1,5][i%3],[0,11,12][(i//5)%3],11 if i%4==0 else 0,morale,active,[-9,1,2,3,4][(i//11)%5],base,fac&1,(fac>>1)&1,abilities&1,(abilities>>1)&1,(abilities>>2)&1,mode,[0,8,-8,64,2**31-1,-2**31][(i//13)%6],[-8,8,32,2**31-1,-2**31][(i//17)%5]]
 base=[0,320,3,1,0,0,3,0,0,1,0,0,0,0,0,0,8,8]
 for idx,values in [(0,[-1,2]),(1,[-1,512]),(2,[-129,128]),(3,[-129,128]),(4,[-1,256]),(5,[-1,256]),(6,[-1,7])]:
  for value in values:c=base.copy();c[idx]=value;yield c

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cs=list(cases());data=b''.join(struct.pack('<18i',*c) for c in cs);ms=[model(c) for c in cs];expected=b''.join(struct.pack('<7I',*(x&0xffffffff for x in m)) for m in ms);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(expected);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==expected,(r.returncode,r.stderr,next((i for i,(x,y) in enumerate(zip(r.stdout,expected)) if x!=y),None));receipt={'cases':len(cs),'accepted':sum(m[0]==0 for m in ms),'invalid':sum(m[0]==1 for m in ms),'overflow':sum(m[0]==2 for m in ms),'guards':sum(m[6] for m in ms),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
