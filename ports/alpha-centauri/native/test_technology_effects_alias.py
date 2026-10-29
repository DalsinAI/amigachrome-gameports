# SPDX-License-Identifier: GPL-3.0-or-later
"""Meaningful input/output range aliases with complete transactional snapshots."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_original_faction import SCHEMA
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','technology_effects.cpp','technology_effects_alias_probe.cpp']
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(HERE/f) for f in SOURCES],'-o',str(a.out/'probe')],check=True)
 results=[];digest=hashlib.sha256()
 for version in [7,12]:
  for actors in [0,3]:
   b=bytearray(valid_fixture(version,actors,2 if actors else 0));_,off,_=decode(b);schema=SCHEMA['Faction' if version>=11 else 'LegacyFactionV10'];economy=next(f['offset'] for f in schema['fields'] if f['name']=='SE_economy_pending');fs=0x20cc if version>=11 else 0x20bc;struct.pack_into('<i',b,off['factions']+fs+economy,0);struct.pack_into('<i',b,off['faction_definitions']+0x59c+0x504,0);struct.pack_into('<i',b,off['faction_definitions']+0x59c+0x50c,0)
   path=a.out/f'world-{version}-{actors}.SC';path.write_bytes(b);p=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(a.out/'probe'),str(path)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));world=21 if not actors else 27;expected=f'ALIASES pure=8 adjacent=4 range=10 world={world} disjoint=1 unchanged=1 RC=0\n'.encode();assert p.returncode==0 and p.stdout==expected,(version,actors,p.returncode,p.stderr,p.stdout,expected);assert path.read_bytes()==bytes(b);digest.update(p.stdout);results.append({'version':version,'actors':actors,'worldAliasRejections':world})
 r={'fixtures':4,'pureAliasRejections':32,'adjacentValidOutputs':16,'rangeBoundaryControls':40,'worldAliasRejections':sum(x['worldAliasRejections'] for x in results),'disjointWorldResults':4,'allInputWorldAndRulesBytesUnchanged':True,'outputSHA256':digest.hexdigest(),'native':'UNRUN'};(a.out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
if __name__=='__main__':main()
