# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess
import test_world_battle_reporting as parent
from test_original_game import decode
SOURCES=[s for s in parent.SOURCES if s!='world_battle_reporting_probe.cpp']+['combat_labels.cpp','world_combat_text.cpp','world_combat_text_probe.cpp']
def pack(ns):return struct.pack('<'+'I'*len(ns),*(x&0xffffffff for x in ns))
def label_file(rows,crlf=False):
 e=b'\r\n'if crlf else b'\n';return b'; synthetic private-free labels'+e+b'#LABELS'+e+str(len(rows)).encode()+e+e.join(rows)+e+b'# ; tail'+e
def expected(text,data,labels):
 raw,counts=parent.expected(text,data);info,_,_=decode(data);output=[];accepted=lookups=missing=longnames=0
 for offset in range(0,len(raw)-4,316):
  words=list(struct.unpack_from('<79I',raw,offset));error=words[0];display=[];resolved=0;names=[bytearray(b'Z'*80)for _ in range(4)];percents=[0x5a5a5a5a]*4
  if not error:
   events=[(words[30+3*i],words[31+3*i])for i in range(words[29])]
   if words[18]:events.append((339,words[16]))
   for lab,pct in events:
    if lab>=len(labels):error=5;missing+=1;break
    resolved+=1;value=labels[lab].strip(b' ')
    if pct and len(display)<4:
     if len(value)>79:error=6;longnames+=1;break
     slot=len(display);display.append((lab,pct));names[slot][:len(value)+1]=value+b'\0';percents[slot]=pct
   if not error:
    assert len(display)==words[69]and resolved==words[78]
    for i,(lab,pct)in enumerate(display):assert [lab,pct]==words[70+2*i:72+2*i]
  if error:output.append(pack([error]+[0]*78)+bytes(344))
  else:output.append(pack(words)+pack([len(display)]+percents)+b''.join(names)+pack([resolved]));accepted+=1;lookups+=resolved
 guards=22+((17 if int(info['bases'])else 16)if accepted else 0)
 return b''.join(output)+pack([guards]),dict(queries=counts['queries'],accepted=accepted,completedLookups=lookups,missingLabels=missing,overlongAppends=longnames,guards=guards)
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--owner-labels',type=Path);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/s)for s in SOURCES]+['-o',str(out/'probe')],check=True)
 rows=[b'L'+str(i).encode()for i in range(1109)];blank=rows.copy();blank[339]=b'';blank[338]=b'  F;G , H  ';long=rows.copy();long[339]=b'x'*80;longfungus=rows.copy();longfungus[338]=b'y'*80
 profiles=[('normal',rows,label_file(rows)),('literal-crlf',blank,label_file(blank,True)),('missing',rows[:339],label_file(rows[:339])),('long-hasty',long,label_file(long)),('long-fungus',longfungus,label_file(longfungus))]
 if a.owner_labels:
  b=a.owner_labels.read_bytes();lines=b.splitlines();at=lines.index(b'#LABELS');count=int(lines[at+1]);labels=[s.strip(b' ')for s in lines[at+2:at+2+count]];assert count==1131;profiles.append(('owner',labels,b))
 totals={};digest=hashlib.sha256();worlds=0
 for name,variant,text,data in parent.controls():
  for profile,labels,file in profiles:
   folder=out/(name+'-'+profile);folder.mkdir();(folder/'rules.txt').write_bytes(text);(folder/'world.sc').write_bytes(data);(folder/'labels.txt').write_bytes(file);want,counts=expected(text,data,labels);(folder/'expected.bin').write_bytes(want)
   run=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe'),str(folder/'rules.txt'),str(folder/'world.sc'),str(folder/'labels.txt')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(folder/'actual.bin').write_bytes(run.stdout);(folder/'stderr').write_bytes(run.stderr);assert run.returncode==0,(name,profile,run.returncode,run.stderr[:1200]);assert run.stdout==want,(name,profile,len(run.stdout),len(want),next((i for i,(x,y)in enumerate(zip(run.stdout,want))if x!=y),None));digest.update(run.stdout);worlds+=1
   for k,v in counts.items():totals[k]=totals.get(k,0)+v
 assert totals['accepted']and totals['missingLabels']and totals['overlongAppends']and totals['completedLookups'];r=dict(worldProfiles=worlds,counts=totals,ownerFileSupplied=bool(a.owner_labels),outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
if __name__=='__main__':main()
