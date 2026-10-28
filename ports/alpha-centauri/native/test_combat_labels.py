# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
p=argparse.ArgumentParser();p.add_argument('--out',required=True,type=Path);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--owner-labels',type=Path);p.add_argument('--world-reporting',type=Path);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/'combat_labels.cpp'),str(src/'combat_labels_probe.cpp'),'-o',str(out/'probe')],check=True)
def pack(ns):return struct.pack('<'+'I'*len(ns),*(x&0xffffffff for x in ns))
def parse(data,ed):
 if not data or ed>1:return 1,None
 pos=0
 def line():
  nonlocal pos
  if pos>=len(data):raise ValueError()
  end=data.find(b'\n',pos);lf=end!=-1
  if not lf:end=len(data)
  s=data[pos:end];pos=end+int(lf)
  if lf and s.endswith(b'\r'):s=s[:-1]
  if b'\0'in s or b'\r'in s or len(s)>(509 if lf else 510):raise ValueError()
  return s.strip(b' ')
 try:
  while True:
   s=line()
   if s==b'#LABELS':break
   if s and not s.startswith(b';'):raise ValueError()
  s=line()
  if not s or any(c<48 or c>57 for c in s)or not 0<int(s)<=2048:raise ValueError()
  return 0,[line()for _ in range(int(s))]
 except ValueError:return 2,None
rows=[];wants=[];imports=resolves=accepted=missing=longnames=zero=saturated=0;lookup_total=0;owner=0
base=[b'L'+str(i).encode()for i in range(1109)]
def file(labels,ending=b'\n'):return b'; checked physical-row controls'+ending+b' #LABELS '+ending+str(len(labels)).encode()+ending+ending.join(labels)+ending+b'# ; ignored original tail'+ending
def add(data,ed=0,events=None,hasty=0,penalty=0,corrupt=0):
 global imports,resolves,accepted,missing,longnames,zero,saturated,lookup_total
 op=int(events is not None);events=events or[];count=0;triples=[]
 for lab,pct in events:
  app=int(bool(pct)and count<4);count+=app;triples.extend([lab,pct,app])
 rows.append(pack([op,ed,len(data),len(events),hasty,penalty,corrupt]+triples+[0]*(39-len(triples)))+data)
 err,labels=parse(data,ed);wanted=pack([err])
 if not err:
  if not op:imports+=1;wanted+=pack([len(labels)])+b''.join(pack([len(s)])+s for s in labels)
  else:
   resolves+=1;count=0;names=[bytearray(b'Z'*80)for _ in range(4)];percent=[0x5a5a5a5a]*4;resolved=0;err=2 if corrupt==4 else 0
   all_events=events+([(339,penalty)]if hasty else[])
   if not err:
    for lab,pct in all_events:
     if lab>=len(labels):err=3;missing+=1;break
     resolved+=1;app=bool(pct)and count<4
     if app:
      s=labels[lab]
      if len(s)>=80:err=4;longnames+=1;break
      names[count][:len(s)+1]=s+b'\0';percent[count]=pct;count+=1
     else:zero+=not pct;saturated+=bool(pct)
   if not err and corrupt in [1,2,3]:err=1
   if not err:accepted+=1;lookup_total+=resolved;body=pack([count]+percent)+b''.join(names)+pack([resolved])
   else:body=pack([0xa5a5a5a5]*5)+b'\xa5'*320+pack([0xa5a5a5a5])
   wanted+=pack([err])+body
 wants.append(wanted)
# Import grammar, physical blank rows, comments and comma literals, tabs and CRLF.
for ed in [0,1]:
 for ending in [b'\n',b'\r\n']:
  for labels in [[b'',b'  a,b; c  ',b'\t tabs \t',b';literal',b'#literal'],base,[b' '*509],[b'x'*509]]:add(file(labels,ending),ed)
 for data in [b'',b'#LABELS\n0\n',b'#LABELS\n2049\n',b'#LABELS\n+1\na\n',b'#LABELS\n1;note\na\n',b'#LABELS\n2\na\n',b'#LABELS\n1\na\r',b'#LABELS\n1\na\0b\n',b'#LABELS\n1\n'+b'x'*510+b'\n',b'wrong\n#LABELS\n1\nx\n',b'#LABELS\n1\n\r\nx\n',b'#LABELS\n1\n'+b'x'*510]:add(data,ed)
add(file([b'x']),2)
rng=random.Random(3391108);canonical=file(base)
for i in range(1024):
 events=[(rng.choice([330,338,528,606,611,1108]),rng.choice([0,50,-25,2147483647,-2147483648]))for _ in range(i%14)]
 add(canonical,i%2,events,i%2,[-100,-33,0,25][i%4])
for size in [0,1,79,80,509]:
 labels=base.copy();labels[339]=b' '*2+b'q'*size+b' '*3
 for events,has,pct in [([],1,-20),([(339,0)],0,0),([(338,50)]*4+[(339,25)],0,0),([(339,25)],0,0)]:add(file(labels),0,events,has,pct)
for events,has,pct in [([(1109,0)],0,0),([(338,50)]*4+[(1109,20)],0,0),([],1,-20)]:add(file(base[:339]),0,events,has,pct)
for bad in [1,2,3,4]:add(canonical,0,[(338,50),(611,0)],1,-10,bad)
if a.owner_labels:
 data=a.owner_labels.read_bytes()
 for ed in [0,1]:add(data,ed);add(data,ed,[(338,50),(528,25),(1108,100),(611,25),(606,10),(330,20)],1,-33);add(data,ed,[],1,-33);owner+=3
worldjoins=0
if a.world_reporting:
 for folder in sorted(a.world_reporting.iterdir()):
  if not folder.is_dir():continue
  data=(folder/'actual.bin').read_bytes();assert len(data)%316==4
  for offset in range(0,len(data)-4,316):
   r=struct.unpack_from('<79I',data,offset)
   if r[0]:continue
   events=[(r[30+3*i],(r[31+3*i]+2**31)%2**32-2**31)for i in range(r[29])]
   add(a.owner_labels.read_bytes()if a.owner_labels else canonical,int(folder.name.startswith('expansion')),events,r[18],(r[16]+2**31)%2**32-2**31);worldjoins+=1
 assert worldjoins==426
raw=b''.join(rows);want=b''.join(wants);run=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe')],input=raw,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'stderr').write_bytes(run.stderr);(out/'actual.bin').write_bytes(run.stdout);(out/'expected.bin').write_bytes(want);assert run.returncode==0,(run.returncode,run.stderr[:2000]);assert run.stdout==want,(len(run.stdout),len(want),next((i for i,(x,y)in enumerate(zip(run.stdout,want))if x!=y),None));assert saturated and zero and missing and longnames and accepted
receipt=dict(cases=len(rows),importSuccess=imports,reportAttempts=resolves,reportSuccess=accepted,lookupCompletions=lookup_total,missingLabelControls=missing,longNameControls=longnames,zeroPercentLookups=zero,saturatedLookups=saturated,ownerFileControls=owner,worldReportingJoins=worldjoins,inputSHA256=hashlib.sha256(raw).hexdigest(),outputSHA256=hashlib.sha256(run.stdout).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
