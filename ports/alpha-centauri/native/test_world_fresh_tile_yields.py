# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh owned technology caches -> original tile leaves; independent wire oracle."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_world_tile_yields as snapshot
from test_original_game import decode
from test_original_faction import SCHEMA
from test_units import sections,material
from test_technology_effects_world import facts
from test_technology_effects import model as effects
from test_tile_yields import expected as leaf
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','tile_yields.cpp','world_tile_yields.cpp','technology_effects.cpp','world_fresh_tile_yields.cpp','world_fresh_tile_yields_probe.cpp']
def rules(variant,case):
 t=sections(snapshot.rules(variant,case))
 for i,row in enumerate(t['TECHNOLOGY']):row[8]=format([0,4,0x40,0x80,0x100,0x1c4][(i+case)%6],'09b')
 return material(t)
def fixture(version,case):
 data=bytearray(snapshot.fixture(version,case));_,off,_=decode(data);legacy=version<11;fs=0x20bc if legacy else 0x20cc;fields={f['name']:f['offset'] for f in SCHEMA['LegacyFactionV10' if legacy else 'Faction']['fields']}
 for f in range(8):
  fp=off['factions']+f*fs;dp=off['faction_definitions']+f*0x59c
  for name in ['tech_commerce_bonus','tech_fungus_nutrient','tech_fungus_mineral','tech_fungus_energy','tech_fungus_unused']:struct.pack_into('<i',data,fp+fields[name],-2147483648 if (case+f)%2 else 2147483647)
  struct.pack_into('<i',data,dp+0x504,case-f);struct.pack_into('<i',data,dp+0x50c,8)
  for i in range(8):
   struct.pack_into('<i',data,dp+0x510+i*4,[7,8,9,0][i%4]);struct.pack_into('<i',data,dp+0x530+i*4,(case*7+f+i)%17-8)
 return bytes(data)
def expected(variant,text,data):
 rows=snapshot.expected(variant,text,data);derived=list(map(effects,facts(text,data)))
 for i,row in enumerate(rows):
  if row[0]:continue
  f=(i//28)%10-1;error,values=derived[f]
  if error:rows[i]=[error]+[0x5a5a5a5a]*59+[error]+[0x5a5a5a5a]*9;continue
  row[12:15]=values[1:4];value=leaf(row[1:60]);row[60]=value[0];row[61:70]=value[1:] if value[0]==0 else [0x5a5a5a5a]*9
 return rows
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',nargs=3,action='append',default=[]);ap.add_argument('--rules-data',nargs=2,action='append',default=[]);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'];cmd=[a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(HERE/n) for n in SOURCES],'-o',str(a.out/'probe')];subprocess.run(cmd,check=True)
 corpus=[(f'v{v}-{c}-{variant}',variant,rules(variant,c),fixture(v,c)) for v in range(7,13) for c in range(8) for variant in ['base','expansion']]
 for v in [7,12]:
  for f in range(8):
   for name,offset,value in [('bad-count',0x50c,9),('commerce-overflow',0x504,2147483647),('fungus-overflow',0x530,2147483647)]:
    b=bytearray(fixture(v,7));_,off,_=decode(b);struct.pack_into('<i',b,off['faction_definitions']+f*0x59c+offset,value);corpus.append((f'{name}-{v}-{f}','expansion',rules('expansion',7),bytes(b)))
 for variant,path in a.rules_data:
  for case in range(8):corpus.append((f'original-rules-{variant}-{case}',variant,Path(path).read_bytes(),fixture(7 if variant=='base' else 12,case)))
 for variant,rp,sp in a.original:corpus.append((Path(sp).name,variant,Path(rp).read_bytes(),Path(sp).read_bytes()))
 digest=hashlib.sha256();accepted=errors=changed=queries=0;records=[];runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 for i,(name,variant,text,data) in enumerate(corpus):
  rp=a.out/f'{i}.txt';sp=a.out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);run=subprocess.run([*runner,str(a.out/'probe'),variant,str(rp),str(sp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert run.returncode==0,(name,run.returncode,run.stderr)
  rows=expected(variant,text,data);encoded=b''.join(struct.pack('<70I',*(n&0xffffffff for n in row)) for row in rows);assert run.stdout==encoded,(name,len(run.stdout),len(encoded),next((j for j in range(min(len(encoded),len(run.stdout))) if encoded[j]!=run.stdout[j]),None))
  old=snapshot.expected(variant,text,data);changed+=sum(row!=old[j] for j,row in enumerate(rows));accepted+=sum(row[60]==0 for row in rows);errors+=sum(row[60]!=0 for row in rows);queries+=len(rows);digest.update(encoded);assert rp.read_bytes()==text and sp.read_bytes()==data;records.append({'name':name,'queries':len(rows),'passed':True})
 receipt={'worlds':len(corpus),'queries':queries,'accepted':accepted,'rejected':errors,'differentFromSnapshot':changed,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256((a.out/'probe').read_bytes()).hexdigest(),'allInputsWorldWireRNGUnchanged':True,'normalSDKExecution':'UNRUN','records':records,'command':cmd};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
