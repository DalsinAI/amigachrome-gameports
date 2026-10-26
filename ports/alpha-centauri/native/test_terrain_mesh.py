# SPDX-License-Identifier: GPL-3.0-or-later
"""Source ground mesh coordinates; independent integer oracle, no art or x86."""
import argparse,json,hashlib,os,random,struct,subprocess
from pathlib import Path

def expected(case):
 x,y,*tail=case;heights=tail[:5];zoom=tail[5]
 if zoom not in [1,2]or any(h<0 or h>96 for h in heights):return struct.pack('<I',0)
 quarter=8 if zoom==1 else 16;cx=x+4*quarter;cy=y+2*quarter+sum(heights[1:])//4
 positions=[(cx,cy-heights[0]),(x,cy-heights[1]),(cx,cy-2*quarter-heights[2]),(cx+4*quarter,cy-heights[3]),(cx,cy+2*quarter-heights[4])]
 points=[(positions[c],c)for triangle in [(0,1,2),(0,2,3),(0,3,4),(0,4,1)]for c in triangle]
 if any(v<-(1<<31)or v>=(1<<31)for p,c in points for v in p):return struct.pack('<I',0)
 return struct.pack('<I',1)+b''.join(struct.pack('<2iI',*p,c)for p,c in points)
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'terrain_mesh.cpp'),str(src/'terrain_mesh_probe.cpp'),'-o',str(exe)],check=True)
 rng=random.Random(0x4632d0);cases=[];origins=[(0,0),(-123,45),(321,-87),(-99999,-23456),(999999,456789),(-(1<<31),0),((1<<31)-1,0),(0,-(1<<31))]
 for i in range(128):
  heights=[rng.randrange(97)for k in range(5)]
  for zoom in [1,2]:
   for x,y in origins:cases.append((x,y,*heights,zoom))
 for h in [-1,97]:
  for i in range(5):heights=[0]*5;heights[i]=h;cases.append((0,0,*heights,1))
 cases.extend([(0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,3)])
 data=b''.join(struct.pack('<7iI',*c)for c in cases);want=b''.join(expected(c)for c in cases);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];r=subprocess.run(runner+[str(exe)],input=data,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'input.bin').write_bytes(data);(out/'output.bin').write_bytes(r.stdout);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want
 rbad=subprocess.run(runner+[str(exe)],input=b'partial',capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert rbad.returncode==20
 record=dict(cases=len(cases),accepted=sum(expected(c)[:4]==b'\1\0\0\0'for c in cases),bigEndian=a.m68k_linux,independentIntegerOracle=True,aliasControls=3,partialRecordRejected=True,sha256=hashlib.sha256(want).hexdigest());(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
