# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import struct,subprocess,os,hashlib,json,argparse
from test_terrain_sea_late import expected as intent
from test_sprite_draw import reference
TRIMS=[(54,48,26,0),(48,33,28,9),(63,38,16,2),(56,39,21,1),(73,35,18,6),(62,41,12,2)]
SOURCES=['sprite_scale','sprite_pixels','sprite_draw','terrain_sea_late','terrain_water_sprites']
def vectors():
 for z in[-5,5]:
  for items in[0,0x10,0x40,0x400000,0x800000,0x1000000,0x50,0x1c00050,0x8000,0x80000000]:
   for expansion,alt in[(0,1),(1,1),(0,2)]:
    for x,y in[(-12,20),(14,38),(-80,-30)]:
     for mw,mh in[(0,0),(50,30)]:
      for padded in[0,1]:yield(-1,7,0,11,9,z,alt,32,items,0,1,expansion,1,1,x,y,mw,mh,0,0,128,128,1,1,63,padded)
def fixture(q,owner=None):
 images=owner if owner is not None else[bytes(9 if(x+y+i)%7==0 else(x*19+y*37+i*11)%256 for y in range(h)for x in range(w))for i,(w,h,ox,oy)in enumerate(TRIMS)];pitch=256 if q[25]else 128;dst=bytes((x*11+y*17+199)%256 if x<128 else 193 for y in range(128)for x in range(pitch));return images,dst
def expected(q,images,dst):
 p=intent(q[:14]);count=struct.unpack_from('<I',p,4)[0];events=[struct.unpack_from('<2I',p,28+i*8)for i in range(count)];status=2 if any(op==2 for op,v in events)else 3 if any(op<4 for op,v in events)else 0
 if status:return struct.pack('<I',status)+dst
 pitch=256 if q[25]else 128;packed=b''.join(dst[y*pitch:y*pitch+128]for y in range(128))
 for op,v in events:
  k=op-4 if op<8 else 4+v;w,h,ox,oy=TRIMS[k];packed=reference((q[5]+16,16,w,h,ox,oy,100,62,q[14],q[15],*q[18:22],q[16],q[17]),images[k],packed)[36:]
 return struct.pack('<I',0)+b''.join(packed[y*128:(y+1)*128]+dst[y*pitch+128:(y+1)*pitch]for y in range(128))
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;flags=['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-g'];runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');bins={}
 for name in['probe','control']:
  f=out/name;subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/(n+'.cpp'))for n in SOURCES+['terrain_water_sprites_'+name]],'-o',str(f)],check=True);bins[name]=hashlib.sha256(f.read_bytes()).hexdigest()
 cases=list(vectors());payload=bytearray();exp=bytearray()
 for q in cases:
  images,dst=fixture(q);payload+=struct.pack('<26I',*[v&0xffffffff for v in q])+b''.join(images)+dst;exp+=expected(q,images,dst)
 r=subprocess.run(runner+[str(out/'probe')],input=payload,capture_output=True,env=env);(out/'stderr.txt').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==exp,(len(r.stdout),len(exp),next((i for i,(x,y)in enumerate(zip(r.stdout,exp))if x!=y),None));assert hashlib.sha256(r.stdout).hexdigest()=='e0e14986ab1b8575c423b3ad1ae7b4db81eab486494337dd35c981a928e64e6f';c=subprocess.run(runner+[str(out/'control')],capture_output=True,env=env);(out/'control.log').write_bytes(c.stdout+c.stderr);assert c.returncode==0,(c.returncode,c.stdout,c.stderr);rec=dict(cases=len(cases),bytes=len(r.stdout),sha256=hashlib.sha256(r.stdout).hexdigest(),controls=c.stdout.decode().strip(),bigEndian=a.m68k_linux,native=False,binaries=bins);(out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n');print(rec)
if __name__=='__main__':main()
