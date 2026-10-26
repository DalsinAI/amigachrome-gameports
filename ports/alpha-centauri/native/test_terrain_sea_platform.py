# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,struct,subprocess,os,hashlib,json
from test_sprite_draw import reference

def cases():
 for z in[-5,5]:
  for x,y in[(-17,-13),(-10,-2),(0,0),(17,29),(111,117),(200,200)]:
   for mw,mh in[(0,0),(50,30)]:
    for alt in[0,2,3]:
     for flags,items,late in[(1,0x8000,1),(0x4000401,0x8000,1),(0x21,0x8000,1),(1,0x8000,0),(1,0,1)]:yield(x,y,z,mw,mh,flags,items,alt,late,1,1,0,0,128,128)
def expected(q,src,dst):
 x,y,z,mw,mh,flags,items,alt,late,normal,supplied,*clip=q
 if not late or alt>=3 or not(flags&1)or not(items&0x8000):status=1
 elif flags&0x20:status=5
 else:
  r=reference((z+16,16,72,40,14,4,100,62,x,y,*clip,mw,mh),src,dst);s=struct.unpack_from('<I',r)[0];return struct.pack('<I',{0:0,1:2,2:3}[s])+r[4:]
 return struct.pack('<I8i',status,*([0]*8))+dst

def main():
 a=argparse.ArgumentParser();a.add_argument('--out',required=True);a.add_argument('--cc',default='g++');a.add_argument('--m68k-linux',action='store_true');args=a.parse_args();out=Path(args.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;flags=['-m68040','-static']if args.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-g'];runner=['qemu-m68k','-cpu','m68040']if args.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');bins={}
 for name in['probe','control']:
  exe=out/name;subprocess.run([args.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/f)for f in['sprite_scale.cpp','sprite_pixels.cpp','sprite_draw.cpp','terrain_sea_platform.cpp','terrain_sea_platform_'+name+'.cpp']],'-o',str(exe)],check=True);bins[name]=hashlib.sha256(exe.read_bytes()).hexdigest()
 src=bytes(9 if(x+y)%5==0 else(x*19+y*37)%256 for y in range(40)for x in range(72));dst=bytes((x*11+y*17+199)%256 for y in range(128)for x in range(128));qs=list(cases());payload=b''.join(struct.pack('<15i',*q)+src+dst for q in qs);exp=b''.join(expected(q,src,dst)for q in qs);r=subprocess.run(runner+[str(out/'probe')],input=payload,capture_output=True,env=env);(out/'stderr.txt').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==exp,(len(r.stdout),len(exp),next((i for i,(x,y)in enumerate(zip(r.stdout,exp))if x!=y),None));assert hashlib.sha256(r.stdout).hexdigest()=='04f12d65430f9db0a1af08944fa747fb14c0b65801c2afa3b94dd3ea09d6fda2';control=subprocess.run(runner+[str(out/'control')],capture_output=True,env=env);(out/'control.log').write_bytes(control.stdout+control.stderr);assert control.returncode==0,(control.returncode,control.stdout,control.stderr);rec=dict(cases=len(qs),sha256=hashlib.sha256(r.stdout).hexdigest(),bytes=len(r.stdout),controls=control.stdout.decode().strip(),bigEndian=args.m68k_linux,native=False,binaries=bins);(out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n');print(rec)
if __name__=='__main__':main()
