# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned original-wire binding for full technology effect recomputation."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_original_faction import SCHEMA
from test_units import sections
from test_technology_effects import model
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','technology_effects.cpp','technology_effects_world_probe.cpp']
def tech_fixture():
 lines=['#TECHNOLOGY']+[f'Technology{i},T{i},0,0,0,0,'+('None,Disable' if i==4 else 'Disable,None' if i==3 else 'None,None')+',000000000' for i in range(89)]+['#END'];rng=random.Random(0x5bae60)
 for i,line in enumerate(lines):
  if line.startswith('Technology'):
   row=line.split(',');row[8]=format(rng.randrange(512),'09b');lines[i]=','.join(row)
 return ('\n'.join(lines)+'\n').encode()
def fixture(version,case):
 b=bytearray(valid_fixture(version,0,0));_,off,_=decode(b);pref=off['preferences'] if version>=11 else off['legacy'];shift=0 if version>=11 else 0x7c;rng=random.Random(case+version*100)
 b[pref+0x1e0-shift:pref+0x1e0-shift+89]=bytes([255 if case%4==0 else 0 if case%4==1 else rng.randrange(256) for _ in range(89)])
 schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];fields={f['name']:f['offset'] for f in schema['fields']};fs=0x20cc if version>=11 else 0x20bc
 for faction in range(8):
  fp=off['factions']+fs*faction;dp=off['faction_definitions']+0x59c*faction
  economy=[-2147483648,-3,0,2,3,4,5,2147483647][(case+faction)%8];struct.pack_into('<i',b,fp+fields['SE_economy_pending'],economy)
  for name in ['tech_commerce_bonus','tech_fungus_nutrient','tech_fungus_mineral','tech_fungus_energy','tech_fungus_unused']:struct.pack_into('<i',b,fp+fields[name],-2147483648 if faction%2 else 2147483647)
  struct.pack_into('<i',b,dp+0x504,2147483647 if case==30 else -faction if case%3 else faction*10);struct.pack_into('<i',b,dp+0x50c,(case+faction)%9)
  for i in range(8):struct.pack_into('<i',b,dp+0x510+i*4,rng.choice([0,7,8,9,15,40]));struct.pack_into('<i',b,dp+0x530+i*4,2147483647 if case==31 else rng.randrange(-30,31))
 return bytes(b)
def facts(text,data):
 info,off,spans=decode(data);version=int(info['version']);pref=spans['preferences'] if version>=11 else spans['legacy'];owners=list(pref[(0x1e0 if version>=11 else 0x164):][:89]);rows=sections(text)['TECHNOLOGY'];keys={r[1].lower():i for i,r in enumerate(rows)};keys.update(none=-1,disable=-2);tech=[[int(r[8],2),keys[r[6].lower()],keys[r[7].lower()],owner] for r,owner in zip(rows,owners)];schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];econ=next(f['offset'] for f in schema['fields'] if f['name']=='SE_economy_pending');fs=0x20cc if version>=11 else 0x20bc;out=[]
 for f in range(8):
  dp=f*0x59c;count=struct.unpack_from('<i',spans['faction_definitions'],dp+0x50c)[0];commerce=struct.unpack_from('<i',spans['faction_definitions'],dp+0x504)[0];economy=struct.unpack_from('<i',spans['factions'],f*fs+econ)[0];bonus=[]
  for i in range(8):bonus += [struct.unpack_from('<i',spans['faction_definitions'],dp+0x510+i*4)[0],struct.unpack_from('<i',spans['faction_definitions'],dp+0x530+i*4)[0]]
  out.append([f,commerce,economy,count,*bonus,*sum(tech,[])])
 return out
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--rules-data',nargs=2,action='append',default=[]);ap.add_argument('--original',nargs=3,action='append',default=[]);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(HERE/n) for n in SOURCES],'-o',str(a.out/'probe')],check=True)
 corpus=[(tech_fixture(),fixture(v,c)) for v in [7,12] for c in range(32)]
 for profile,path in a.rules_data:corpus += [(Path(path).read_bytes(),fixture(7 if profile=='base' else 12,c)) for c in range(32)]
 for profile,rules,save in a.original:corpus.append((Path(rules).read_bytes(),Path(save).read_bytes()))
 digest=hashlib.sha256();safe=[];errors=0
 for i,(text,data) in enumerate(corpus):
  rp=a.out/f'{i}.txt';sp=a.out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);cases=facts(text,data);want=''
  for f,c in enumerate(cases):
   error,result=model(c);errors+=bool(error)
   if not error:safe.append(c)
   else:result=[11,22,33,44,55]
   want+='EFFECTS '+' '.join(map(str,[f,error,*result]))+'\n'
  p=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(a.out/'probe'),str(rp),str(sp)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert p.returncode==0 and p.stdout==want.encode(),(i,p.returncode,p.stderr,p.stdout,want);assert sp.read_bytes()==data;digest.update(p.stdout)
 (a.out/'safe-original-facts.bin').write_bytes(b''.join(struct.pack('<376i',*c) for c in safe));r={'worlds':len(corpus),'factionQueries':len(corpus)*8,'overflowOrInvalidResults':errors,'allIndependentWireValuesMatch':True,'wholeDecodedWorldAndWireUnchanged':True,'safeOriginalFactContexts':len(safe),'outputSHA256':digest.hexdigest()};(a.out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
if __name__=='__main__':main()
