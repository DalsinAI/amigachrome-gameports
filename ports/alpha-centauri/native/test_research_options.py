# SPDX-License-Identifier: GPL-3.0-or-later
"""Research eligibility: recovered source and independent raw-wire reference."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_original_faction import SCHEMA
from test_units import sections
SRC=Path(__file__).resolve().parent
FILES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','research_options.cpp','research_options_probe.cpp']
def fixture_rules():
 rows=[]
 for i in range(89):
  p=('None','None') if i%4==0 else (f'T{max(0,i-1)}','None')
  p={0:('None','None'),1:('T0','None'),2:('T0','T1'),3:('Disable','None'),4:('None','Disable'),5:('T0','Disable'),6:('T4','None'),7:('T88','None'),87:('Disable','Disable'),88:('T0','T1')}.get(i,p)
  rows.append(f'Technology{i},T{i},0,0,0,0,{p[0]},{p[1]},000000000')
 return ('#TECHNOLOGY\n'+'\n'.join(rows)+'\n#END\n').encode()
def world_fixture(version,case):
 b=bytearray(valid_fixture(version,0,0));_,off,_=decode(b);pref=off['preferences'] if version>=11 else off['legacy'];shift=0 if version>=11 else 0x7c
 owners=bytes(89) if case==0 else bytes([255])*89 if case==1 else bytes([1])*89 if case==2 else bytes([1<<7])*89 if case==3 else bytes(random.Random((case<<8)+i).randrange(256) for i in range(89));b[pref+0x1e0-shift:pref+0x1e0-shift+89]=owners
 schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];size=0x20cc if version>=11 else 0x20bc
 for faction in range(8):
  for name,value in [('tech_research_id',[-1,0,88,89,-2,999][(case+faction)%6]),('tech_accumulated',case*37-100),('tech_cost',case*103-50),('tech_count_transcendent',faction+case)]:
   field=next(f for f in schema['fields'] if f['name']==name);struct.pack_into('<i',b,off['factions']+size*faction+field['offset'],value)
 return bytes(b)
def model(rules,data,expansion):
 _,off,spans=decode(data);expanded='preferences' in spans;pref=spans['preferences'] if expanded else spans['legacy'];owners=pref[(0x1e0 if expanded else 0x164):][:89];rows=sections(rules)['TECHNOLOGY'];keys={row[1].lower():i for i,row in enumerate(rows)};keys.update(none=-1,disable=-2);prereqs=[tuple(keys[x.lower()] for x in row[6:8]) for row in rows]
 version=struct.unpack_from('<i',pref,0x380 if expanded else 0x304)[0];fs=0x20cc if version>=11 else 0x20bc;schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];out=[]
 def known(id,faction):
  if not faction:return False
  if id==-1:return True
  if id<0 or id>=88:return False
  a,b=prereqs[id]
  return a!=-2 and not(b==-2 and a!=-1) and bool(owners[id]&(1<<faction))
 for faction in range(8):
  table=[]
  for id,(a,b) in enumerate(prereqs):
   have=known(id,faction);missing=0
   if have:reason=1
   elif not expansion and 78<=id<=86:reason=3
   elif a==-2 or b==-2:reason=2
   elif faction==0:reason=5
   else:
    missing=int(not known(a,faction))+2*int(not known(b,faction));reason=4 if missing else 0
   table.append((faction,id,int(reason==0),int(have),int(id==88),missing,reason))
  raw=spans['factions'][fs*faction:fs*(faction+1)];values=[]
  for name in ['tech_research_id','tech_accumulated','tech_cost','tech_count_transcendent']:
   f=next(f for f in schema['fields'] if f['name']==name);values.append(struct.unpack_from('<i',raw,f['offset'])[0])
  out.append('FACTION '+' '.join(map(str,[faction,*values,int(-1<=values[0]<89),sum(t[3] for t in table),sum(t[2] for t in table)])))
  out+=['TECH '+' '.join(map(str,t)) for t in table]
 return '\n'.join(out)+'\nRESEARCH queries=712 invalid=7 unchanged=1 RC=0\n'
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--rules-data',nargs=2,action='append',default=[],metavar=('VARIANT','RULES'));ap.add_argument('--original',nargs=3,action='append',default=[],metavar=('VARIANT','RULES','SAVE'));a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 exe=out/'research-probe';subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(SRC/n) for n in FILES],'-o',str(exe)],check=True)
 cases=[];rules=fixture_rules()
 for version in [7,12]:
  for case in range(32):
   for expansion in [False,True]:cases.append((f'v{version}-owners{case}-'+('expansion' if expansion else 'base'),expansion,rules,world_fixture(version,case)))
 for variant,r in a.rules_data:
  assert variant in ['base','expansion'];expansion=variant=='expansion'
  for case in range(32):cases.append(('original-rules-'+variant+'-synthetic-state'+str(case),expansion,Path(r).read_bytes(),world_fixture(12 if expansion else 7,case)))
 for variant,r,s in a.original:
  assert variant in ['base','expansion'];cases.append(('original-'+Path(s).stem+'-'+variant,variant=='expansion',Path(r).read_bytes(),Path(s).read_bytes()))
 digest=hashlib.sha256();receipt=[]
 for name,expansion,rules,data in cases:
  rule_path=out/(name+'.txt');save_path=out/(name+'.sav');rule_path.write_bytes(rules);save_path.write_bytes(data);want=model(rules,data,expansion)
  result=subprocess.run(runner+[str(exe),'expansion' if expansion else 'base',str(rule_path),str(save_path)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert result.returncode==0,(name,result.returncode,result.stdout[-500:],result.stderr);assert result.stdout==want,(name,next(((x,y) for x,y in zip(result.stdout.splitlines(),want.splitlines()) if x!=y),None));digest.update(result.stdout.encode());receipt.append({'case':name,'queries':712,'invalid':7,'rc':0})
 output={'worldContexts':len(cases),'eligibilityQueries':len(cases)*712,'invalidTransactionalControls':len(cases)*7,'sourceAndIndependentMatches':True,'outputSHA256':digest.hexdigest(),'cases':receipt};(out/'receipt.json').write_text(json.dumps(output,indent=2)+'\n');print(json.dumps({k:v for k,v in output.items() if k!='cases'}))
if __name__=='__main__':main()
