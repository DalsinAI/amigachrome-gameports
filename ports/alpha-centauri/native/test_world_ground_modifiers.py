# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os
from test_world_basic_combat import controls as inherited_controls
from test_world_base_combat import RULE_ROWS
from test_units import sections,material,reference
from test_original_game import decode
from test_saved_speed import calculate
from test_ground_modifiers import model,PROFILES
from test_terrain_defense import model as terrain_model
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','terrain_defense.cpp','world_terrain_defense.cpp','ground_modifiers.cpp','world_ground_modifiers.cpp','world_ground_modifiers_probe.cpp']
KEYS=['combat_pct_mobile_open_ground','combat_pct_def_vs_mobile_rough','combat_pct_atk_road','combat_pen_pct_atk_lwr_elev','combat_pct_atk_higher_elev']
def controls():
 for k,(name,v,text,data)in enumerate(inherited_controls()):
  b=bytearray(data);info,o,sp=decode(b);t=sections(text)
  for key,value in zip(KEYS,PROFILES[k%3]):t['RULES'][RULE_ROWS[key]][0]=str(value)
  for i,row in enumerate(t['CHASSIS']):row[8]=str(1+(i+k)%3);row[9]='0'
  for i in range(len(sp['tiles'])//44):
   at=o['tiles']+i*44;b[at]=[0x40,0x60,0x80,0xa0][(i+k)%4];b[at+5]=[0,64,128,255][(i+k//4)%4];items=struct.unpack_from('<I',b,at+8)[0]&~0x200020;items|=[0,0x20,0x200000][(i+k)%3];struct.pack_into('<I',b,at+8,items)
  for actor in range(int(info['vehicles'])):
   uid=struct.unpack_from('<h',b,o['vehicles']+actor*52+10)[0];p=o['prototypes']+uid*52;b[p+36]=(actor+k)%9;struct.pack_into('<I',b,p+32,[0,0x400,0x800000,0x800400][(actor+k)%4])
  yield name,v,material(t),bytes(b)
 name,v,text,data=next(x for x in inherited_controls()if x[1]=='expansion')
 for k,(pre,owned)in enumerate([('None',False),('T0',False),('T0',True)]):
  t=sections(text)
  for row in t['CHASSIS']:row[9]='0'
  t['ABILITIES'][10][2]=pre;b=bytearray(data);info,o,sp=decode(b)
  for uid in range(512):struct.pack_into('<I',b,o['prototypes']+uid*52+32,0)
  owner=o['faction_definitions']+5*0x59c;struct.pack_into('<i',b,owner+1292,1);struct.pack_into('<i',b,owner+1296,15);struct.pack_into('<i',b,owner+1328,10);b[o['preferences']+0x1e0]=(1<<5)if owned else 0
  yield 'antigrav-grant'+str(k),v,material(t),bytes(b)
 for v in['base','expansion']:
  name,_,text,data=next(x for x in inherited_controls()if x[1]==v);t=sections(text)
  for i,row in enumerate(t['CHASSIS']):row[9]=str(i%3)
  yield v+'-nonland-held',v,material(t),data

def expected(text,data):
 info,o,sp=decode(data);variant='expansion'if int(info['version'])>=11 else'base';v=variant=='expansion';tab=reference(text,variant);t=sections(text);n=int(info['vehicles']);width=int(info['width']);bs=308 if v else 304;pf=sp['preferences']if v else sp['legacy'];road=int(t['RULES'][0][0]);keys={row[1].lower():i for i,row in enumerate(t['TECHNOLOGY'])};keys.update(none=-1,disable=-2);profiles=[]
 phidx=struct.unpack_from('<i',pf,0x84+16*4)[0];ph=sp['bases'][phidx*bs+4]if phidx>=0 else -1
 def flags(uid,p):
  raw=struct.unpack_from('<I',p,32)[0]
  if not v or uid<64 or raw&0x800400==0x800400:return raw
  owner=uid//64;fd=sp['faction_definitions'][owner*0x59c:(owner+1)*0x59c];count=struct.unpack_from('<i',fd,1292)[0]
  for j in range(count):
   if struct.unpack_from('<i',fd,1296+j*4)[0]!=15:continue
   a=struct.unpack_from('<i',fd,1328+j*4)[0];bit=1<<a
   if not(bit&0x800400)or raw&bit:continue
   tid=int(tab[f'abilities.{a}.prerequisite']);known=tid==-1
   if 0<=tid<88:
    row=t['TECHNOLOGY'][tid];l,r=keys[row[6].lower()],keys[row[7].lower()];known=l!=-2 and not(r==-2 and l!=-1)and bool(pf[0x1e0+tid]&(1<<owner))
   if known:raw|=bit
  return raw
 for i in range(n):
  a=sp['vehicles'][i*52:(i+1)*52];uid=struct.unpack_from('<h',a,10)[0];p=sp['prototypes'][uid*52:(uid+1)*52];triad=int(tab[f'chassis.{p[36]}.triad']);f=flags(uid,p);speed=calculate([int(v),uid,p[36],p[37],p[39],f,p[42],road,0,8,0],tab)[0];x,y=struct.unpack_from('<hh',a);tile=sp['tiles'][((y*width+x)//2)*44:((y*width+x)//2+1)*44];base=bool(struct.unpack_from('<I',tile,8)[0]&1);stale=base and not any(struct.unpack_from('<hh',sp['bases'],j*bs)==(x,y)for j in range(int(info['bases'])));profiles.append((a,uid,int(tab[f'weapons.{p[37]}.offense']),triad,f,speed,tile,base,stale))
 rows=[]
 for a,uid,w,triad,flags_a,speed_a,ta,base_a,stale_a in profiles:
  for b,oid,ow,ot,flags_b,speed_b,tb,base_b,stale_b in profiles:
   for off,deff in[(864,144),(12345,-6789),(2147483647,-2147483648)]:
    if triad!=0 or ot!=0 or stale_b:rows.append([1,0,0,0]);continue
    tm=terrain_model([tb[0],tb[5],struct.unpack_from('<I',tb,8)[0],tb[2]&15,b[14],oid,ow,ot,1,uid,w,triad,a[14],ph]);assert tm[0]==0
    c=[off,deff,tm[1],tb[5],int(base_b),speed_a,speed_b,road,ta[0],tb[0],int(bool(flags_a&0x400)),int(bool(flags_b&0x400)),*[int(t['RULES'][RULE_ROWS[key]][0])for key in KEYS]];rows.append(model(c)[:4])
 return b''.join(struct.pack('<4I',*(x&0xffffffff for x in row))for row in rows)+struct.pack('<I',17),len(rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for i,(name,variant,text,data) in enumerate(controls()):
  rp=out/f'{i}.txt';gp=out/f'{i}.SC';rp.write_bytes(text);gp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(gp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);wanted,nq=expected(text,data);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert gp.read_bytes()==data and rp.read_bytes()==text;h.update(r.stdout);records.append({'name':name,'queries':nq})
 receipt={'worlds':len(records),'queries':sum(r['queries'] for r in records),'guards':17*len(records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
