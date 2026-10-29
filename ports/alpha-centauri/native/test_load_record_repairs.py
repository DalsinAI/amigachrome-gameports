# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
from test_original_world import valid_fixture
from test_original_game import decode
from test_units import sections
SRC=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','load_record_repairs.cpp','load_record_repairs_probe.cpp']
def rules(mode):
 rows=['#TECHNOLOGY']
 for i in range(89):
  a,b=[('None','None'),('None','Disable'),('Disable','None'),('T0','Disable'),('T1','T2')][(mode+i)%5]
  flags=0 if mode%8==0 else 8 if mode%8==1 and i==88 else 8 if mode%8==2 and i==0 else ((i*53+mode*17)%512)
  rows.append(f'Technology{i},T{i},0,0,0,0,{a},{b},{flags:09b}')
 return ('\n'.join(rows+['#END'])+'\n').encode()
def fixture(version,mode):
 nv=mode%6;nb=(mode//6)%4;b=bytearray(valid_fixture(version,nv,nb));_,o,_=decode(b);old=version<11;pref=o['legacy'] if old else o['preferences'];fs=0x20bc if old else 0x20cc;bs=304 if old else 308;r=random.Random(0x5a9760+version*1000+mode)
 b[pref+0x58:pref+0x5c]=r.randbytes(4)
 for i in range(89):b[pref+(0x164 if old else 0x1e0)+i]=0 if mode%4==0 else 255 if mode%4==1 else r.randrange(256)
 for i in range(8):
  p=o['factions']+i*fs;struct.pack_into('<I',b,p,r.getrandbits(32));sat=0x804 if old else 0x808
  for j in range(4):struct.pack_into('<i',b,p+sat+4*j,0 if mode%3==0 else [-2147483648,-1,0,1,2147483647][(mode+i+j)%5])
 for i in range(24):
  p=o['tiles']+i*44;b[p:p+8]=r.randbytes(8);struct.pack_into('<I',b,p+8,r.getrandbits(32))
 for i in range(nv):
  p=o['vehicles']+i*52;uid=(mode*31+i)%512;xy=[(0,0),(0,0),(1,1),(4,2),(6,4),(0,0)][i]
  struct.pack_into('<hh',b,p,*xy);struct.pack_into('<h',b,p+10,uid);b[p+14]=(mode+i*3)%8;struct.pack_into('<hhh',b,p+46,-1,-1,-1);b[o['prototypes']+uid*52+42]=[0,8,255,8,1][(mode+i)%5]
 for i in range(nb):
  p=o['bases']+i*bs;struct.pack_into('<hhb',b,p,*[(0,0),(0,0),(1,1)][i],(mode+i*5)%8)
 return bytes(b)
def model(text,data):
 info,o,sp=decode(data);old=int(info['version'])<11;pf=sp['legacy'] if old else sp['preferences'];rows=sections(text)['TECHNOLOGY'];keys={r[1].lower():i for i,r in enumerate(rows)};keys.update(none=-1,disable=-2);owners=pf[0x164 if old else 0x1e0:][:89];fs=0x20bc if old else 0x20cc;bs=304 if old else 308;out=bytearray(data);nv=int(info['vehicles']);nb=int(info['bases']);width=int(info['width']);height=int(info['height']);vs=[sp['vehicles'][i*52:(i+1)*52] for i in range(nv)];bases=[sp['bases'][i*bs:(i+1)*bs] for i in range(nb)];alive=pf[0x59]&1
 for f in range(1,8):
  at=o['factions']+f*fs;flags=struct.unpack_from('<I',out,at)[0]&~0x200;sat=struct.unpack_from('<4i',out,at+(0x804 if old else 0x808))
  reveal=any(sat)
  for i,row in enumerate(rows[:88]):
   a,b=keys[row[6].lower()],keys[row[7].lower()]
   if int(row[8],2)&8 and a!=-2 and not(b==-2 and a!=-1) and owners[i]&(1<<f):reveal=True
  if reveal:flags|=0x200
  struct.pack_into('<I',out,at,flags)
  if any(b[4]==f for b in bases) or any(v[14]==f and sp['prototypes'][struct.unpack_from('<h',v,10)[0]*52+42]==8 for v in vs):alive|=1<<f
 # Independent source-order nested scans; first record wins, stale base bit
 # inhibits vehicle faction refresh before base repair, even on a baseless tile.
 for y in range(height):
  for x in range(y&1,width,2):
   at=o['tiles']+(y*(width//2)+x//2)*44;olditems=struct.unpack_from('<I',out,at+8)[0];items=olditems&~2
   for v in vs:
    if struct.unpack_from('<hh',v)==(x,y):
     items|=2
     if not olditems&1:out[at+2]=(out[at+2]&0xf0)|(v[14]&15)
     break
   struct.pack_into('<I',out,at+8,items)
 for y in range(height):
  for x in range(y&1,width,2):
   at=o['tiles']+(y*(width//2)+x//2)*44;items=struct.unpack_from('<I',out,at+8)[0]&~1
   for b in bases:
    if struct.unpack_from('<hh',b)==(x,y):items|=1;out[at+2]=(out[at+2]&0xf0)|(b[4]&15);break
   struct.pack_into('<I',out,at+8,items)
 return bytes([alive])+bytes(out)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--owner',type=Path,action='append',default=[]);ap.add_argument('--owner-rules',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(exist_ok=False,parents=True);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 exe=out/'probe';cmd=[a.cc,*flags,*[str(SRC/n) for n in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True)
 cases=[(f'v{v}-{m}',rules(m),fixture(v,m)) for v in range(7,13) for m in range(48)]
 for p in a.owner:cases.append(('owner-'+p.name,a.owner_rules.read_bytes() if a.owner_rules else rules(9),p.read_bytes()))
 h=hashlib.sha256();records=[];controls=0
 for i,(name,text,data) in enumerate(cases):
  f=out/f'{i}.SC';t=out/f'{i}.txt';f.write_bytes(data);t.write_bytes(text);expected=model(text,data)
  for mode in ([0,1,2,3,4,5,6,7] if i<6 else [0]):
   target=out/f'{i}-{mode}.decoded';r=subprocess.run([*runner,str(exe),str(f),str(t),str(target),str(mode)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30);assert r.returncode==0,(name,mode,r.returncode,r.stdout,r.stderr)
   if mode in [0,5]:actual=target.read_bytes();assert actual==expected,(name,mode,next((j for j,(x,y) in enumerate(zip(actual,expected)) if x!=y),-1));h.update(actual)
   else:assert b'rejected unchanged=1 rc=0' in r.stdout
   controls+=1
  assert f.read_bytes()==data and t.read_bytes()==text
  records.append(dict(name=name,inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(expected).hexdigest()))
 receipt=dict(worlds=len(cases),controls=controls,exports=len(cases)+6,rejections=36,outputSHA256=h.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd,runner=runner,records=records)
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
