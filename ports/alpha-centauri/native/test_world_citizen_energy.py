#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Current owned faction technology ownership into original specialist facts."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_citizen_definitions import reference as definitions_reference
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','citizens_text.cpp','world_citizen_energy.cpp','world_citizen_energy_probe.cpp']

def material(case):
 tech=[]
 for i in range(89):
  a,b=('Disable','None') if i==3 else ('None','Disable') if i==4 else ('T0','Disable') if i==5 else ('None','None')
  tech.append(f'Technology{i},T{i},0,0,0,0,{a},{b},000000000')
 pairs=[('None','Disable'),('T3','T4'),('T4','T3'),('T5','T88'),('T88','None'),('T0','T1'),('T87','T2')]
 if case>=16:pairs=[(f'T{(case*7+i)%89}',f'T{(case*13+i*3)%89}') for i in range(7)]
 values=[-2147483648,-17,0,1,19,2147483647]
 rows=[f'Citizen{i},Citizens{i},{a},{b},'+','.join(str(values[(case+i+j)%len(values)]) for j in range(3))+',ignored' for i,(a,b) in enumerate(pairs)]
 rows += ['Drone,Drones,','Worker,Workers,','Talent,Talents,']
 return ('#TECHNOLOGY\n'+'\n'.join(tech)+'\n#CITIZENS\n'+'\n'.join(rows)+'\n#END\n').encode()

def owner_offset(data):
 info,off,_=decode(data);v=int(info['version'])
 return off['preferences']+0x1e0 if v>=11 else off['legacy']+0x164

def fixture(version,case):
 data=bytearray(valid_fixture(version,0 if case==31 else 3,0 if case==31 else 2));o=owner_offset(data);rng=random.Random(version*100+case)
 data[o:o+89]=bytes([0 if case==0 else 255 if case==1 else (1<<(case-2)) if 2<=case<=9 else rng.randrange(256) for _ in range(89)])
 return bytes(data)

def reverse_owners(data):
 data=bytearray(data);o=owner_offset(data)
 for i in range(89):data[o+i]^=255
 return bytes(data)

def reference(text,data,profile):
 sections={};name=None
 for line in text.decode('latin1').splitlines():
  line=line.strip()
  if not line or line.startswith(';'):continue
  if line.startswith('#'):name=line[1:].lower();sections[name]=[];continue
  if name:sections[name].append([x.strip() for x in (line.split(';')[0] if name=='technology' else line).split(',')])
 tech=sections['technology'];keys={r[1].lower():i for i,r in enumerate(tech)};keys.update(none=-1,disable=-2)
 prereq=[(keys[r[6].lower()],keys[r[7].lower()]) for r in tech];owners=data[owner_offset(data):][:89]
 definitions=definitions_reference(text,profile);rows=[struct.unpack_from('<5i',definitions,8+i*276+256) for i in range(7)]
 def has(id,faction):
  if faction==0:return False
  if id==-1:return True
  if not 0<=id<88:return False
  a,b=prereq[id]
  return a!=-2 and (b!=-2 or a==-1) and bool(owners[id]&(1<<faction))
 result=bytearray()
 for faction in range(8):
  result+=struct.pack('<II',profile=='expansion',faction)
  for pre,obs,ec,ps,la in rows:result+=struct.pack('<IIiii',has(pre,faction),has(obs,faction),ec,ps,la)
 info,_,_=decode(data);guards=29-int(info['vehicles']=='0')-int(info['bases']=='0')
 result+=struct.pack('<I',guards)
 return bytes(result)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--rules-data',nargs=2,action='append',default=[]);ap.add_argument('--original',nargs=3,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
 exe=out/'world-citizens-probe';command=[a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];subprocess.run(command,check=True)
 corpus=[('base' if v<11 else 'expansion',material(c),fixture(v,c),'synthetic') for v in range(7,13) for c in range(32)]
 for profile,path in a.rules_data:
  before=Path(path).read_bytes();corpus += [(profile,before,fixture(7 if profile=='base' else 12,c),'owner-rules') for c in range(32)]
 for profile,rules,save in a.original:corpus.append((profile,Path(rules).read_bytes(),Path(save).read_bytes(),'owner-world'))
 digest=hashlib.sha256();records=[];totalguards=0
 for i,(profile,text,data,kind) in enumerate(corpus):
  rp=out/f'{i}.txt';sp=out/f'{i}.SC';sp2=out/f'{i}-reload.SC';rp.write_bytes(text);sp.write_bytes(data);flipped=reverse_owners(data);sp2.write_bytes(flipped)
  wanted=reference(text,data,profile)+reference(text,flipped,profile)
  p=subprocess.run([*runner,str(exe),profile,str(rp),str(sp),str(sp2)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30)
  assert p.returncode==0 and p.stdout==wanted,(i,p.returncode,p.stderr,p.stdout[:32],wanted[:32],len(p.stdout),len(wanted))
  assert sp.read_bytes()==data and sp2.read_bytes()==flipped and rp.read_bytes()==text
  digest.update(p.stdout);totalguards+=struct.unpack_from('<I',wanted,1184)[0]+struct.unpack_from('<I',wanted,2372)[0]
  records.append(dict(kind=kind,profile=profile,outputSHA256=hashlib.sha256(p.stdout).hexdigest()))
  # The same supplied definition profile must reject the opposite wire edition.
  if i in [0,128]:
   other=out/f'{i}-wrong.SC';other.write_bytes(fixture(12 if profile=='base' else 7,0));p=subprocess.run([*runner,str(exe),profile,str(rp),str(other),str(other)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30);assert p.returncode==22 and not p.stdout,(p.returncode,p.stderr)
  if i==191:public_digest=digest.hexdigest()
 for profile,path in a.rules_data:assert Path(path).read_bytes()==next(t for pr,t,_,kind in corpus if pr==profile and kind=='owner-rules')
 for profile,rules,save in a.original:assert Path(save).read_bytes()==next(d for pr,t,d,kind in corpus if pr==profile and t==Path(rules).read_bytes() and kind=='owner-world' and d==Path(save).read_bytes())
 receipt=dict(worlds=len(corpus),phases=len(corpus)*2,factionQueries=len(corpus)*16,specialistFacts=len(corpus)*112,invalidAndAliasGuards=totalguards,oppositeEditionRejects=2,publicWorlds=192,publicOutputSHA256=public_digest,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),worldWireAndTablesUnchanged=True,reloadRefreshesOwnership=True,records=records,command=command)
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ('records','command')}))
if __name__=='__main__':main()
