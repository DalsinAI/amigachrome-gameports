# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os
from test_world_basic_combat import controls as inherited_controls
from test_world_base_combat import RULE_ROWS
from test_units import sections,material,reference
from test_original_game import decode
from test_saved_speed import calculate
from test_defense_abilities import model,PROFILES
from test_terrain_defense import model as terrain_model
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','defense_abilities.cpp','world_defense_abilities.cpp','world_defense_abilities_probe.cpp']
KEYS=['combat_pct_com_jam_def_vs_mobl','combat_pct_aaa_bonus_vs_air']
def controls():
 ordinary=list(inherited_controls())
 for k,(name,v,text,data)in enumerate(ordinary):
  b=bytearray(data);info,o,sp=decode(b);t=sections(text)
  for key,value in zip(KEYS,PROFILES[k%3]):t['RULES'][RULE_ROWS[key]][0]=str(value)
  for i,row in enumerate(t['CHASSIS']):row[8]=str([0,1,2,3][(i+k)%4]);row[9]=str(i%3)
  for actor in range(int(info['vehicles'])):
   uid=struct.unpack_from('<h',b,o['vehicles']+actor*52+10)[0];at=o['prototypes']+uid*52;b[at+36]=(actor+k)%9;b[at+38]=[0,10,12,13][(actor+k)%4]if v=='expansion'else(actor+k)%10;struct.pack_into('<I',b,at+32,[0,0x100,0x200,0x300,0x2000000,0x2000300][(actor+k)%6]&(0xffffffff if v=='expansion'else 0xffffff))
  yield name,v,material(t),bytes(b)
 for v in ['base','expansion']:
  name,_,text,data=next(x for x in ordinary if x[1]==v)
  for ability in ([8,9,25]if v=='expansion'else[8,9]):
   for pre,known in [('None',False),('T0',False),('T0',True)]:
    t=sections(text);t['ABILITIES'][ability][2]=pre
    for i,row in enumerate(t['CHASSIS']):row[8]='2';row[9]=str(i%3)
    b=bytearray(data);info,o,sp=decode(b)
    for uid in range(512):struct.pack_into('<I',b,o['prototypes']+uid*52+32,0)
    for f in range(1,8):
     at=o['faction_definitions']+f*0x59c;struct.pack_into('<i',b,at+1292,1);struct.pack_into('<i',b,at+1296,15);struct.pack_into('<i',b,at+1328,ability)
    pf=o['preferences']if v=='expansion'else o['legacy'];b[pf+0x1e0-(0 if v=='expansion'else 0x7c)]=0xfe if known else 0
    for actor in range(int(info['vehicles'])):
     uid=struct.unpack_from('<h',b,o['vehicles']+actor*52+10)[0];b[o['prototypes']+uid*52+36]=actor%9
    yield v+'-grant-'+str(ability)+pre+str(known),v,material(t),bytes(b)

def expected(text,data):
 info,o,sp=decode(data);v=int(info['version'])>=11;variant='expansion'if v else'base';tab=reference(text,variant);t=sections(text);n=int(info['vehicles']);pf=sp['preferences']if v else sp['legacy'];keys={row[1].lower():i for i,row in enumerate(t['TECHNOLOGY'])};keys.update(none=-1,disable=-2)
 def ability(uid,p,mask):
  if struct.unpack_from('<I',p,32)[0]&mask:return True
  if not v or uid<64:return False
  owner=uid//64;fd=sp['faction_definitions'][owner*0x59c:(owner+1)*0x59c];count=struct.unpack_from('<i',fd,1292)[0]
  for j in range(count):
   if struct.unpack_from('<i',fd,1296+j*4)[0]!=15:continue
   a=struct.unpack_from('<i',fd,1328+j*4)[0]
   if 1<<a!=mask:continue
   tid=int(tab[f'abilities.{a}.prerequisite']);known=owner>0 and tid==-1
   if owner>0 and 0<=tid<88:
    row=t['TECHNOLOGY'][tid];left,right=keys[row[6].lower()],keys[row[7].lower()];known=left!=-2 and not(right==-2 and left!=-1)and bool(pf[0x1e0+tid]&(1<<owner))
   if known:return True
  return False
 profiles=[]
 for i in range(n):
  a=sp['vehicles'][i*52:(i+1)*52];uid=struct.unpack_from('<h',a,10)[0];p=sp['prototypes'][uid*52:(uid+1)*52];profiles.append((uid,p))
 rows=[]
 for uid,a in profiles:
  for oid,b in profiles:
   for deff in [576,-6789,2147483647,-2147483648,0]:
    c=[int(v),deff,int(tab[f'chassis.{a[36]}.triad']),int(tab[f'chassis.{a[36]}.speed']),b[38],int(ability(uid,a,0x2000000))if v else 0,int(ability(oid,b,0x200)),int(ability(oid,b,0x100)),*[int(t['RULES'][RULE_ROWS[k]][0])for k in KEYS]];rows.append(model(c)[:3])
 return b''.join(struct.pack('<3I',*(x&0xffffffff for x in row))for row in rows)+struct.pack('<I',17),len(rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for i,(name,variant,text,data) in enumerate(controls()):
  rp=out/f'{i}.txt';gp=out/f'{i}.SC';rp.write_bytes(text);gp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(gp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);wanted,nq=expected(text,data);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert gp.read_bytes()==data and rp.read_bytes()==text;h.update(r.stdout);records.append({'name':name,'queries':nq})
 receipt={'worlds':len(records),'queries':sum(r['queries'] for r in records),'guards':17*len(records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
