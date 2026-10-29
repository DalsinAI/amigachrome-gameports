# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,itertools
import test_world_unit_stats as stats
from test_original_game import decode
from test_units import reference,sections
from test_saved_speed import calculate
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','riot_morale_probe.cpp']
def model(c,check):
 variant,se,rule,rank,home,riot,creche,brood,immune,unit,off=c
 if unit<64 and off<0:return min(rank,6)
 m=max(-4,min(4,se));m+=int(m<=-2)-int(m>=2);m+=min(0,rule)
 if home and creche and m<0:m=-((-m)//2)
 if home and variant and brood and unit<64 and (off<0 or unit==15) and m<0:m=-((-m)//2)
 m+=max(0,rule)
 if immune:m=max(0,m)
 if check and home and riot and not immune:m=max(0,min(6,m-1))
 return max(0,min(6,rank+m))
def controls():
 for variant,version in [('base',7),('expansion',12)]:
  text=stats.rules_fixture(variant)
  options=list(itertools.product([-9,-4,-2,0,2,4,9],[-7,0,7],[0,1]))+[(0,0,0)]*8
  for case,(se,rule,creche) in enumerate(options):
   data=bytearray(stats.world_fixture(version,0));_,o,_=decode(data);fs=0x20cc if version>=11 else 0x20bc;bs=308 if version>=11 else 304;home=case%4!=0 if case>=42 else True;riot=case%4!=1 if case>=42 else True;immune=case%4==2 if case>=42 else False;brood=(case//2)%2==0
   struct.pack_into('<i',data,o['factions']+5*fs+0x288,se);struct.pack_into('<i',data,o['faction_definitions']+5*0x59c+0x4d8,rule);struct.pack_into('<I',data,o['faction_definitions']+5*0x59c+0x508,0x8000 if immune else 0)
   struct.pack_into('<I',data,o['bases']+48,2 if riot else 0);data[o['bases']+0x8c:o['bases']+0x8c+8]=bytes(8);data[o['bases']+0x8c]=4 if creche else 0;data[o['bases']+0x90]=8 if brood else 0
   for uid in [320,15,8]:
    p=o['prototypes']+uid*52;data[p:p+52]=bytes(52);data[p+39]=1;data[p+37]=12 if uid==8 else 0
   facts=[]
   for i,rank in enumerate([0,1,2,3,4,5,6,7,255]):
    unit=[320,15,8][case%3];p=o['vehicles']+52*i;data[p+14]=5;data[p+36]=rank;data[p+16]=0;struct.pack_into('<h',data,p+10,unit);struct.pack_into('<h',data,p+46,0 if home else -1);facts.append([int(version>=11),se,rule,rank,int(home),int(riot),creche,int(brood),int(immune),unit,-1 if unit==8 else 1])
   yield variant+'-'+str(case),variant,text,bytes(data),facts

def expected(text,data,facts):
 info,o,sp=decode(data);variant='base' if int(info['version'])<11 else 'expansion';table=reference(text,variant);road=int(sections(text)['RULES'][0][0]);out=[]
 for i,c in enumerate(facts):
  a=sp['vehicles'][i*52:(i+1)*52];uid=c[9];p=sp['prototypes'][uid*52:(uid+1)*52];m0,m1=model(c,False),model(c,True);speeds=[]
  for skip in [False,True]:_,speed=calculate([c[0],uid,p[36],p[37],p[39],0,p[42],road,0,8*int(skip),m1],table);speeds.append(speed)
  out.append([m0,m1,*speeds])
 return b''.join(struct.pack('<4I',*r) for r in out)+struct.pack('<I',1)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for i,(name,variant,text,data,facts) in enumerate(controls()):
  rp=out/f'{i}.txt';gp=out/f'{i}.SC';rp.write_bytes(text);gp.write_bytes(data);(out/f'{i}.facts.json').write_text(json.dumps(facts)+'\n');r=subprocess.run([*runner,str(exe),str(rp),str(gp)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);wanted=expected(text,data,facts);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert gp.read_bytes()==data and rp.read_bytes()==text;h.update(r.stdout);records.append({'name':name,'actors':len(facts),'moraleQueries':2*len(facts),'speedQueries':2*len(facts)})
 receipt={'worlds':len(records),'actors':sum(r['actors'] for r in records),'moraleQueries':sum(r['moraleQueries'] for r in records),'speedQueries':sum(r['speedQueries'] for r in records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
