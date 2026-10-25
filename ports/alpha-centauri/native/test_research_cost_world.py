# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned saved-world cost bindings against independent wire extraction."""
import argparse,hashlib,json,math,os,struct,subprocess
from pathlib import Path
from test_research_cost import calculate,FILES
from test_original_world import valid_fixture
from test_original_game import decode
from test_original_faction import SCHEMA
from test_units import sections
SRC=Path(__file__).resolve().parent
def scalar(data,schema,name):
 field=next(f for f in schema['fields'] if f['name']==name);return struct.unpack_from('<i',data,field['offset'])[0]
def contexts(rules,data):
 info,offset,spans=decode(data);expanded='preferences' in spans;pref=spans['preferences'] if expanded else spans['legacy'];version=int(info['version']);schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];fs=0x20cc if version>=11 else 0x20bc;fs_data=[spans['factions'][i*fs:(i+1)*fs] for i in range(8)];defs=[spans['faction_definitions'][i*0x59c:(i+1)*0x59c] for i in range(8)]
 table=sections(rules);rate=max(0,min(1000,int(table['RULES'][7][0])));root=math.isqrt(int(info['width'])//2*int(info['height']));turn=struct.unpack_from('<i',pref,0x44)[0];diff=struct.unpack_from('<i',pref,0x34)[0];flags=struct.unpack_from('<i',pref,0xc)[0];human=pref[0x58]
 arr=[[scalar(p,schema,name) for p in fs_data] for name in ['tech_ranking','earned_techs_saved','unk_27','diff_level','SE_research_base']];arr.append([scalar(p,SCHEMA['MFaction'],'rule_techcost') for p in defs]);result=[]
 for faction in range(8):result.append([scalar(fs_data[faction],schema,'tech_cost'),rate,root,turn,diff,flags,human,faction,*sum(arr,[])])
 return result
def fixture(version,case):
 b=bytearray(valid_fixture(version,0,0));_,off,_=decode(b);pref=off['preferences'] if version>=11 else off['legacy'];schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];fs=0x20cc if version>=11 else 0x20bc
 b[pref+0x58]=[0,255,0x55,0x80][case%4]
 for offset,value in [(0x44,case*15),(0x34,case%6),(0xc,0x20 if case&1 else 0)]:struct.pack_into('<i',b,pref+offset,value)
 for faction in range(8):
  for name,value in [('tech_cost',-1 if case%3==0 else case*100+faction),('tech_ranking',faction*100+case),('earned_techs_saved',case*13),('unk_27',case+faction),('diff_level',(case+faction)%6),('SE_research_base',(case+faction)%7-3)]:
   field=next(f for f in schema['fields'] if f['name']==name);struct.pack_into('<i',b,off['factions']+fs*faction+field['offset'],value)
  field=next(f for f in SCHEMA['MFaction']['fields'] if f['name']=='rule_techcost');struct.pack_into('<i',b,off['faction_definitions']+faction*0x59c+field['offset'],[0,50,100,125][faction%4])
 return bytes(b)
def fixture_rules(variant):
 f=json.loads((SRC/'basic_fixture.json').read_text());tech='#TECHNOLOGY\n'+'\n'.join(f'Test{i},T{i},0,1,2,3,None,None,1' for i in range(89))+'\n';rows=[r[:] for r in (f['rules_rows'] if variant=='expansion' else f['rules_rows'][:-2])];rows[7]=['125' if variant=='expansion' else '100']
 return (tech+'#RULES\n'+'\n'.join(','.join(row) for row in rows)+'\n#WORLDBUILDER\n'+'\n'.join(','.join(row) for row in f['world_rows'])+'\n#END\n').encode()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',nargs=3,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];run=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 exe=out/'probe';files=['rules_text.cpp',*[f for f in FILES if f!='research_cost_probe.cpp'],'research_cost_world_probe.cpp'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(SRC/f) for f in files],'-o',str(exe)],check=True)
 corpus=[(f'v{v}-case{case}',variant,fixture_rules(variant),fixture(v,case)) for v,variant in [(7,'base'),(12,'expansion')] for case in range(24)]
 for variant,r,s in a.original:corpus.append(('original-'+Path(s).stem,variant,Path(r).read_bytes(),Path(s).read_bytes()))
 all_contexts=[];digest=hashlib.sha256()
 for name,variant,rules,data in corpus:
  rp=out/(name+'.txt');sp=out/(name+'.sav');rp.write_bytes(rules);sp.write_bytes(data);rows=contexts(rules,data);all_contexts+=rows;want=''
  for faction,row in enumerate(rows):e,cost,flags=calculate(row);want+=f'COST {faction} {e} {cost} {int(bool(flags&1))} {int(bool(flags&2))} root={row[2]}\n'
  p=subprocess.run(run+[str(exe),variant,str(rp),str(sp)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert p.returncode==0 and p.stdout==want,(name,p.returncode,p.stdout,want,p.stderr);digest.update(p.stdout.encode())
 (out/'contexts.json').write_text(json.dumps(all_contexts));receipt={'savedWorlds':len(corpus),'factionCostQueries':len(all_contexts),'mapRootDerivedFromOwnedTiles':True,'independentWireMatches':True,'outputSHA256':digest.hexdigest()};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
