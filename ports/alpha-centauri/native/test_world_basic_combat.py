# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os
import test_riot_morale as riot
import test_world_unit_stats as stats
from test_original_game import decode
from test_units import sections,material,reference
from test_world_combat_prototype import PSI
from test_combat_prototype import model as prototype_model
from test_basic_combat import model as basic_model
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','combat_prototype.cpp','world_combat_prototype.cpp','native_combat_morale.cpp','world_native_combat_morale.cpp','basic_combat.cpp','world_basic_combat.cpp','world_basic_combat_probe.cpp']
def ordinary_controls():
 for c,(name,variant,text,data,_) in enumerate(riot.controls()):
  if c%4 not in [0,1]:continue
  t=sections(text);t['WEAPONS'][0][3]='11' if c%5==0 else '0';t['DEFENSES'][0][2]='1';b=bytearray(data);info,o,sp=decode(b);v=int(info['version'])>=11;fs=0x20cc if v else 0x20bc;bs=308 if v else 304;pf=o['preferences'] if v else o['legacy'];width=int(info['width'])
  for f in range(8):
   struct.pack_into('<i',b,o['factions']+fs*f+0x1d8,1 if c%3==0 else 0);struct.pack_into('<i',b,o['factions']+fs*f+0x25c,[0,2,3,4][c%4]);struct.pack_into('<i',b,o['faction_definitions']+f*0x59c+0x4fc,[-150,0,50][c%3])
  if v:
   for f in range(8):struct.pack_into('<i',b,o['tectonic']+4*f,-1 if c%3==1 else 0)
  for i in range(int(info['bases'])):
   p=o['bases']+i*bs;x,y=struct.unpack_from('<hh',b,p);at=o['tiles']+((y*width+x)//2)*44+8;struct.pack_into('<I',b,at,struct.unpack_from('<I',b,at)[0]|1);b[p+0x8c]|=2 # HQ is deliberately present but must not add original morale.
  for i in range(int(info['vehicles'])):
   p=o['vehicles']+i*52;uid=struct.unpack_from('<h',b,p+10)[0];proto=o['prototypes']+52*uid;b[proto+42]=[0,11,12][(c//8)%3];struct.pack_into('<I',b,proto+32,([0,0x1000000,0x2000000,0x3000000][(c//8)%4] if v else 0));b[p+14]=0 if c%3==0 and i%2 else 5
  yield name,variant,material(t),bytes(b)
def controls():
 ordinary=list(ordinary_controls())
 yield from ordinary
 name,variant,text,data=next(r for r in ordinary if r[1]=='expansion' and int(r[0].split('-')[-1])%3==0)
 for k,(ability,prerequisite,owned) in enumerate([(24,'None',False),(25,'None',False),(24,'T0',False),(24,'T0',True)]):
  t=sections(text);t['ABILITIES'][ability][2]=prerequisite;b=bytearray(data);info,o,sp=decode(b)
  for uid in range(512):struct.pack_into('<I',b,o['prototypes']+uid*52+32,0)
  owner=o['faction_definitions']+5*0x59c;struct.pack_into('<i',b,owner+1292,1);struct.pack_into('<i',b,owner+1296,15);struct.pack_into('<i',b,owner+1328,ability)
  b[o['preferences']+0x1e0]=(1<<5) if owned else 0
  yield 'expansion-grant'+str(k),variant,material(t),bytes(b)
def expected(text,data):
 info,o,sp=decode(data);v=int(info['version'])>=11;variant='expansion' if v else 'base';tab=reference(text,variant);ts=sections(text);ratios=[int(ts['RULES'][PSI[k][0]][PSI[k][1]]) for k in ['psi_combat_ratio_atk_land','psi_combat_ratio_atk_sea','psi_combat_ratio_atk_air','psi_combat_ratio_def_land','psi_combat_ratio_def_sea','psi_combat_ratio_def_air']];n=int(info['vehicles']);pf=sp['preferences'] if v else sp['legacy'];fs=0x20cc if v else 0x20bc;bs=308 if v else 304;width=int(info['width']);rows=[]
 def scalar(key):return int(tab[key])
 def faction(f,offset):return struct.unpack_from('<i',sp['factions'],f*fs+offset)[0]
 def project(idx,f):
  loc=struct.unpack_from('<i',pf,0x84+idx*4)[0];return loc>=0 and sp['bases'][loc*bs+4]==f
 def fac(base,idx):return base>=0 and bool(sp['bases'][base*bs+0x8c+idx//8]&(1<<(idx%8)))
 def hastech(tid,owner):
  if owner<=0:return False
  if tid==-1:return True
  if not 0<=tid<88:return False
  keys={row[1].lower():i for i,row in enumerate(ts['TECHNOLOGY'])};keys.update(none=-1,disable=-2);row=ts['TECHNOLOGY'][tid];left,right=keys[row[6].lower()],keys[row[7].lower()]
  return left!=-2 and not(right==-2 and left!=-1) and bool(pf[0x1e0-(0 if v else 0x7c)+tid]&(1<<owner))
 def ability(uid,mask):
  p=sp['prototypes'][uid*52:(uid+1)*52]
  if struct.unpack_from('<I',p,32)[0]&mask:return True
  if not v or uid<64:return False
  owner=uid//64;fd=sp['faction_definitions'][owner*0x59c:(owner+1)*0x59c];count=struct.unpack_from('<i',fd,1292)[0]
  for k in range(count):
   if struct.unpack_from('<i',fd,1296+4*k)[0]!=15:continue
   a=struct.unpack_from('<i',fd,1328+4*k)[0]
   if 1<<a!=mask:continue
   keys={row[1].lower():i for i,row in enumerate(ts['TECHNOLOGY'])};keys.update(none=-1,disable=-2)
   if hastech(keys[ts['ABILITIES'][a][2].lower()],owner):return True
  return False
 for i in range(n):
  a=sp['vehicles'][52*i:52*(i+1)];uid=struct.unpack_from('<h',a,10)[0];p=sp['prototypes'][uid*52:(uid+1)*52];f=a[14];off=scalar(f'weapons.{p[37]}.offense');arm=scalar(f'armor.{p[38]}.defense');x,y=struct.unpack_from('<hh',a);at=((y*width+x)//2)*44;physical=-1
  if struct.unpack_from('<I',sp['tiles'],at+8)[0]&1:
   physical=next((k for k in range(int(info['bases'])) if struct.unpack_from('<hh',sp['bases'],k*bs)==(x,y)),-2)
  for j in range(n):
   b=sp['vehicles'][52*j:52*(j+1)];oid=struct.unpack_from('<h',b,10)[0];op=sp['prototypes'][oid*52:(oid+1)*52];of=b[14]
   if not f:
    turn=int(info['turn']);morale=sum(turn>=t for t in [45,90,170,250,330,330])+int(bool(struct.unpack_from('<I',a,4)[0]&256))+int(uid==10)+((struct.unpack_from('<H',a,8)[0]>>3)&3)
    if of>0:morale+=int(faction(of,0x1d8)!=0)+int(v and struct.unpack_from('<i',sp['tectonic'],4*of)[0]!=0)
    morale=max(0,min(6,morale))
   elif p[42]==11:
    morale=max(2,min(6,max(0,min(3,faction(f,0x298)))+2*int(project(30,f))+sum(bool(int(row[8],2)&2) and hastech(tid,f) for tid,row in enumerate(ts['TECHNOLOGY']))+a[36]))
   else:
    home=struct.unpack_from('<h',a,46)[0];riotflag=home>=0 and bool(struct.unpack_from('<I',sp['bases'],home*bs+48)[0]&2);rule=struct.unpack_from('<i',sp['faction_definitions'],f*0x59c+0x4d8)[0];immune=bool(struct.unpack_from('<I',sp['faction_definitions'],f*0x59c+0x508)[0]&0x8000);morale=riot.model([int(v),faction(f,0x288),rule,a[36],int(home>=0),int(riotflag),int(fac(home,2)),int(fac(home,35)),int(immune),uid,off],True)
   for bits in range(8):
    psi,bombard,mode=bool(bits&1),bool(bits&2),bool(bits&4);c=[int(v),uid,oid,off,scalar(f'weapons.{op[37]}.offense'),arm,scalar(f'armor.{op[38]}.defense'),scalar(f'weapons.{p[37]}.mode'),op[42],scalar(f'chassis.{p[36]}.missile'),scalar(f'chassis.{p[36]}.triad'),scalar(f'chassis.{op[36]}.triad'),int(bombard),*ratios,0,struct.unpack_from('<i',sp['faction_definitions'],f*0x59c+0x4fc)[0],int(project(9,f)),int(project(19,f)),1,int(uid==19)];r=prototype_model(c);assert not r[0];atk,deff=r[1],r[2];fixed=p[42]==12 or p[42]==11 and arm==1 and op[42]!=11
    if psi:
     c[19]=atk;atk=prototype_model(c)[5]
     if not fixed:c[19]=deff;c[23]=0;deff=prototype_model(c)[5]
    ownflags=struct.unpack_from('<I',p,32)[0];oppflags=struct.unpack_from('<I',op,32)[0];c=[int(v),uid,off,arm,p[42],op[42],morale,faction(f,0x288),faction(f,0x25c),int(physical>=0),int(fac(physical,2)),int(fac(physical,35)),int(of==0),int(ability(uid,0x2000000)),int(ability(oid,0x1000000)),int(mode),atk,deff];m=basic_model(c);rows.append(m[:6] if physical!=-2 else [1,0,0,0,0,0])
 return b''.join(struct.pack('<6I',*(x&0xffffffff for x in row)) for row in rows)+struct.pack('<I',11),len(rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for i,(name,variant,text,data) in enumerate(controls()):
  rp=out/f'{i}.txt';gp=out/f'{i}.SC';rp.write_bytes(text);gp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(gp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);wanted,nq=expected(text,data);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert gp.read_bytes()==data and rp.read_bytes()==text;h.update(r.stdout);records.append({'name':name,'queries':nq})
 receipt={'worlds':len(records),'queries':sum(r['queries'] for r in records),'guards':11*len(records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
