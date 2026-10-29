# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os
from test_world_basic_combat import controls as inherited_controls
from test_world_base_combat import RULE_ROWS
from test_units import sections,material,reference
from test_original_game import decode
from test_saved_speed import calculate
from test_artillery_combat import model,PROFILES
from test_terrain_defense import model as terrain_model
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','artillery_combat.cpp','world_artillery_combat.cpp','world_artillery_combat_probe.cpp']
KEYS=['combat_pct_land_gun_vs_ship_art','combat_pct_art_bonus_lvl_alt']
MODES=[2,3,6,10,18,26]
def controls():
 for k,(name,v,text,data)in enumerate(inherited_controls()):
  b=bytearray(data);info,o,sp=decode(b);t=sections(text)
  for key,value in zip(KEYS,PROFILES[k%3]):t['RULES'][RULE_ROWS[key]][0]=str(value)
  for i,row in enumerate(t['CHASSIS']):row[9]=str(i%3)
  for i in range(len(sp['tiles'])//44):b[o['tiles']+i*44]=((i+k)%8)*32+(i*13+k)%32
  for actor in range(int(info['vehicles'])):
   uid=struct.unpack_from('<h',b,o['vehicles']+actor*52+10)[0];b[o['prototypes']+uid*52+36]=(actor+k)%9
  yield name,v,material(t),bytes(b)
def expected(text,data):
 info,o,sp=decode(data);variant='expansion'if int(info['version'])>=11 else'base';tab=reference(text,variant);t=sections(text);n=int(info['vehicles']);width=int(info['width']);profiles=[]
 for i in range(n):
  a=sp['vehicles'][i*52:(i+1)*52];uid=struct.unpack_from('<h',a,10)[0];p=sp['prototypes'][uid*52:(uid+1)*52];x,y=struct.unpack_from('<hh',a);climate=sp['tiles'][((y*width+x)//2)*44];profiles.append((int(tab[f'chassis.{p[36]}.triad']),climate))
 rows=[]
 for at,aa in profiles:
  for dt,da in profiles:
   for mode in MODES:
    for off,deff in[(864,144),(12345,-6789),(2147483647,-2147483648)]:rows.append(model([off,deff,at,dt,aa,da,mode,*[int(t['RULES'][RULE_ROWS[k]][0])for k in KEYS]])[:4])
 return b''.join(struct.pack('<4I',*(x&0xffffffff for x in row))for row in rows)+struct.pack('<I',20),len(rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for i,(name,variant,text,data) in enumerate(controls()):
  rp=out/f'{i}.txt';gp=out/f'{i}.SC';rp.write_bytes(text);gp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(gp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);wanted,nq=expected(text,data);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert gp.read_bytes()==data and rp.read_bytes()==text;h.update(r.stdout);records.append({'name':name,'queries':nq})
 receipt={'worlds':len(records),'queries':sum(r['queries'] for r in records),'guards':20*len(records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
