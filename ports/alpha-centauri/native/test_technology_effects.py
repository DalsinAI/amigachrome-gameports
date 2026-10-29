# SPDX-License-Identifier: GPL-3.0-or-later
"""Complete original callback-free commerce/fungus recomputation."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
MIN=-2**31;MAX=2**31-1
def model(c):
 faction,commerce,economy,count=c[:4];rows=[c[80//4+i*4:80//4+i*4+4] for i in range(89)];bonus=list(zip(c[4:20:2],c[5:20:2]))
 if faction<0 or faction>7 or count<0 or count>8 or any(p<-2 or p>=89 for r in rows for p in r[1:3]):return 1,[0]*5
 n,m,e=0,0,0
 def checked(x):
  if not MIN<=x<=MAX:raise OverflowError
  return x
 try:
  for id,(flag,a,b,owner) in enumerate(rows):
   if faction>0 and id<88 and a>=-1 and (b>=-1 or a==-1) and owner&(1<<faction):
    n+=bool(flag&0x100);m+=bool(flag&0x80);e+=bool(flag&0x40)
    if flag&4:commerce=checked(commerce+1)
  for kind,value in bonus[:count]:
   if kind==7:n=checked(n+value)
   if kind==8:m=checked(m+value)
   if kind==9:e=checked(e+value)
  commerce=checked(commerce+(3 if economy>4 else 2 if economy>3 else 1 if economy>2 else 0))
 except OverflowError:return 2,[0]*5
 return 0,[commerce,max(0,n),max(0,m),max(0,e),0]
def cases():
 rng=random.Random(0x5bae60);out=[]
 for f in range(8):
  for econ in [-3,0,1,2,3,4,5,9]:
   for mode in range(4):
    bonuses=[[rng.choice([0,7,8,9,10]),rng.randrange(-20,21)] for _ in range(8)];rows=[[rng.randrange(512),rng.choice([-2,-1,0,88]),rng.choice([-2,-1,0,88]),0 if mode==0 else 255 if mode==1 else rng.randrange(256)] for _ in range(89)];out.append([f,rng.randrange(-20,21),econ,rng.randrange(9),*sum(bonuses,[]),*sum(rows,[])])
 def fixture():return [1,0,0,0,*([0]*16),*sum([[0,-1,-1,0] for _ in range(89)],[])]
 for f in [-1,8]:c=fixture();c[0]=f;out.append(c)
 for count in [-1,9]:c=fixture();c[3]=count;out.append(c)
 for p in [-3,89]:c=fixture();c[21]=p;out.append(c)
 c=fixture();c[1]=MAX;c[20:24]=[4,-1,-1,2];out.append(c)
 for econ in [3,4,5,MAX]:c=fixture();c[1]=MAX;c[2]=econ;out.append(c)
 for kind in [7,8,9]:
  for values in [[MAX,1],[MIN,-1],[MAX,1,-1],[-3,1],[MIN,MAX]]:
   c=fixture();c[3]=len(values)
   for i,value in enumerate(values):c[4+i*2:6+i*2]=[kind,value]
   out.append(c)
 for f in [0,1,7]:
  c=fixture();c[0]=f;c[20:24]=[0x1c4,-1,-2,255];c[20+88*4:24+88*4]=[0x1c4,-1,-1,255];out.append(c)
 return out
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-input',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);corpus=cases();public=len(corpus)
 if a.original_input:
  raw=a.original_input.read_bytes();assert len(raw)%1504==0;corpus+=list(map(list,struct.iter_unpack('<376i',raw)))
 raw=b''.join(struct.pack('<376i',*c) for c in corpus);(a.out/'inputs.bin').write_bytes(raw);flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(HERE/'technology_effects.cpp'),str(HERE/'technology_effects_probe.cpp'),'-o',str(a.out/'probe')],check=True)
 p=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(a.out/'probe'),str(a.out/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));want=b''.join(struct.pack('<6i',e,*r) for e,r in map(model,corpus));assert p.returncode==0 and p.stdout==want,(p.returncode,p.stderr,next((i for i in range(len(corpus)) if p.stdout[i*24:(i+1)*24]!=want[i*24:(i+1)*24]),None));(a.out/'outputs.bin').write_bytes(p.stdout);r={'contexts':len(corpus),'publicContexts':public,'originalContexts':len(corpus)-public,'additionalInvalidRulesCount':1,'overflowContexts':sum(model(c)[0]==2 for c in corpus),'invalidContexts':sum(model(c)[0]==1 for c in corpus),'outputSHA256':hashlib.sha256(p.stdout).hexdigest()};(a.out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
if __name__=='__main__':main()
