# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os
from test_world_basic_combat import controls as inherited_controls
from test_world_base_combat import RULE_ROWS
from test_units import sections,material,reference
from test_original_game import decode
from test_saved_speed import calculate

from test_terrain_defense import model as terrain_model
HERE=Path(__file__).resolve().parent
from test_world_terrain_defense import controls as terrain_controls
from test_fungus_combat import model
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','fungus_combat.cpp','world_fungus_combat.cpp','world_fungus_combat_probe.cpp']
def controls():
 for k,(name,v,text,data)in enumerate(terrain_controls()):
  t=sections(text)
  for i,row in enumerate(t['WEAPONS']):
   if i in[0,12,13]:row[2]=str(-1 if(i+k)%2 else 3)
  for uid in[15,320]:
   b=bytearray(data);info,o,sp=decode(b);old=struct.unpack_from('<h',b,o['vehicles']+10)[0];b[o['prototypes']+uid*52:o['prototypes']+(uid+1)*52]=b[o['prototypes']+old*52:o['prototypes']+(old+1)*52];struct.pack_into('<h',b,o['vehicles']+10,uid)
   yield name+'-uid'+str(uid),v,material(t),bytes(b)
def expected(text,data):
 info,o,sp=decode(data);v=int(info['version'])>=11;tab=reference(text,'expansion'if v else'base');n=int(info['vehicles']);width=int(info['width']);pf=sp['preferences']if v else sp['legacy'];loc=struct.unpack_from('<i',pf,0x84+16*4)[0];ph=sp['bases'][loc*(308 if v else 304)+4]if loc>=0 else -1;profiles=[]
 for i in range(n):
  a=sp['vehicles'][i*52:(i+1)*52];uid=struct.unpack_from('<h',a,10)[0];p=sp['prototypes'][uid*52:(uid+1)*52];profiles.append((a,uid,p,int(tab[f'weapons.{p[37]}.offense'])))
 rows=[]
 for a,uid,p,weapon in profiles:
  for defender,oid,op,defoff in profiles:
   x,y=struct.unpack_from('<hh',defender);at=((y*width+x)//2)*44;tile=sp['tiles'][at:at+44]
   for off in[864,12345,2147483647]:rows.append(model([off,uid,weapon,defoff,tile[0],struct.unpack_from('<I',tile,8)[0],a[14],ph,p[42]])[:3])
 return b''.join(struct.pack('<3I',*(x&0xffffffff for x in row))for row in rows)+struct.pack('<I',14),len(rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for i,(name,variant,text,data) in enumerate(controls()):
  rp=out/f'{i}.txt';gp=out/f'{i}.SC';rp.write_bytes(text);gp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(gp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);wanted,nq=expected(text,data);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert gp.read_bytes()==data and rp.read_bytes()==text;h.update(r.stdout);records.append({'name':name,'queries':nq})
 receipt={'worlds':len(records),'queries':sum(r['queries'] for r in records),'guards':14*len(records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
