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
from test_world_psi_abilities import controls as inherited_psi_controls
from test_combat_factions import model
from test_original_faction import SCHEMA
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','combat_factions.cpp','world_combat_factions.cpp','world_combat_factions_probe.cpp']
KEYS=['combat_pen_pct_atk_airdrop','combat_pct_fanatic_atk_bonus','combat_pct_psi_atk_bonus_planet']
def controls():
 for k,(name,v,text,data)in enumerate(inherited_psi_controls()):
  t=sections(text);b=bytearray(data);info,o,sp=decode(b);fs=0x20cc if v=='expansion'else 0x20bc;schema=SCHEMA['Faction'if v=='expansion'else'LegacyFactionV10'];po=next(x['offset']for x in schema['fields']if x['name']=='SE_planet')
  for key,value in zip(KEYS,[(25,20,10),(0,0,0),(-25,-20,-10)][k%3]):t['RULES'][RULE_ROWS[key]][0]=str(value)
  t['ABILITIES'][4][2]=['None','T0','Disable'][k%3]
  for f in range(8):
   at=o['faction_definitions']+f*0x59c;struct.pack_into('<i',b,at+0x508,0x400 if(f+k)%2 else 0);struct.pack_into('<i',b,at+0x50c,5)
   for j,(kind,value)in enumerate([(18,127),(17,73),(15,4),(18,75),(17,150)]):struct.pack_into('<i',b,at+0x510+4*j,kind);struct.pack_into('<i',b,at+0x530+4*j,value)
   struct.pack_into('<i',b,o['factions']+f*fs+po,[-2,0,3][(k+f)%3])
  for actor in range(int(info['vehicles'])):
   at=o['vehicles']+actor*52;state=struct.unpack_from('<I',b,at+4)[0];struct.pack_into('<I',b,at+4,(state&~32)|(32 if(actor+k)%2 else 0));uid=struct.unpack_from('<h',b,at+10)[0];at=o['prototypes']+uid*52+32;flags=struct.unpack_from('<I',b,at)[0];struct.pack_into('<I',b,at,(flags&~16)|(16 if(actor+k)%3 else 0))
  yield name,v,material(t),bytes(b)
def expected(text,data):
 info,o,sp=decode(data);v=int(info['version'])>=11;variant='expansion'if v else'base';tab=reference(text,variant);t=sections(text);n=int(info['vehicles']);pf=sp['preferences']if v else sp['legacy'];fs=0x20cc if v else 0x20bc;po=next(x['offset']for x in SCHEMA['Faction'if v else'LegacyFactionV10']['fields']if x['name']=='SE_planet');keys={row[1].lower():i for i,row in enumerate(t['TECHNOLOGY'])};keys.update(none=-1,disable=-2)
 def ability(uid,p):
  if struct.unpack_from('<I',p,32)[0]&16:return True
  if not v or uid<64:return False
  owner=uid//64;fd=sp['faction_definitions'][owner*0x59c:(owner+1)*0x59c];count=struct.unpack_from('<i',fd,0x50c)[0]
  for j in range(count):
   if struct.unpack_from('<i',fd,0x510+j*4)[0]!=15 or struct.unpack_from('<i',fd,0x530+j*4)[0]!=4:continue
   tid=int(tab['abilities.4.prerequisite']);known=owner>0 and tid==-1
   if owner>0 and 0<=tid<88:
    row=t['TECHNOLOGY'][tid];left,right=keys[row[6].lower()],keys[row[7].lower()];known=left!=-2 and not(right==-2 and left!=-1)and bool(pf[0x1e0+tid]&(1<<owner))
   if known:return True
  return False
 profiles=[]
 for i in range(n):
  a=sp['vehicles'][i*52:(i+1)*52];uid=struct.unpack_from('<h',a,10)[0];p=sp['prototypes'][uid*52:(uid+1)*52];profiles.append((uid,p,a))
 rows=[]
 for uid,a,actor in profiles:
  for oid,b,defender in profiles:
   missile=int(tab[f'chassis.{a[36]}.missile']);psi=0
   if not missile or(v and(uid==15 or oid==15)):psi=int(int(tab[f'weapons.{a[37]}.offense'])<0)|2*int(int(tab[f'armor.{b[38]}.defense'])<0)
   af=sp['faction_definitions'][actor[14]*0x59c:(actor[14]+1)*0x59c];bf=sp['faction_definitions'][defender[14]*0x59c:(defender[14]+1)*0x59c]
   factors=lambda f,kind:[struct.unpack_from('<i',f,0x530+j*4)[0]for j in range(struct.unpack_from('<i',f,0x50c)[0])if struct.unpack_from('<i',f,0x510+j*4)[0]==kind]
   offFactors=factors(af,18)if v else[];defFactors=factors(bf,17)if v else[]
   for off,deff in[(864,144),(12345,-6789),(2147483647,-2147483648)]:
    c=[int(v),off,deff,psi,a[42],int(bool(struct.unpack_from('<I',actor,4)[0]&32)),int(ability(uid,a)),int(bool(struct.unpack_from('<i',af,0x508)[0]&0x400)),actor[14],struct.unpack_from('<i',sp['factions'],actor[14]*fs+po)[0],*[int(t['RULES'][RULE_ROWS[k]][0])for k in KEYS],len(offFactors),len(defFactors),*offFactors,*([0]*(8-len(offFactors))),*defFactors,*([0]*(8-len(defFactors)))];rows.append(model(c)[:4])
 return b''.join(struct.pack('<4I',*(x&0xffffffff for x in row))for row in rows)+struct.pack('<I',17),len(rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for i,(name,variant,text,data) in enumerate(controls()):
  rp=out/f'{i}.txt';gp=out/f'{i}.SC';rp.write_bytes(text);gp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(gp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);wanted,nq=expected(text,data);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert gp.read_bytes()==data and rp.read_bytes()==text;h.update(r.stdout);records.append({'name':name,'queries':nq})
 receipt={'worlds':len(records),'queries':sum(r['queries'] for r in records),'guards':17*len(records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
