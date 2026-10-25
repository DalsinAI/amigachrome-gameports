# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only normal human technology choice pool, independent list reference."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent
FILES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','research_options.cpp','research_pool.cpp','research_pool_probe.cpp']
def expected(case):
 faction,rank,turn,human,blind,rows=case;path=0;result=[];error=0
 if faction<0 or faction>=8 or turn<0:error=1
 elif not human or blind or turn==0:path=1
 else:
  category=[max(range(4),key=lambda c:r[[3,1,2,0][c]]) for r in rows]
  for group in range(4):
   for i,row in enumerate(rows):
    if not row[4]:continue
    if result:
     n=i-rank
     if not(-2**31<=n<2**31):error=2;break
     n+=faction
     if not(-2**31<=n<2**31):error=2;break
     if n%3==0 and (i>=7 or rank!=0):continue
    if category[i]==group:result.append((i,group))
   if error:break
 if error:path=0;result=[]
 return struct.pack('<181I',error,path,len(result),*[i for i,c in result],*([0xffffffff]*(89-len(result))),*[c for i,c in result],*([0xffffffff]*(89-len(result))))
def cases():
 rng=random.Random(0x4868aa);out=[]
 for faction in range(8):
  for rank in [-10,-1,0,1,2,3,8,89,2147483647,-2147483648]:
   rows=[[*[rng.randrange(-5,8) for _ in range(4)],int(rng.randrange(4)!=0)] for i in range(89)];out.append([faction,rank,1,1,0,rows])
 for group in range(4):
  for chosen in [0,1,6,7,20,88]:
   rows=[]
   for i in range(89):
    values=[0]*4;values[[3,1,2,0][group]]=3;rows.append([*values,int(i==chosen)])
   out.append([1,(chosen+1)%3,1,1,0,rows])
 for rank in [0,1,2]:out.append([1,rank,1,1,0,[[1,1,1,1,1] for _ in range(89)]])
 rows=[[1,1,1,1,1] for _ in range(89)]
 for faction,turn,human,blind in [(-1,1,1,0),(8,1,1,0),(1,-1,1,0),(1,0,1,0),(1,1,0,0),(1,1,1,1),(1,0,0,1)]:out.append([faction,0,turn,human,blind,rows])
 out.append([1,-2147483648,1,1,0,[[1,1,1,1,0] for _ in range(89)]])
 return out
def from_original(cases):
 out=[]
 for faction,ranking,variant,flag,rows in cases:
  def known(i):
   if not faction:return False
   if i==-1:return True
   if i<0 or i>=88:return False
   a,b=rows[i][4:6];return a!=-2 and not(b==-2 and a!=-1) and bool(rows[i][6]&(1<<faction))
  converted=[]
  for i,row in enumerate(rows):
   a,b=row[4:6];available=not known(i) and (variant or not 78<=i<=86) and a>=-1 and b>=-1 and known(a) and known(b);converted.append([*row[:4],int(available)])
  out.append([faction,ranking,1,1,0,converted])
 return out
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-contexts',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);corpus=cases();base_count=len(corpus)
 if a.original_contexts:corpus+=from_original(json.loads(a.original_contexts.read_text()))
 raw=b''.join(struct.pack('<450i',*case[:5],*sum(case[5],[])) for case in corpus);(out/'inputs.bin').write_bytes(raw);flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];run=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(SRC/f) for f in FILES],'-o',str(out/'probe')],check=True)
 p=subprocess.run(run+[str(out/'probe'),str(out/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));want=b''.join(expected(c) for c in corpus);assert p.returncode==0 and p.stdout==want,(p.returncode,p.stderr,next((i for i in range(len(corpus)) if p.stdout[i*724:(i+1)*724]!=want[i*724:(i+1)*724]),None));(out/'outputs.bin').write_bytes(p.stdout);receipt={'contexts':len(corpus),'publicGeneratedContexts':base_count,'originalDecisionContexts':len(corpus)-base_count,'additionalInvalidControls':2,'outputSHA256':hashlib.sha256(p.stdout).hexdigest()};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
