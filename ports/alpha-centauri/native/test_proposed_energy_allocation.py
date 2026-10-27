#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Current-population worker proposals supply fresh specialists without world mutation."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
import test_world_energy_allocation as world
from test_energy_allocation import expected,SENTINEL
HERE=Path(__file__).resolve().parent
NAMES=world.NAMES[:-1]+['proposed_energy_allocation_probe']
def proposed_fixture(version,case):
 data=bytearray(world.fixture(version,case%70));_,offset,_=world.intake.decode(data)
 # A stale impossible specialist total must not block a valid new assignment.
 if case%11==0:struct.pack_into('<i',data,offset['bases']+0x7c,2147483647)
 return bytes(data)
def proposal(data,case):
 _,_,spans=world.intake.decode(data);pop=struct.unpack_from('<b',spans['bases'],6)[0];rng=random.Random(0xC1712E+case);workers=1+case%min(21,pop+1);count=pop+1-workers;bits=(1<<workers)-1;types=[0,0]
 for i in range(min(count,16)):
  kind=2 if case%3==0 and pop>=5 else 0 if case%5==0 and pop>=5 else rng.randrange(7)
  types[i//8]|=kind<<(4*(i%8))
 p=[pop,bits,count,*types]
 if case%17==1:p[0]=pop+1
 if case%17==2:p[1]&=~1
 if case%17==3:p[1]|=0x200000
 if case%17==4:p[2]=-1
 if case%17==5:p[2]=pop+1
 if case%17==6 and count:p[3]|=15
 if case%17==7:p[1]=1;p[2]=0
 return p
def model(text,data,base,surplus,commerce,p):
 words=world.world_words(text,data,base,surplus,commerce);fail=lambda:[1]+[SENTINEL]*13
 if words is None:return fail()
 pop,worked,count,t0,t1=p
 if pop!=words[4]or not 0<=pop<=127 or not 0<=count<=pop or not worked&1 or worked&~0x1fffff or worked.bit_count()+count!=pop+1:return fail()
 if any(((p[3+i//8]>>(4*(i%8)))&15)>6 for i in range(min(count,16))):return fail()
 words[5]=count;words[7:9]=[t0,t1];result=expected(words)
 if result[0]:return result
 pre=words[9:16];obsolete=words[16:23];citizens=[words[i:i+5]for i in range(24,59,5)];minimum=words[6]
 valid=lambda kind:pre[kind]and not obsolete[kind]and(citizens[kind][3]>=2 or pop>=minimum)
 for i in range(min(count,16)):
  kind=(result[9+i//8]>>(4*(i%8)))&15
  if not valid(kind):return fail()
 if count>16 and not any(valid(kind)for kind in range(7)):return fail()
 return result
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--binary',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.binary or a.out/'proposed-energy-allocation-probe';cmd=[]
 if not a.binary:
  cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(n+'.cpp'))for n in NAMES],'-o',str(exe)];subprocess.run(cmd,check=True)
 text=world.rules();rule=a.out/'base.txt';rule.write_bytes(text);records=[]
 for version in range(7,11):
  for case in range(70):
   data=proposed_fixture(version,case);path=a.out/f'v{version}-{case}.SC';path.write_bytes(data);p=proposal(data,case);surplus=case%31;commerce=case%5;want=model(text,data,0,surplus,commerce,p);run=subprocess.run([str(exe),str(rule),str(path),'0',str(surplus),str(commerce),*map(str,p)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30);assert run.returncode==0,(path,run.returncode,run.stderr);got=list(map(int,run.stdout.split()));assert got==want,(path,p,got,want);records.append(dict(name=path.name,sha256=hashlib.sha256(data).hexdigest(),proposal=p,surplus=surplus,commerce=commerce,result=got,passed=True))
 receipt=dict(cases=len(records),command=cmd,casesDetail=records,native='UNRUN',policy='Qualified current-population citizen assignment with existing pinned allocation policy');(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(records)} proposed assignment allocation cases passed')
if __name__=='__main__':main()
