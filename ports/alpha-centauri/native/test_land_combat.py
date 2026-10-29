# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os
from test_world_basic_combat import controls as basic_controls,expected as basic_expected
from test_world_base_combat import RULE_ROWS
from test_units import sections,material,reference
from test_original_game import decode
from test_saved_speed import calculate
from test_terrain_defense import model as terrain_model
from test_ground_modifiers import model as ground_model
from test_world_sensor_combat import coverage
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','combat_prototype.cpp','world_combat_prototype.cpp','native_combat_morale.cpp','world_native_combat_morale.cpp','basic_combat.cpp','world_basic_combat.cpp','fungus_combat.cpp','world_fungus_combat.cpp','combat_factions.cpp','world_combat_factions.cpp','terrain_defense.cpp','world_terrain_defense.cpp','ground_modifiers.cpp','world_ground_modifiers.cpp','base_combat.cpp','world_base_combat.cpp','sensor_combat.cpp','world_sensor_combat.cpp','air_port_combat.cpp','world_air_port_combat.cpp','defense_abilities.cpp','world_defense_abilities.cpp','land_combat.cpp','land_combat_probe.cpp']
def controls():
 source=list(basic_controls())
 for version in ['base','expansion']:
  _,_,text,data=next(x for x in source if x[1]==version)
  for k in range(12):
   t=sections(text);b=bytearray(data);info,o,sp=decode(b);bs=308 if version=='expansion'else 304;pf=o['preferences']if version=='expansion'else o['legacy']
   for row in t['CHASSIS']:row[9]='0';row[11]='0'
   for i,row in enumerate(t['CHASSIS']):row[8]=str(1+(i+k)%3)
   for row in t['WEAPONS']:row[2]='3';row[3]='0'
   t['WEAPONS'][0][2]='0'
   for row in t['DEFENSES']:row[2]='2'
   t['DEFENSES'][0][2]='1'
   for key,value in [('combat_pct_mobile_open_ground',25),('combat_pct_def_vs_mobile_rough',30),('combat_pct_atk_road',10),('combat_pen_pct_atk_lwr_elev',15),('combat_pct_atk_higher_elev',20),('combat_pct_base_def',25),('combat_pct_infantry_vs_base',33),('combat_pct_def_range_sensor',25),('combat_pen_pct_non_cbt_def_vs_cbt',50),('combat_pct_com_jam_def_vs_mobl',50),('combat_pen_pct_atk_airdrop',25),('combat_pct_fanatic_atk_bonus',20)]:t['RULES'][RULE_ROWS[key]][0]=str(value if k else 0)
   # Explicit current physical base bits only, no inferred loader fixup.
   for i in range(len(sp['tiles'])//44):
    at=o['tiles']+i*44;b[at]=[64,96,128,160][(i+k)%4];b[at+5]=[0,64,128][(i+k)%3];b[at+2]=(b[at+2]&240)|15;b[at+7]=255;struct.pack_into('<I',b,at+8,[0,32,0x200000,0x800,0x80000000][(i+k)%5])
   for j in range(int(info['bases'])):
    at=o['bases']+j*bs;x,y=struct.unpack_from('<hh',b,at);tile=o['tiles']+((y*int(info['width'])+x)//2)*44;struct.pack_into('<I',b,tile+8,struct.unpack_from('<I',b,tile+8)[0]|1);b[tile+2]=(b[tile+2]&240)|1;b[at+4]=1
    for bit in [2,4,5,35,41]:
     pos=at+0x8c+bit//8;b[pos]=(b[pos]&~(1<<(bit%8)))|((1<<(bit%8))if bit!=2 and(j+k+bit)%3==0 else 0)
   for f in range(8):
    at=o['faction_definitions']+f*0x59c;struct.pack_into('<i',b,at+0x508,0x400 if k%2 else 0);struct.pack_into('<i',b,at+0x50c,4)
    for j,(kind,value)in enumerate([(18,127),(17,73),(18,75),(17,150)]):struct.pack_into('<i',b,at+0x510+j*4,kind);struct.pack_into('<i',b,at+0x530+j*4,value if k<7 else 2147483647)
   for i in range(64 if version=='expansion'else 33):struct.pack_into('<i',b,pf+0x84+4*i,-1)
   if k%2:struct.pack_into('<i',b,pf+0x84+16*4,0)
   if k%3==0:struct.pack_into('<i',b,pf+0x84+5*4,0)
   for i in range(int(info['vehicles'])):
    at=o['vehicles']+i*52;old=struct.unpack_from('<h',b,at+10)[0];uid=320+i;b[o['prototypes']+uid*52:o['prototypes']+(uid+1)*52]=b[o['prototypes']+old*52:o['prototypes']+(old+1)*52];struct.pack_into('<h',b,at+10,uid);b[at+14]=1;struct.pack_into('<I',b,at+4,32 if(i+k)%2 else 0);b[at+36]=3;struct.pack_into('<h',b,at+46,-1);p=o['prototypes']+uid*52;b[p+36]=(i+k)%9;b[p+37]=0 if(i+k)%3==0 else 1;b[p+38]=[0,10,12][(i+k)%3]if version=='expansion'else 0;b[p+39]=1;b[p+42]=0;struct.pack_into('<I',b,p+32,[0,0x200,0x20410,0x20000][(i+k)%4])
   if k==8:
    for row in t['CHASSIS']:row[9]='1'
   if k==9:
    for row in t['CHASSIS']:row[11]='1'
   if k==10:
    for row in t['WEAPONS']:row[2]='-1'
   if k==11:
    for i in range(int(info['vehicles'])):b[o['prototypes']+(320+i)*52+42]=11
   yield version+'-'+str(k),version,material(t),bytes(b)
def expected(text,data):
 info,o,sp=decode(data);n=int(info['vehicles']);tab0=reference(text,'expansion'if int(info['version'])>=11 else'base');held=all(int(tab0[f'chassis.{i}.triad'])!=0 for i in range(9))or all(int(tab0[f'chassis.{i}.missile'])for i in range(9))or all(int(tab0[f'weapons.{i}.offense'])<0 for i in range(21))or all(sp['prototypes'][(320+i)*52+42]==11 for i in range(n))
 if held:return b''.join(struct.pack('<8I',1,*([0]*7))for _ in range(n*n))+struct.pack('<I',17),n*n
 info,o,sp=decode(data);v=int(info['version'])>=11;tab=reference(text,'expansion'if v else'base');ts=sections(text);n=int(info['vehicles']);width=int(info['width']);bs=308 if v else 304;pf=sp['preferences']if v else sp['legacy'];basebytes,_=basic_expected(text,data);basic=[struct.unpack_from('<6i',basebytes,i*24)for i in range(n*n*8)];rows=[]
 rule=lambda key:int(ts['RULES'][RULE_ROWS[key]][0]);word=lambda x:-2**31<=x<2**31
 def trunc(x,d):return abs(x)//d*(-1 if x<0 else 1)
 def mul(x,f):
  if not word(x*f):raise OverflowError
  return x*f
 def scale(x,f):return trunc(mul(x,f),100)
 def project(i):
  loc=struct.unpack_from('<i',pf,0x84+4*i)[0];return sp['bases'][loc*bs+4]if loc>=0 else -1
 profiles=[]
 for i in range(n):
  a=sp['vehicles'][i*52:(i+1)*52];uid=struct.unpack_from('<h',a,10)[0];p=sp['prototypes'][uid*52:(uid+1)*52];x,y=struct.unpack_from('<hh',a);tile=sp['tiles'][((y*width+x)//2)*44:((y*width+x)//2+1)*44];flags=struct.unpack_from('<I',p,32)[0];speed=calculate([int(v),uid,p[36],p[37],p[39],flags,p[42],rule('move_rate_roads'),0,8,0],tab)[0];physical=next((j for j in range(int(info['bases']))if struct.unpack_from('<hh',sp['bases'],j*bs)==(x,y)),-1)if struct.unpack_from('<I',tile,8)[0]&1 else -1;profiles.append((a,uid,p,tile,flags,speed,physical))
 for i,(a,uid,p,ta,af,asp,abase)in enumerate(profiles):
  for j,(b,oid,q,tb,bf,bsp,base)in enumerate(profiles):
   try:
    attack=basic[(i*n+j)*8];defend=basic[(j*n+i)*8]
    if attack[0]or defend[0]:rows.append([1]+[0]*7);continue
    off,deff=attack[1],defend[2];aw=int(tab[f'weapons.{p[37]}.offense']);dw=int(tab[f'weapons.{q[37]}.offense']);armor=int(tab[f'armor.{q[38]}.defense']);items=struct.unpack_from('<I',tb,8)[0]
    if items&32 and tb[0]>=64 and(uid<64 and aw<0 or project(16)==a[14])and dw>=0:
     off+=trunc(off,2)
     if not word(off):raise OverflowError
    if struct.unpack_from('<I',a,4)[0]&32 and af&16 and rule('combat_pen_pct_atk_airdrop'):off=scale(off,100-rule('combat_pen_pct_atk_airdrop'))
    fd=sp['faction_definitions'][a[14]*0x59c:(a[14]+1)*0x59c]
    if struct.unpack_from('<I',fd,0x508)[0]&0x400:off=scale(off,100+rule('combat_pct_fanatic_atk_bonus'))
    if v:
     for k in range(struct.unpack_from('<i',fd,0x50c)[0]):
      kind=struct.unpack_from('<i',fd,0x510+4*k)[0];f=struct.unpack_from('<i',fd,0x530+4*k)[0]
      if kind==18:off=scale(off,f)
     fd=sp['faction_definitions'][b[14]*0x59c:(b[14]+1)*0x59c]
     for k in range(struct.unpack_from('<i',fd,0x50c)[0]):
      if struct.unpack_from('<i',fd,0x510+4*k)[0]==17:deff=scale(deff,struct.unpack_from('<i',fd,0x530+4*k)[0])
    terrain=terrain_model([tb[0],tb[5],items,tb[2]&15,b[14],oid,dw,0,1,uid,aw,0,a[14],project(16)])[1]
    gr=ground_model([off,deff,terrain,tb[5],int(base>=0),asp,bsp,rule('move_rate_roads'),ta[0],tb[0],int(bool(af&0x400)),int(bool(bf&0x400)),*[rule(k)for k in['combat_pct_mobile_open_ground','combat_pct_def_vs_mobile_rough','combat_pct_atk_road','combat_pen_pct_atk_lwr_elev','combat_pct_atk_higher_elev']]])
    if gr[0]:raise OverflowError
    off,deff=gr[1:3];deff=mul(deff,terrain);multi=3 if items&0x800 else 2
    if base>=0:
     fac=lambda k:bool(sp['bases'][base*bs+0x8c+k//8]&(1<<(k%8)));multi=(4 if fac(4)or project(5)==b[14]else 2)+(2 if fac(5)else 0)
     if multi==2 and not af&0x20000:deff=scale(deff,100+rule('combat_pct_base_def'))
     if af&0x20000:multi=min(2,multi)
     if int(tab[f'chassis.{p[36]}.speed'])==1:off=scale(off,100+rule('combat_pct_infantry_vs_base'))
    if af&0x20000:multi=min(2,multi)
    deff=mul(deff,multi);cov=coverage(data,j)
    if cov is None:rows.append([1]+[0]*7);continue
    for bit in[1,2]:
     if cov&bit:deff=scale(deff,100+rule('combat_pct_def_range_sensor'))
    if dw==0 and armor==1:deff=scale(deff,100-rule('combat_pen_pct_non_cbt_def_vs_cbt'))
    mobile=int(tab[f'chassis.{p[36]}.speed'])>1
    if v and q[38]in[10,12]and mobile:deff=scale(deff,125)
    if mobile and bf&0x200 and(not v or not af&0x2000000):deff=scale(deff,100+rule('combat_pct_com_jam_def_vs_mobl'))
    rows.append([0,off,deff,attack[3],defend[4],terrain,multi,cov])
   except OverflowError:rows.append([2]+[0]*7)
 return b''.join(struct.pack('<8I',*(x&0xffffffff for x in row))for row in rows)+struct.pack('<I',17),len(rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x)for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];h=hashlib.sha256();records=[]
 for i,(name,v,text,data)in enumerate(controls()):
  rp=out/f'{i}.txt';gp=out/f'{i}.SC';rp.write_bytes(text);gp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(gp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);wanted,nq=expected(text,data);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,next((j for j,(x,y)in enumerate(zip(r.stdout,wanted))if x!=y),None));assert gp.read_bytes()==data and rp.read_bytes()==text;h.update(r.stdout);records.append({'name':name,'queries':nq})
 rec={'worlds':len(records),'queries':sum(r['queries']for r in records),'guards':17*len(records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n');print({k:v for k,v in rec.items()if k not in['command','records']})
if __name__=='__main__':main()
