# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');p.add_argument('--original',type=Path);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/'attack_modifier_ledger.cpp'),str(src/'attack_modifier_ledger_probe.cpp'),'-o',str(out/'probe')],check=True)
def s(n):return (n+2**31)%2**32-2**31
def pack(ns):return struct.pack('<'+'I'*len(ns),*(x&0xffffffff for x in ns))
rows=[];wants=[];accepted=lookups=appends=saturated=zero_lookup=originals=0;maximum=0
def add(row,original_display=None):
 global accepted,lookups,appends,saturated,zero_lookup,maximum
 obs,ed,fun,fan,fp,count=row[:6];factors=row[6:14];ground,mp,rp,dp=row[14:18];slots=[row[19+i*2:21+i*2]for i in range(4)]
 e=2 if obs!=1 else 1 if ed>1 or fun>1 or fan>1 or count>8 or ground>31 or ground&1 and not mp or ground&16 and not dp or bool(ground&4)!=(rp!=0)else 0
 if e:want=[e]+[0xa5a5a5a5]*49
 else:
  events=[]
  if fun:events.append([338,50])
  if fan and fp:events.append([528,fp])
  if ed:events.extend([1108,s(x-100)]for x in factors[:count])
  if ground&1:events.append([611,mp])
  if ground&4:events.append([606,rp])
  if ground&16:events.append([330,dp])
  display=[v for v in events if v[1]][:4]
  if original_display is not None:assert display==original_display
  n=0;trace=[]
  for label,percent in events:
   append=int(bool(percent)and n<4);trace.extend([label,percent,append])
   if append:slots[n]=[label,percent];n+=1
   saturated+=int(bool(percent)and not append);zero_lookup+=not percent
  maximum=max(maximum,len(events));accepted+=1;lookups+=len(events);appends+=n
  want=[0,n]+[v for slot in slots for v in slot]+[len(events)]+trace+[0]*(39-len(trace))
 rows.append(pack(row));wants.append(pack(want))
rng=random.Random(543926)
for i in range(8192):
 mp=[0,25,-30,2147483647,-2147483648][i%5];rp=[0,10,-15][i//5%3];dp=[0,20,-25][i//15%3];ground=(1 if mp and i&1 else 0)|(4 if rp else 0)|(16 if dp and i&2 else 0)|(i&10)
 factors=[s(rng.getrandbits(32))if i&4 else[0,75,100,125,200,-25,-2147483648,2147483647][j]for j in range(8)]
 add([1,i&1,int(bool(i&8)),int(bool(i&16)),[-20,0,25][i//45%3],i%9]+factors+[ground,mp,rp,dp,rng.getrandbits(32)]+[rng.getrandbits(32)for _ in range(8)])
normal=[1,1,1,1,20,8]+[100]*8+[21,25,10,20,17]+[0x5a5a5a5a]*8
for index,values in {0:[0,2],1:[2,0xffffffff],2:[2,0xffffffff],3:[2,0xffffffff],5:[9,0xffffffff],14:[32,0xffffffff],15:[0],16:[0],17:[0]}.items():
 for v in values:r=normal.copy();r[index]=v;add(r)
if a.original:
 for ed,profile in enumerate(['base','expansion']):
  folder=a.original/(profile+'-original1');cases=json.loads((folder/'cases.json').read_text());actual=(folder/'actual.bin').read_bytes();assert len(actual)==len(cases)*340
  for i,c in enumerate(cases):
   n=struct.unpack_from('<I',actual,i*340)[0];values=struct.unpack_from('<4i',actual,i*340+4);labels=[]
   for j in range(n):
    txt=actual[i*340+20+j*80:i*340+20+(j+1)*80].split(b'\0')[0];assert txt.startswith(b'L');labels.append([int(txt[1:]),values[j]])
   ground=(1 if c['mobilePercent']and c['roughness']<128 and c['attackerSpeed']>1 and c['defenderSpeed']<=c['attackerSpeed']else 0)|(4 if c['roadPercent']else 0)|(16 if c['downhillPercent']and c['attackerAltitude']>c['defenderAltitude']else 0)
   factors=c['factors'];add([1,ed,c['fungus'],c['fanatic'],c['fanaticPercent'],len(factors)]+factors+[0]*(8-len(factors))+[ground,c['mobilePercent'],c['roadPercent'],c['downhillPercent'],17]+[0x5a5a5a5a]*8,labels);originals+=1
raw=b''.join(rows);wanted=b''.join(wants);(out/'inputs.bin').write_bytes(raw);(out/'expected.bin').write_bytes(wanted);r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe')],input=raw,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'actual.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr[:1500]);assert r.stdout==wanted,(len(r.stdout),len(wanted),next((i for i,(x,y)in enumerate(zip(r.stdout,wanted))if x!=y),None));assert maximum==13 and saturated and zero_lookup
j=dict(cases=len(rows),accepted=accepted,rejected=len(rows)-accepted,lookupObligations=lookups,appends=appends,saturatedNonzeroLookups=saturated,zeroPercentLookups=zero_lookup,maxLookupEvents=maximum,originalJoins=originals,aliasGuards=5*len(rows),outputSHA256=hashlib.sha256(r.stdout).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(j,indent=2)+'\n');print(json.dumps(j))
