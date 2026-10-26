#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original prefix arithmetic, independent exact integer model and boundaries."""
import argparse,hashlib,itertools,json,os,random,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent
MIN,MAX=-2147483648,2147483647
S=0x31415926

def model(row):
 profile,flags,intake,surplus,food,stock,item,best=row
 fail=lambda e:(e,S,S,S,1,1)
 if profile not in (0,1):return fail(1)
 doubled=2*surplus;candidate=intake+doubled
 if not MIN<=doubled<=MAX or not MIN<=candidate<=MAX:return fail(2)
 flags&=~0x800000;best=max(best,candidate);abort=food<0;accrue=not abort and surplus>=0 and item!=(-38 if profile==0 else -69) and not flags&2
 if accrue:
  stock+=surplus
  if not MIN<=stock<=MAX:return fail(2)
 return 0,flags,best,stock,int(abort),int(accrue)

def contexts():
 for profile,flags,intake,surplus,food,stock,item,best in itertools.product(range(2),[0,2,8,0x800000,0x800002,0xffffffff],[-1,17],[-9,-1,0,1,13],[-1,0,7],[-1,0,111,MAX],[0,-38,-69,-70],[MIN,-7,MAX]):yield profile,flags,intake,surplus,food,stock,item,best
 for profile,intake,surplus,food,stock,flags in itertools.product(range(2),[MIN,MIN+1,-1,0,1,MAX-1,MAX],[-1073741825,-1073741824,-1,0,1,1073741823,1073741824],[MIN,-1,0,MAX],[MIN,-1,0,MAX],[0,2,0x800000]):yield profile,flags,intake,surplus,food,stock,0,MIN
 rng=random.Random(851390)
 for i in range(5000):yield i%2,rng.randrange(1<<32),rng.randrange(MIN,MAX+1),rng.randrange(MIN,MAX+1),rng.choice([-1,0,1]),rng.randrange(MIN,MAX+1),rng.choice([MIN,MAX,-38,-69,0,320]),rng.randrange(MIN,MAX+1)
 for profile in [-1,2]:yield profile,0,0,0,0,0,0,0

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 rows=list(contexts());data=b''.join(struct.pack('<iI6i',*r) for r in rows);expected=b''.join(struct.pack('<6I',*[v&0xffffffff for v in model(r)]) for r in rows);(out/'inputs.bin').write_bytes(data)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 exe=out/'production-accrual-probe';cmd=[a.cc,*flags,str(SRC/'production_accrual.cpp'),str(SRC/'production_accrual_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True);r=subprocess.run([*runner,str(exe),str(out/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=180);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(expected)
 for i,row in enumerate(rows):assert r.stdout[i*24:(i+1)*24]==expected[i*24:(i+1)*24],(row,model(row),struct.unpack_from('<6I',r.stdout,i*24))
 (out/'outputs.bin').write_bytes(r.stdout);results=[model(r) for r in rows];receipt=dict(cases=len(rows),accepted=sum(r[0]==0 for r in results),invalid=sum(r[0]==1 for r in results),overflow=sum(r[0]==2 for r in results),negativeFoodAbort=sum(r[0]==0 and r[4] for r in results),accrualBranches=sum(r[0]==0 and r[5] for r in results),inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
