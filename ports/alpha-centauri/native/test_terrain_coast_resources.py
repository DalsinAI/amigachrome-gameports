# SPDX-License-Identifier: GPL-3.0-or-later
"""Source coast geometry/UVs and explicit edition-bound metadata. No owner data."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path

def cases():
 return [(cx,cy,*hs,z,0,my,s,m)for z in [1,2]for hs in [[0]*5,[5,7,6,3,9],[96,84,95,76,93]]for cx,cy in [(5,7),(-32,-15)]for s in range(4)for m in range(1,8)for my in [0,2]]
def reference(rain,meta,cs):
 classes=meta[:53];rules=meta[53:];mask=bytes(classes.index(rain[(333+y)*640+x+2])if rain[(333+y)*640+x+2]in classes else 0 for y in range(56)for x in range(56));palette=bytes(9 if rain[(344+(j//4)*17)*640+130+j%4*14]in [252,253]else(rain[(344+(j//4)*17)*640+130+j%4*14]+10)%256 for j in range(2,8));out=bytearray(mask+palette)
 for cx,cy,hc,hw,hn,he,hs,z,mx,my,s,m in cs:
  half=16 if z==1 else 32;quarter=half//2;x=cx+half*2;y=cy+(hw+hn+he+hs)//4+half;corners=[(x-2*half,y-hw),(x,y-2*quarter-hn),(x+2*half,y-he),(x,y+2*quarter-hs)];c=corners[s];before=corners[(s-1)%4];after=corners[(s+1)%4];xy=[(x,y-hc),tuple((a+b)//2 for a,b in zip(before,c)),c,tuple((a+b)//2 for a,b in zip(c,after))]
  u,v=(28 if s in [1,2]else 27),(28 if s>=2 else 27);offsets=[(-27,0),(0,-27),(27,0),(0,27)];a,b=offsets[s],offsets[(s+1)%4];uv=[(u,v),(u+a[0],v+a[1]),(u+a[0]+b[0],v+a[1]+b[1]),(u+b[0],v+b[1])];lut=bytearray(palette[t]if t>1 else t for t in rules[m::8]);
  if m==7 and(my-mx)&2:lut[5]=0;lut[14:18]=bytes(4);lut[18:30]=palette[2:6][::-1]*3
  out+=struct.pack('<16i',*[v for p in xy+uv for v in p])+lut
 return out

def run(exe,runner,out,rain,meta):
 (out/'rain.bin').write_bytes(rain);(out/'metadata.bin').write_bytes(meta);cs=cases();packed=struct.pack('<i',len(cs))+b''.join(struct.pack('<12i',*c)for c in cs);env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run(runner+[str(exe),str(out/'rain.bin'),str(out/'metadata.bin')],input=packed,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);expected=reference(rain,meta,cs);assert r.stdout==expected,(len(r.stdout),len(expected),next((i for i,(a,b)in enumerate(zip(r.stdout,expected))if a!=b),None));(out/'output.bin').write_bytes(r.stdout);bad=subprocess.run(runner+[str(exe),str(out/'rain.bin'),str(out/'metadata.bin')],input=struct.pack('<i',1)+b'partial',capture_output=True,env=env);assert bad.returncode==20;return dict(cases=len(cs),maskPixels=3136,paletteSamples=6,geometryCoordinates=len(cs)*16,lookupBytes=len(cs)*53,rejections=16,partialRejected=True,sha256=hashlib.sha256(r.stdout).hexdigest())
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in ['terrain_mesh','terrain_coast_resources','terrain_coast_resources_probe']],'-o',str(exe)],check=True);rain=bytes((x*3+y*5)%53 for y in range(480)for x in range(640));meta=bytes(range(53))+bytes((i*5+k)%6 for i in range(53)for k in range(8));record=run(exe,['qemu-m68k','-cpu','m68040']if a.m68k_linux else[],out,rain,meta);assert record['sha256']=='cee760b1f2b4b67991a38a5d72b3bff8a88838ddd286721f0012dd92bb9b81bc';record['bigEndian']=a.m68k_linux;(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
