# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
from test_original_world import valid_fixture
from test_original_game import decode
SRC=Path(__file__).resolve().parent
SOURCES=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','energy_convoy_orders.cpp','energy_convoy_orders_probe.cpp']
def fixture(v,m):
 nv=0 if m==0 else 32;b=bytearray(valid_fixture(v,nv,3));_,o,_=decode(b);bs=304 if v<11 else 308;r=random.Random(0x4f4dc0+v*100+m)
 for i in range(24):
  p=o['tiles']+i*44;struct.pack_into('<I',b,p+8,r.getrandbits(32));b[p+2]=(i*7+m)%256
 for i in range(nv):
  p=o['vehicles']+i*52;uid=64+i;xy=[(0,0),(1,1),(4,2),(6,4)][i%4];struct.pack_into('<hh',b,p,*xy);struct.pack_into('<h',b,p+10,uid);b[p+14]=(i+m)%8;b[p+17]=[0,3,3,3][(i//4)%4];b[p+38]=[0,1,2,255][(i//8)%4];struct.pack_into('<hhh',b,p+46,i%3,-1,-1);b[o['prototypes']+uid*52+42]=[10,0,10,255][(i//2+m)%4]
 for i in range(3):b[o['bases']+i*bs+4]=(m+i)%8
 # Force eligible convoys at home-unrelated and foreign physical-base tiles;
 # all four source guards also fail independently in the remaining records.
 if nv:
  for i in range(8):
   p=o['vehicles']+i*52;struct.pack_into('<hh',b,p,2*(i%4),0);b[p+14]=m%8;b[p+17]=3;b[p+38]=2;b[o['prototypes']+(64+i)*52+42]=10
   tile=o['tiles']+(i%4)*44;struct.pack_into('<I',b,tile+8,(struct.unpack_from('<I',b,tile+8)[0]&~1)|(1 if i%4!=2 else 0));b[tile+2]=(b[tile+2]&0xf0)|([m%8,(m+1)%8,0,8][i%4])
 return bytes(b)
def model(data,surplus,base):
 info,o,sp=decode(data);nb=int(info['bases']);nv=int(info['vehicles']);bs=304 if int(info['version'])<11 else 308
 if base>=nb:return struct.pack('<3I',1,0x31415926,0x31415926)+b'\x5a'*4096
 owner=sp['bases'][base*bs+4];mask=bytearray(4096);count=0
 if surplus<0:
  for i in range(nv):
   v=sp['vehicles'][i*52:(i+1)*52];uid=struct.unpack_from('<h',v,10)[0];x,y=struct.unpack_from('<hh',v)
   if v[14]!=owner or sp['prototypes'][uid*52+42]!=10 or v[17]!=3 or v[38]!=2:continue
   t=sp['tiles'][(y*(int(info['width'])//2)+x//2)*44:]
   if struct.unpack_from('<I',t,8)[0]&1 and t[2]&15<8:mask[i//8]|=1<<(i%8);count+=1
 return struct.pack('<3I',0,nv if surplus<0 else 0,count)+mask

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 exe=out/'probe';cmd=[a.cc,*flags,*[str(SRC/n) for n in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cases=[(f'v{v}-{m}',fixture(v,m)) for v in range(7,13) for m in range(24)]+[(p.name,p.read_bytes()) for p in a.original];h=hashlib.sha256();nq=0;accepted=0;changed=0;guards=0;records=[]
 for i,(name,data) in enumerate(cases):
  info,_,_=decode(data);nb=int(info['bases']);queries=[(energy,b) for energy in [-2147483648,-1,0,1,2147483647] for b in [*range(nb),nb,0xffffffff]];f=out/f'{i}.SC';q=out/f'{i}.queries';f.write_bytes(data);q.write_bytes(b''.join(struct.pack('<iI',*v) for v in queries));r=subprocess.run([*runner,str(exe),'base' if int(info['version'])<11 else 'expansion',str(f),str(q)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr);expected=b''.join(model(data,*v) for v in queries);assert r.stdout==expected,(name,len(r.stdout),len(expected));h.update(r.stdout);guards+=9+int(int(info['vehicles'])>0)+int(nb>0);nq+=len(queries);accepted+=5*nb;changed+=sum(struct.unpack_from('<I',model(data,*v),8)[0] for v in queries if v[1]<nb);records.append(dict(name=name,inputSHA256=hashlib.sha256(data).hexdigest(),queries=len(queries)));assert f.read_bytes()==data
 receipt=dict(worlds=len(cases),queries=nq,accepted=accepted,rejected=nq-accepted,cancellations=changed,guards=guards,outputSHA256=h.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd,runner=runner,records=records);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
