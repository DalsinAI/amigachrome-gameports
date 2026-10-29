# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,itertools
HERE=Path(__file__).resolve().parent
SOURCES=['combat_prototype.cpp','combat_prototype_probe.cpp']
def trunc(n,d):return (abs(n)//abs(d))*(-1 if (n<0)!=(d<0) else 1)
def i32(n):return -2**31<=n<2**31
def model(c):
 v,p,o,off,ooff,arm,oarm,mode,plan,missile,triad,otriad,bombard,*tail=c;atk=tail[:3];deff=tail[3:6];strength,rule,neural,dream,attacking,tower=tail[6:];bad=v not in [0,1] or not(0<=p<512 and 0<=o<512) or any(not(-128<=x<=127) for x in [off,ooff,arm,oarm]) or not(0<=mode<=255 and 0<=missile<=255) or triad not in [0,1,2] or otriad not in [0,1,2] or any(not(1<=x<=1000) for x in atk+deff)
 if bad:pe,x,y,flags=1,0,0,0
 elif mode==11 and plan==11:pe,x,y,flags=0,16,16,4
 else:
  spore=v==1 and (p==15 or o==15);op=not((bool(bombard) or off>=0 and oarm>=0) and not spore);dp=(not bombard or spore) and (arm<0 or ooff<0);x=atk[otriad]*8 if op else abs(off)*8
  if not op and missile and off<99:x=trunc(abs(off)*3,2)*8
  y=deff[triad]*8 if dp else max(1,arm)*8;pe=0;flags=int(op)+2*int(dp)
 factor=strength;fe=1 if v not in [0,1] else 0
 if not fe:
  if rule:
   percent=rule+100
   if not i32(percent) or not i32(percent*factor):fe=2
   else:factor=trunc(percent*factor,100)
  if not fe and ((attacking and dream) or (not attacking and neural)):
   factor+=trunc(factor,2)
   if not i32(factor):fe=2
  if not fe and v==1 and not attacking and tower:
   factor+=trunc(factor,2)
   if not i32(factor):fe=2
 if fe:factor=0x12345678
 return [pe,x,y,flags,fe,factor,2+int(pe!=0)+int(fe!=0)]
def cases():
 n=0
 for v,(pid,oid),off,arm,missile,triad,bombard,ooff,oarm in itertools.product(range(2),[(64,65),(15,65),(64,15)],[-128,-1,0,1,98,99,127],[-1,0,1,99,127],[0,1],[0,1,2],[0,1],[-1,0,3],[-1,0,2]):
  yield [v,pid,oid,off,ooff,arm,oarm,0,0,missile,triad,(n//3)%3,bombard,3,5,7,2,4,6,[-129,-1,0,1,7,99][n%6],[-150,-101,-100,-99,-1,0,1,50,200][n%9],(n//3)%2,(n//5)%2,(n//7)%2,(n//11)%2];n+=1
 for v,bombard,mode,plan in itertools.product(range(2),range(2),[0,11], [0,11]):yield [v,64,65,-1,-1,-1,-1,mode,plan,1,1,2,bombard,3,5,7,2,4,6,101,50,1,1,0,1]
 base=[0,64,65,3,2,1,2,0,0,1,0,1,0,3,5,7,2,4,6,3,0,0,0,0,0]
 for index,values in [(0,[-1,2]),(1,[-1,512]),(2,[-1,512]),(3,[-129,128]),(4,[-129,128]),(5,[-129,128]),(6,[-129,128]),(7,[-1,256]),(9,[-1,256]),(10,[-1,3]),(11,[-1,3]),(13,[0,1001])]:
  for value in values:c=base.copy();c[index]=value;yield c
 for v,s,r,neural,dream,attacking,tower in itertools.product(range(2),[-2**31,2**31-1,-101,0,101],[-2**31,2**31-1,-100,0,50],range(2),range(2),range(2),range(2)):
  c=base.copy();c[0]=v;c[19:]=[s,r,neural,dream,attacking,tower];yield c

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cs=list(cases());data=b''.join(struct.pack('<25i',*c) for c in cs);expected=b''.join(struct.pack('<7I',*(x&0xffffffff for x in model(c))) for c in cs);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(expected);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==expected,(r.returncode,r.stderr,next((i for i,(x,y) in enumerate(zip(r.stdout,expected)) if x!=y),None));ms=[model(c) for c in cs];receipt={'cases':len(cs),'prototypeAccepted':sum(m[0]==0 for m in ms),'prototypeRejected':sum(m[0]!=0 for m in ms),'psiAccepted':sum(m[4]==0 for m in ms),'psiInvalid':sum(m[4]==1 for m in ms),'psiOverflow':sum(m[4]==2 for m in ms),'guards':sum(m[6] for m in ms),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
