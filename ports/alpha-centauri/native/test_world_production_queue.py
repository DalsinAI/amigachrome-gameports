#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,struct,subprocess,random
from functools import lru_cache
from pathlib import Path
from test_original_game import decode
from test_world_facility_options import fixture as facility_fixture,model as facility_model
from test_world_unit_availability import fixture as units_fixture,oracle as unit_oracle
from test_world_production_cost import rules
HERE=Path(__file__).resolve().parent
NAMES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','movement.cpp','prototype.cpp','saved_speed.cpp','world_unit_stats.cpp','facility_availability.cpp','world_facility_options.cpp','unit_availability.cpp','world_unit_availability.cpp','queue_shift.cpp','world_production_queue.cpp','world_production_queue_probe.cpp']
S=0x31415926

def fixture(version,case):
 b=bytearray(facility_fixture(version,case%36));info,off,spans=decode(b);u=units_fixture(version,case%4);_,uo,us=decode(u);b[off['prototypes']:off['prototypes']+512*52]=us['prototypes'];old=version<11;bs=304 if old else 308;fs=0x20bc if old else 0x20cc;qo=0xa54 if old else 0xa58;rng=random.Random(version*1000+case)
 for f in range(8):b[off['factions']+f*fs+qo:off['factions']+f*fs+qo+512]=bytes(rng.choice([0,1,127,255]) for _ in range(512))
 for i in range(int(info['bases'])):
  at=off['bases']+i*bs;b[at+4]=1+(case+i)%7;struct.pack_into('<i',b,at+0x4c,case%11-1);last=71 if old else 133
  items=[rng.choice([0,63,64,127,128,319,320,511,-1,-3,-4,-5,-10,-(39 if old else 70),-last]) for _ in range(10)]
  # Independent controls for retry exhaustion, invalid future candidates and
  # queued-unit counter underflow across repeated occurrences.
  if case%8==0:items=[511]*10
  if case%8==1:items=[64]*10
  if case%8==2:items[1]=-2147483648
  if case%8==3:items[1]=512
  if case%8==4:items[0]=512
  struct.pack_into('<10i',b,at+0x50,*items)
 return bytes(b)

def queries(data):
 info,_,_=decode(data);n=int(info['bases']);return [(e,b) for e in range(2) for b in range(n)]+[(2,0),(0,-1),(0,n)]

@lru_cache(maxsize=4)
def facs(text,data,variant,exp):return facility_model(text,data,variant,bool(exp))

def oracle(text,data,variant):
 info,off,spans=decode(data);old=int(info['version'])<11;bs=304 if old else 308;fs=0x20bc if old else 0x20cc;qo=0xa54 if old else 0xa58;n=int(info['bases']);unit=unit_oracle(text,data,variant);slots=72 if old else 134;first=39 if old else 70
 fail=struct.pack('<5I10I',1,S,1,1,S,*([S]*10))+bytes([0x5a])*512+struct.pack('<I',1)
 def model(q):
  exp,base=q
  if exp not in (0,1) or old and exp or not 0<=base<n:return fail
  b=spans['bases'][base*bs:(base+1)*bs];owner=b[4];size=struct.unpack_from('<i',b,0x4c)[0];items=list(struct.unpack_from('<10i',b,0x50));counts=bytearray(spans['factions'][owner*fs+qo:owner*fs+qo+512]);iterations=0;accepted=project=False
  if not 1<=owner<=7 or not 0<=size<=9:return fail
  while size:
   olditem=items[0]
   if olditem>=512:return fail
   if olditem>=0:counts[olditem]=(counts[olditem]-1)&255
   items[:size]=items[1:size+1];size-=1;iterations+=1;item=items[0]
   if item>=0:
    error,available=unit((exp,owner,base,item))
    if error:return fail
   else:
    if item<=-slots:return fail
    result=facs(text,data,variant,exp);ix=((owner-1)*(n+1)+(base+1))*3*158;row=result[ix:ix+158]
    if struct.unpack_from('<i',row,12)[0]:return fail
    available=row[24-item]
   if available:accepted=True;project=item<=-first;break
  return struct.pack('<5I10i',0,iterations,int(accepted),int(project),size,*items)+counts+struct.pack('<I',int(iterations>0))
 return model

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--owner-dir',type=Path);ap.add_argument('--scenario-dir',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 exe=out/'world-production-queue-probe';cmd=[a.cc,*flags,*[str(HERE/n) for n in NAMES],'-o',str(exe)];subprocess.run(cmd,check=True);cases=[]
 for version in range(7,13):
  variant='base' if version<11 else 'expansion'
  for case in range(44):cases.append((f'v{version}-{case}',variant,rules(variant,case),fixture(version,case)))
 if a.owner_dir or a.scenario_dir:
  assert a.owner_dir and a.scenario_dir
  for name in ['Explore.SC','Build.SC','conquer.SC']:cases.append(('original-'+name,'base',(a.owner_dir/'alpha.txt').read_bytes(),(a.scenario_dir/name).read_bytes()))
 digest=hashlib.sha256();records=[];accepted=rejected=available=iterations=project=0
 for name,variant,text,data in cases:
  fp=out/(name+'.SC');fp.write_bytes(data);rp=out/(name+'.txt');rp.write_bytes(text);qs=queries(data);qp=out/(name+'.queries');qp.write_bytes(b''.join(struct.pack('<2i',*q) for q in qs));model=oracle(text,data,variant);expected=[model(q) for q in qs];want=b''.join(expected);r=subprocess.run([*runner,str(exe),variant,str(rp),str(fp),str(qp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr,len(r.stdout));assert r.stdout==want,(name,len(r.stdout),len(want),[(q,struct.unpack_from('<5I',r.stdout,i*576),struct.unpack_from('<5I',want,i*576)) for i,q in enumerate(qs) if r.stdout[i*576:(i+1)*576]!=want[i*576:(i+1)*576]][:2]);assert fp.read_bytes()==data;digest.update(want)
  for x in expected:
   e,it,av,pr=struct.unpack_from('<4I',x);accepted+=e==0;rejected+=e!=0
   if not e:iterations+=it;available+=av;project+=pr
  (out/(name+'.bin')).write_bytes(r.stdout);records.append(dict(name=name,queries=len(qs),inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest()))
 receipt=dict(worlds=len(cases),queries=accepted+rejected,accepted=accepted,rejected=rejected,available=available,iterations=iterations,projectNotificationRequired=project,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),records=records,command=cmd,scope='Read-only complete queue structural retry proposal using real World predicates, independent wire oracle. Full World/tables/RNG/opaque snapshots unchanged, aliases/errors preserve output; no commit/completion/phase/UI.');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ('records','command')}))
if __name__=='__main__':main()
