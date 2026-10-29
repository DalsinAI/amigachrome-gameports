# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_local_worker_actions as actions
import test_local_workers as workers
from test_load_record_repairs import model as repair
HERE=Path(__file__).resolve().parent
EXTRA=['load_record_repairs.cpp','phase_snapshot.cpp','initialized_snapshot.cpp','resumed_commands.cpp','resumed_commands_probe.cpp']
SOURCES=list(dict.fromkeys(actions.SOURCES[:-1]+EXTRA))
def normalized(text,data):
 alive_projection=repair(text,data);b=bytearray(alive_projection[1:]);info,o,_=workers.e.population.decode(b);version=int(info['version']);pref=o['legacy'] if version<11 else o['preferences'];b[pref+0x59]=alive_projection[0]
 # Parser's explicit wire layout: normalized CurrentFaction390 ->legacy314.
 more=pref+(0x310 if version<11 else 0x38c);struct.pack_into('<I',b,more,struct.unpack_from('<I',b,more)[0]&~0x40);human=struct.unpack_from('<i',b,o['factions']-4)[0];struct.pack_into('<i',b,pref+(0x314 if version<11 else 0x390),human);return bytes(b)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--binary',type=Path);ap.add_argument('--owner',type=Path,action='append',default=[]);ap.add_argument('--owner-rules',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=a.binary.resolve() if a.binary else out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];
 if not a.binary:subprocess.run(cmd,check=True)
 runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for version in range(7,13):
  for case in [1,2,3]:
   text=actions.rules() if version<11 else workers.rules('alien-crossfire');base=actions.fixture(version,case) if version<11 else workers.fixture(version,case)
   if version>=11:
    b=bytearray(base);_,off,_=workers.e.population.decode(b);struct.pack_into('<i',b,off['bases']+0x4c,0);struct.pack_into('<i',b,off['bases']+0x50,-1);base=bytes(b)
   for selector in range(16):
    active=selector%8;restart=selector//8
    data=bytearray(base);_,o,_=workers.e.population.decode(data);pf=o['legacy'] if version<11 else o['preferences'];struct.pack_into('<i',data,pf+(0x314 if version<11 else 0x390),active);struct.pack_into('<I',data,pf+(0x310 if version<11 else 0x38c),(struct.unpack_from('<I',data,pf+(0x310 if version<11 else 0x38c))[0]&~0x40)|(restart*0x40));data=bytes(data);want=normalized(text,data)
    for mode in (['baseline','actions'] if version<11 and active in [0,7] else ['baseline']):
     i=len(records);rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);args=[*runner,str(exe),str(rp),str(sp)]+(['actions'] if mode=='actions' else []);r=subprocess.run(args,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));expected=actions.expected(text,want) if mode=='actions' else want;(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);(out/f'{i}.expected').write_bytes(expected);assert r.returncode==0 and r.stdout[4:]==expected,(version,case,active,mode,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout[4:],expected)) if x!=y),None));assert rp.read_bytes()==text and sp.read_bytes()==data;h.update(r.stdout);records.append(dict(version=version,case=case,savedActive=active,restart=restart,mode=mode,guards=struct.unpack_from('<I',r.stdout)[0]))
 continuations=[]
 for i,record in enumerate(records):
  if record['mode']!='actions':continue
  data=(out/f'{i}.bin').read_bytes()[4:];sp=out/f'{i}-continued.SC';sp.write_bytes(data);text=(out/f'{i}.txt').read_bytes();r=subprocess.run([*runner,str(exe),str(out/f'{i}.txt'),str(sp)],capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));wanted=normalized(text,data);(out/f'{i}-continued.bin').write_bytes(r.stdout);(out/f'{i}-continued.stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout[4:]==wanted,(record,r.returncode,r.stderr);assert sp.read_bytes()==data;h.update(r.stdout);continuations.append(dict(source=i,guards=struct.unpack_from('<I',r.stdout)[0],outputSHA256=hashlib.sha256(r.stdout).hexdigest()))
 for source in a.owner:
  assert a.owner_rules
  text=a.owner_rules.read_bytes();data=source.read_bytes();expected=normalized(text,data);i=len(records);rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(sp)],capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);(out/f'{i}.expected').write_bytes(expected);assert r.returncode==0 and r.stdout[4:]==expected,(source.name,r.returncode,r.stderr);assert source.read_bytes()==data and a.owner_rules.read_bytes()==text;h.update(r.stdout);records.append(dict(owner=source.name,mode='baseline',guards=struct.unpack_from('<I',r.stdout)[0],inputSHA256=hashlib.sha256(data).hexdigest()))
 receipt=dict(worlds=288+len(a.owner),controls=len(records)+len(continuations),repairedCommandReloads=len(continuations),coherentActionRuns=sum(r['mode']=='actions' for r in records),guards=sum(r['guards'] for r in records)+sum(r['guards'] for r in continuations),outputSHA256=h.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd,reusedBinary=bool(a.binary),records=records,continuations=continuations,native='UNRUN');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records','continuations']}))
if __name__=='__main__':main()
