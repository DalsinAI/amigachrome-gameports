# SPDX-License-Identifier: GPL-3.0-or-later
"""Human research pool saved-world bindings vs independent raw-wire reference."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_research_pool import expected,FILES
from test_research_options import fixture_rules,world_fixture,model
from test_original_game import decode
from test_original_faction import SCHEMA
from test_units import sections
SRC=Path(__file__).resolve().parent
def reference(rules,data,expansion):
 info,offset,spans=decode(data);version=int(info['version']);expanded='preferences' in spans;pref=spans['preferences'] if expanded else spans['legacy'];turn=struct.unpack_from('<i',pref,0x44)[0];mask=pref[0x58];blind=bool(struct.unpack_from('<i',pref,0xc)[0]&0x200);schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];size=0x20cc if version>=11 else 0x20bc;field=next(f for f in schema['fields'] if f['name']=='tech_ranking');priorities=[[int(x) for x in row[2:6]] for row in sections(rules)['TECHNOLOGY']]
 eligibility={}
 for line in model(rules,data,expansion).splitlines():
  if line.startswith('TECH '):
   p=list(map(int,line.split()[1:]));eligibility[p[0],p[1]]=p[2]
 output=''
 for faction in range(8):
  rank=struct.unpack_from('<i',spans['factions'],size*faction+field['offset'])[0];case=[faction,rank,turn,int(bool(mask&(1<<faction))),int(blind),[[*priorities[i],eligibility[faction,i]] for i in range(89)]];words=struct.unpack('<181I',expected(case));e,path,count=words[:3];ids=words[3:92];categories=words[92:];output+=f'POOL {faction} {e} {path} {count}'+''.join(f' {ids[i]}:{categories[i]}' for i in range(count))+'\n'
 return output
def fixture(version,case):
 b=bytearray(world_fixture(version,case));_,off,_=decode(b);pref=off['preferences'] if version>=11 else off['legacy'];b[pref+0x58]=[0,255,0x55,0x80][case%4];struct.pack_into('<i',b,pref+0x44,[0,1,12,-1][case%4]);struct.pack_into('<i',b,pref+0xc,0x200 if case&4 else 0);return bytes(b)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',nargs=3,action='append',default=[]);ap.add_argument('--rules-data',nargs=2,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];run=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 files=[f for f in FILES if f!='research_pool_probe.cpp']+['research_pool_world_probe.cpp'];exe=out/'probe';subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(SRC/f) for f in files],'-o',str(exe)],check=True)
 corpus=[(f'v{v}-context{i}',variant,fixture_rules(),fixture(v,i)) for v,variant in [(7,'base'),(12,'expansion')] for i in range(32)]
 for variant,p in a.rules_data:
  for i in range(32):corpus.append(('original-rules-'+variant+'-synthetic'+str(i),variant,Path(p).read_bytes(),fixture(12 if variant=='expansion' else 7,i)))
 for variant,r,p in a.original:corpus.append(('original-'+Path(p).stem,variant,Path(r).read_bytes(),Path(p).read_bytes()))
 digest=hashlib.sha256()
 for name,variant,rules,data in corpus:
  assert variant in ['base','expansion'];rp=out/(name+'.txt');sp=out/(name+'.sav');rp.write_bytes(rules);sp.write_bytes(data);want=reference(rules,data,variant=='expansion');p=subprocess.run(run+[str(exe),variant,str(rp),str(sp)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert p.returncode==0 and p.stdout==want,(name,p.returncode,p.stdout,want,p.stderr);digest.update(p.stdout.encode())
 receipt={'savedWorlds':len(corpus),'factionQueries':len(corpus)*8,'independentWireMatches':True,'outputSHA256':digest.hexdigest()};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
