# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,re
import test_ready_search as ready
from test_units import sections,material,reference
from test_original_game import decode
from test_combat_prototype import model
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','combat_prototype.cpp','world_combat_prototype.cpp','world_combat_prototype_probe.cpp']
def psi_rows():
 row=-1;col=0;out={}
 for line in (HERE/'basic_fields.inc').read_text().splitlines():
  m=re.match(r'RULE_(ROW|ITEM)\(([^,]+),',line)
  if not m:continue
  if m[1]=='ROW':row+=1;col=0
  else:col+=1
  if m[2].startswith('psi_combat_ratio_'):out[m[2]]=(row,col)
 return out
PSI=psi_rows()
def fixture(version,c):
 text,data,_=ready.fixture(version,c%24);t=sections(text)
 for i in range(3):t['CHASSIS'][i][9]=str((c+i)%3);t['CHASSIS'][i][11]=str((c+i)%2)
 t['WEAPONS'][0][2:4]=[str([-1,0,1,98,99][c%5]),'11' if c%3==0 else '0'];t['WEAPONS'][1][2:4]=[str([-1,2,5][c%3]),'0'];t['DEFENSES'][0][2]=str([-1,0,1,5][c%4]);t['DEFENSES'][1][2]=str([-1,1,3][c%3])
 for key,(r,col) in PSI.items():t['RULES'][r][col]=str({'psi_combat_ratio_atk_land':3,'psi_combat_ratio_atk_sea':5,'psi_combat_ratio_atk_air':7,'psi_combat_ratio_def_land':2,'psi_combat_ratio_def_sea':4,'psi_combat_ratio_def_air':6}[key])
 b=bytearray(data);info,o,sp=decode(b);baseproto=bytes(b[o['prototypes']+64*52:o['prototypes']+65*52])
 for uid,ch,weap,arm,plan in [(64,0,0,0,0),(65,1,1,1,11 if c%2 else 0),(15,2,0,0,0),(19,0,1,1,0)]:
  p=o['prototypes']+uid*52;b[p:p+52]=baseproto;b[p+36:p+39]=bytes([ch,weap,arm]);b[p+42]=plan
 ids=[64,65,15,19] if version>=11 else [64,65,15]
 for i in range(int(info['vehicles'])):p=o['vehicles']+i*52;struct.pack_into('<h',b,p+10,ids[(c+i)%len(ids)]);b[p+14]=(c+i)%3
 for faction in range(8):struct.pack_into('<i',b,o['faction_definitions']+faction*0x59c+0x4fc,[-150,0,50,200][(c+faction)%4])
 pf=o['legacy'] if version<11 else o['preferences'];struct.pack_into('<i',b,pf+0x84+9*4,0 if c%3 else -1);struct.pack_into('<i',b,pf+0x84+19*4,1 if c%4 else -1)
 bs=308 if version>=11 else 304;b[o['bases']+4]=1;b[o['bases']+bs+4]=2
 return material(t),bytes(b)
def expected(text,data):
 info,o,sp=decode(data);v=int(info['version'])>=11;tab=reference(text,'expansion' if v else 'base');ts=sections(text);ratios=[int(ts['RULES'][PSI[k][0]][PSI[k][1]]) for k in ['psi_combat_ratio_atk_land','psi_combat_ratio_atk_sea','psi_combat_ratio_atk_air','psi_combat_ratio_def_land','psi_combat_ratio_def_sea','psi_combat_ratio_def_air']];n=int(info['vehicles']);pf=sp['preferences'] if v else sp['legacy'];bs=308 if v else 304
 def scalar(k):return int(tab[k])
 def row(i,j,bombard):
  a=sp['vehicles'][52*i:52*(i+1)];other=sp['vehicles'][52*j:52*(j+1)];uid=struct.unpack_from('<h',a,10)[0];oid=struct.unpack_from('<h',other,10)[0];p=sp['prototypes'][uid*52:(uid+1)*52];op=sp['prototypes'][oid*52:(oid+1)*52];f=a[14]
  def project(idx):
   loc=struct.unpack_from('<i',pf,0x84+4*idx)[0];return loc>=0 and sp['bases'][loc*bs+4]==f
  c=[int(v),uid,oid,scalar(f'weapons.{p[37]}.offense'),scalar(f'weapons.{op[37]}.offense'),scalar(f'armor.{p[38]}.defense'),scalar(f'armor.{op[38]}.defense'),scalar(f'weapons.{p[37]}.mode'),op[42],scalar(f'chassis.{p[36]}.missile'),scalar(f'chassis.{p[36]}.triad'),scalar(f'chassis.{op[36]}.triad'),bombard,*ratios,0,struct.unpack_from('<i',sp['faction_definitions'],f*0x59c+0x4fc)[0],int(project(9)),int(project(19)),1,int(uid==19)]
  m=model(c);assert not m[0];c[19]=m[1];atk=model(c);c[19]=m[2];c[23]=0;deff=model(c);assert not atk[4] and not deff[4];return [m[1],m[2],m[3],atk[5],deff[5]]
 return b''.join(struct.pack('<5i',*row(i,j,b)) for i in range(n) for j in range(n) for b in range(2))+struct.pack('<I',7),n*n*2

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--private-media',type=Path);ap.add_argument('--private-rules',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[];inputs=[(f'v{v}-{c}',*fixture(v,c)) for v in range(7,13) for c in range(12)]
 if a.private_media:
  assert a.private_rules
  for name in ['Explore.SC','Build.SC','conquer.SC']:inputs.append((name,a.private_rules.read_bytes(),(a.private_media/name).read_bytes()))
 for name,text,data in inputs:
  result,nq=expected(text,data);i=len(records);rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(sp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==result,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,result)) if x!=y),None));assert rp.read_bytes()==text and sp.read_bytes()==data;h.update(r.stdout);records.append({'name':name,'queries':nq})
 receipt={'worlds':len(records),'queries':sum(x['queries'] for x in records),'guards':7*len(records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
