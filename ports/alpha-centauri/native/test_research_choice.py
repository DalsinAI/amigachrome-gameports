# SPDX-License-Identifier: GPL-3.0-or-later
"""Pure externally supplied manual research-choice field transition."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent
def expected(c):
 target,labs,cost,stage,choice=c;confirm=int(target>=0 and labs!=0)
 if stage not in [0,1]:return struct.pack('<7i',1,0,0,0,0,0,confirm)
 if stage==1 and choice>88:return struct.pack('<7i',2,0,0,0,0,0,confirm)
 disposition=0;sync=0
 if stage==1:
  sync=1
  if choice<0:disposition=1
  elif choice==target:disposition=2
  else:
   disposition=3 if target<0 else 4
   if target>=0:labs=(abs(labs)//2)*(-1 if labs<0 else 1)
   target=choice
 return struct.pack('<7i',0,target,labs,cost,disposition,sync,confirm)
def cases():
 out=[];rng=random.Random(0x51508a)
 for labs in [-2**31,-101,-3,-1,0,1,3,101,2**31-1]:
  for old in [-2**31,-2,-1,0,6,78,88,89,2**31-1]:
   for choice in [-2**31,-2,-1,0,6,78,88,89,2**31-1]:
    for stage in [0,1]:out.append([old,labs,rng.randrange(-2**31,2**31),stage,choice])
 for _ in range(1024):out.append([rng.randrange(-2**31,2**31),rng.randrange(-2**31,2**31),rng.randrange(-2**31,2**31),rng.choice([0,1]),rng.randrange(-1,89)])
 for stage in [-1,2,99,255]:out.append([6,101,250,stage,7])
 return out
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-corpus',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 corpus=cases();public=len(corpus);original=0
 if a.original_corpus:
  data=a.original_corpus.read_bytes();assert len(data)%20==0
  for faction,labs,old,cost,choice in struct.iter_unpack('<5i',data):
   if choice<=88:corpus.append([old,labs,cost,1,choice]);original+=1
 raw=b''.join(struct.pack('<5i',*c) for c in corpus);(out/'inputs.bin').write_bytes(raw)
 flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];run=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(SRC/'research_choice.cpp'),str(SRC/'research_choice_probe.cpp'),'-o',str(out/'probe')],check=True)
 p=subprocess.run(run+[str(out/'probe'),str(out/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));want=b''.join(expected(c) for c in corpus);assert p.returncode==0 and p.stdout==want,(p.returncode,p.stderr,next((i for i in range(len(corpus)) if p.stdout[i*28:(i+1)*28]!=want[i*28:(i+1)*28]),None))
 # The probe also checks output-state aliasing on success and poisoned output
 # preservation on every error. Malformed/truncated packet input must fail.
 (out/'truncated.bin').write_bytes(raw[:19]);bad=subprocess.run(run+[str(out/'probe'),str(out/'truncated.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert bad.returncode==20 and not bad.stdout
 (out/'outputs.bin').write_bytes(p.stdout);r={'contexts':len(corpus),'publicContexts':public,'originalPostChoiceContexts':original,'aliasAndTransactionalControls':True,'truncatedInputRejected':True,'outputSHA256':hashlib.sha256(p.stdout).hexdigest()};(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
if __name__=='__main__':main()
