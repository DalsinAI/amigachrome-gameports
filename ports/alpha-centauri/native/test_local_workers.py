#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned refresh and complete byte encoding against independent resource models."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_world_energy as e
import test_local_production_session as prod
from test_facility_definitions import tables
from world_human_allocation_reference import model as allocation_model
HERE=Path(__file__).resolve().parent
SOURCES=list(dict.fromkeys(e.SOURCES[:-1]+prod.SOURCES[:-1]+['local_accounting.cpp','local_accounting_snapshot.cpp','human_worker_allocation.cpp','world_human_allocation.cpp','local_workers.cpp','local_workers_snapshot.cpp','local_workers_probe.cpp']))
def rules(profile):
 _,rows=tables(profile)
 return e.rules(profile)+b'\n#FACILITIES\n'+('\n'.join(','.join(r) for r in rows)+'\n#END\n').encode()
def fixture(v,c):
 b=bytearray(e.fixture(v,c));info,o,sp=e.population.decode(b);pf=o['legacy'] if v<11 else o['preferences'];human=b[o['bases']+4]
 struct.pack_into('<i',b,o['player'],human);b[pf+0x58]=1<<human
 struct.pack_into('<i',b,pf+0x390-(0x7c if v<11 else 0),human)
 # Session admission also qualifies the original ASCII faction filename.
 field=next(x for x in e.population.SCHEMA['MFaction']['fields'] if x['name']=='filename')
 for i in range(8):b[o['faction_definitions']+i*0x59c+field['offset']:o['faction_definitions']+i*0x59c+field['offset']+24]=b'FACTION'+bytes(17)
 pop=b[o['bases']+6]
 if c%4==0:struct.pack_into('<Iii',b,o['bases']+0x78,1,0,0)
 elif c%4==2:struct.pack_into('<Iii',b,o['bases']+0x78,1,pop,0)
 struct.pack_into('<II',b,o['bases']+0x84,0x01234560,0x65432106)
 if c==28:struct.pack_into('<i',b,o['bases']+0x30,0x10000000)
 return bytes(b)
def worker_patch(text,data,profile):
 try:values=allocation_model(profile,text,data,expansion_enabled=profile=='expansion')
 except (ValueError,OverflowError):return None,1
 if values[0]:return None,11 if values[0]==7 else 1 if values[0] in [1,2] else 8
 info,o,_=e.population.decode(data);version=int(info['version']);b=bytearray(data);base=o['bases'];a=values[107:117];intake=values[117:195]
 fields={0x30:intake[11],0x78:a[0],0x7c:a[1],0x80:a[2],0x84:a[6],0x88:a[7],0xb0:intake[6],0xb4:intake[7],0xb8:intake[8],0xbc:0,0xc0:intake[6],0xc4:intake[7],0xc8:intake[8],0xcc:0}
 for offset,value in fields.items():struct.pack_into('<I',b,base+offset-(4 if version<11 and offset>=0x98 else 0),value&0xffffffff)
 return bytes(b),0

def patched(text,data,profile):
 intermediate,error=worker_patch(text,data,profile)
 if error:return None,error
 from test_local_accounting import patched as accounting_patch
 return accounting_patch(text,intermediate,profile)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--no-public',action='store_true');ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--binary',type=Path);ap.add_argument('--original',nargs=3,action='append',default=[]);ap.add_argument('--rules-data',nargs=2,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
 exe=a.binary.resolve() if a.binary else out/'accounting';cmd=[] if a.binary else [a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)]
 if cmd:subprocess.run(cmd,check=True)
 corpus=[(rules('base' if v<11 else 'expansion'),fixture(v,c),'base' if v<11 else 'expansion','synthetic',c) for v in range(7,13) for c in range(1,33)]
 if a.no_public:corpus=[]
 for profile,path in a.rules_data:corpus += [(Path(path).read_bytes(),fixture(7 if profile=='base' else 12,c),profile,'owner-rules',c) for c in range(1,33)]
 for profile,rp,sp in a.original:corpus.append((Path(rp).read_bytes(),Path(sp).read_bytes(),profile,'owner-world',0))
 records=[];digest=hashlib.sha256();guards=accepted=0;public=None
 for i,(text,data,profile,kind,c) in enumerate(corpus):
  rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data)
  expected,error=patched(text,data,profile)
  if c==28:error=8;expected=None
  r=subprocess.run([*runner,str(exe),profile,str(rp),str(sp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.out').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr)
  assert r.returncode==0,(i,r.returncode,r.stderr)
  load=struct.unpack_from('<I',r.stdout)[0];assert load==0,(i,'load',load)
  prep=struct.unpack_from('<I',r.stdout,4)[0];assert prep==error,(i,'prepare',prep,error)
  if error:assert len(r.stdout)==12;g=struct.unpack_from('<I',r.stdout,8)[0]
  else:
   changed,g=struct.unpack_from('<II',r.stdout,8);assert bool(changed)==(expected!=data),(i,'changed');assert r.stdout[16:]==expected,(i,'container',next((j for j,(x,y) in enumerate(zip(r.stdout[16:],expected)) if x!=y),None));accepted+=1
   (out/f'{i}.expected').write_bytes(expected)
  guards+=g;digest.update(r.stdout);assert rp.read_bytes()==text and sp.read_bytes()==data
  records.append(dict(kind=kind,profile=profile,error=error,guards=g,sha256=hashlib.sha256(r.stdout).hexdigest()))
  if not a.no_public and i==191:public=digest.hexdigest()
 receipt=dict(worlds=len(corpus),accepted=accepted,rejected=len(corpus)-accepted,guards=guards,publicOutputSHA256=public,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd,records=records)
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
