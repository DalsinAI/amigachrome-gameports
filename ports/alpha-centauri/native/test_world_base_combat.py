# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,re
from test_world_basic_combat import controls as inherited_controls
from test_original_game import decode
from test_units import reference,sections,material
from test_base_combat import model
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','base_combat.cpp','world_base_combat.cpp','world_base_combat_probe.cpp']
def rule_rows():
 row=-1;out={}
 for line in (HERE/'basic_fields.inc').read_text().splitlines():
  m=re.match(r'RULE_(ROW|ITEM|TECH)\(([^,]+),',line)
  if not m:continue
  if m[1]!='ITEM':row+=1
  out[m[2]]=row
 return out
RULE_ROWS=rule_rows()
def controls():
 for k,(name,v,text,data)in enumerate(inherited_controls()):
  b=bytearray(data);info,o,sp=decode(b);n=int(info['vehicles']);bs=308 if v=='expansion'else 304;pf=o['preferences']if v=='expansion'else o['legacy'];t=sections(text)
  # Named original row ordinals, no saved-accounting substitution.
  for key,value in [('combat_pct_base_def',[-100,0,25,33,100][k%5]),('combat_pct_infantry_vs_base',[-100,0,25,50][k%4])]:
   row=RULE_ROWS[key];t['RULES'][row][0]=str(value)
  for actor in range(n):
   at=o['vehicles']+actor*52;uid=struct.unpack_from('<h',b,at+10)[0];b[o['prototypes']+uid*52+36]=(actor+k)%9
   struct.pack_into('<I',b,o['prototypes']+uid*52+32,0x20000 if (actor+k)%3==0 else 0)
  for base in range(int(info['bases'])):
   at=o['bases']+base*bs;b[at+4]=[0,5,7][(base+k)%3]
   for idx in [4,5,28,29]:
    bit=1<<(idx%8);slot=at+0x8c+idx//8;b[slot]=(b[slot]&~bit)|(bit if (base+k+idx)%4==0 else 0)
  for idx in [5,10,35]:
   if idx==35 and v!='expansion':continue
   struct.pack_into('<i',b,pf+0x84+idx*4,[-1,0,1][(idx+k)%3])
  yield name,v,material(t),bytes(b)
 original=list(inherited_controls());name,v,text,data=next(x for x in original if x[1]=='expansion')
 for k,(pre,owned)in enumerate([('None',False),('T0',False),('T0',True)]):
  t=sections(text);t['ABILITIES'][17][2]=pre;b=bytearray(data);info,o,sp=decode(b)
  for uid in range(512):struct.pack_into('<I',b,o['prototypes']+uid*52+32,0)
  owner=o['faction_definitions']+5*0x59c;struct.pack_into('<i',b,owner+1292,1);struct.pack_into('<i',b,owner+1296,15);struct.pack_into('<i',b,owner+1328,17);b[o['preferences']+0x1e0]=(1<<5)if owned else 0
  yield 'blink-grant'+str(k),v,material(t),bytes(b)
def expected(text,data):
 info,o,sp=decode(data);v=int(info['version'])>=11;tab=reference(text,'expansion'if v else'base');n=int(info['vehicles']);width=int(info['width']);bs=308 if v else 304;pf=sp['preferences']if v else sp['legacy'];rows=[];t=sections(text)
 def rule(key):return int(t['RULES'][RULE_ROWS[key]][0])
 def fac(base,idx):return int(bool(sp['bases'][base*bs+0x8c+idx//8]&(1<<(idx%8))))
 def project(idx,f):
  loc=struct.unpack_from('<i',pf,0x84+idx*4)[0];return int(loc>=0 and sp['bases'][loc*bs+4]==f)
 def ability(uid):
  p=sp['prototypes'][uid*52:(uid+1)*52]
  if struct.unpack_from('<I',p,32)[0]&0x20000:return 1
  if not v or uid<64:return 0
  owner=uid//64;fd=sp['faction_definitions'][owner*0x59c:(owner+1)*0x59c];count=struct.unpack_from('<i',fd,1292)[0];keys={row[1].lower():i for i,row in enumerate(t['TECHNOLOGY'])};keys.update(none=-1,disable=-2)
  for a in range(count):
   if struct.unpack_from('<i',fd,1296+4*a)[0]!=15 or struct.unpack_from('<i',fd,1328+4*a)[0]!=17:continue
   tid=keys[t['ABILITIES'][17][2].lower()]
   if tid==-1:return 1
   if 0<=tid<88:
    row=t['TECHNOLOGY'][tid];l,r=keys[row[6].lower()],keys[row[7].lower()]
    if l!=-2 and not(r==-2 and l!=-1)and pf[0x1e0+tid]&(1<<owner):return 1
  return 0
 for i in range(n):
  a=sp['vehicles'][i*52:(i+1)*52];x,y=struct.unpack_from('<hh',a);tile=((y*width+x)//2)*44;base=-1
  if struct.unpack_from('<I',sp['tiles'],tile+8)[0]&1:base=next((j for j in range(int(info['bases']))if struct.unpack_from('<hh',sp['bases'],j*bs)==(x,y)),-1)
  for j in range(-1,n):
   for off,deff in [(12345,-6789),(0,0),(2147483647,-2147483648),(864,288)]:
    if base<0:rows.append([1,0,0,0,0,0]);continue
    if j<0:profile=[0,0,0,0,0,0];blink=0
    else:
     other=sp['vehicles'][j*52:(j+1)*52];uid=struct.unpack_from('<h',other,10)[0];p=sp['prototypes'][uid*52:(uid+1)*52];profile=[1,uid,int(tab[f'weapons.{p[37]}.offense']),int(tab[f'chassis.{p[36]}.triad']),int(tab[f'chassis.{p[36]}.speed']),other[14]];blink=ability(uid)
    owner=sp['bases'][base*bs+4];c=[int(v),off,deff,*profile,fac(base,4),fac(base,28),fac(base,29),fac(base,5),project(5,a[14]),project(10,owner),project(35,owner)if v else 0,blink,rule('combat_pct_base_def'),rule('combat_pct_infantry_vs_base')];rows.append(model(c)[:6])
 return b''.join(struct.pack('<6I',*(x&0xffffffff for x in r))for r in rows)+struct.pack('<I',17),len(rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for i,(name,variant,text,data) in enumerate(controls()):
  rp=out/f'{i}.txt';gp=out/f'{i}.SC';rp.write_bytes(text);gp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(gp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);wanted,nq=expected(text,data);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert gp.read_bytes()==data and rp.read_bytes()==text;h.update(r.stdout);records.append({'name':name,'queries':nq})
 receipt={'worlds':len(records),'queries':sum(r['queries'] for r in records),'guards':17*len(records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
