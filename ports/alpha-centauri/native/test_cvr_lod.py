# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic chained-geometry controls. No original data or table bytes."""
import argparse,json,os,struct,subprocess,math
from pathlib import Path
from test_cvr_model import fixture,digest

def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def level(units=2,chain=False,remaining=0,count=2,flags=0):
 points=bytes([13<<3,0,5])*count
 span=42+len(points)+5
 return struct.pack('<IIfII9hI',flags|int(chain),remaining or span,units,span,span-1,3,3,max(3,count+1),-1,-1,0,0,0,0,count)+points+b'\0'*5

def chain(units=(2,4,8)):
 parts=[level(u,i<len(units)-1) for i,u in enumerate(units)]
 for i in range(len(parts)):
  p=bytearray(parts[i]);struct.pack_into('<I',p,4,sum(map(len,parts[i:])));parts[i]=bytes(p)
 return b''.join(parts)
def expected(units,count=2):
 g=b'test\0'+struct.pack('<fI',units,count)
 for n in range(count):g+=struct.pack('<4IB',0,0,n+1,0,5)
 return digest(g)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined']
 exe=out/'probe';subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'cvr_model.cpp'),str(src/'cvr_probe.cpp'),'-o',str(exe)],check=True)
 threshold=f32(1.414199948310852);scales=[.0448,.0896,.1,.176,.177,.2,.353,.354,.5,.707,.708,1,2]
 for u in [2,4,8]:
  edge=f32(threshold/u)
  bits=struct.unpack('<I',struct.pack('<f',edge))[0]
  scales += [struct.unpack('<f',struct.pack('<I',bits+d))[0] for d in [-1,0,1]]
 cases=[]
 for scale in scales:
  scale=f32(scale);selected=2
  for current,next_ in [(2,4),(4,8)]:
   if current*scale<threshold and next_*scale<=threshold:selected=next_
   else:break
  cases.append((f'scale-{scale}',fixture(geom=chain(),version=0x41200000),str(scale),expected(selected)))
 bad=[]
 for offset,value in [(0,0),(4,1),(12,50),(16,0),(42,31<<3),(len(level())+8,0)]:
  data=bytearray(chain());struct.pack_into('<I',data,offset,value);bad.append((f'bad-{offset}',bytes(data)))
 bad += [('descending',chain((4,2))),('too-many-levels',chain(tuple(range(1,18)))),('invalid-unselected',chain((2,4))+b'\0')]
 for name,g in bad:cases.append((name,fixture(geom=g),'1',None))
 for scale in ['0','-1','nan','inf']:cases.append(('invalid-scale-'+scale,fixture(geom=chain()),scale,None))
 cases += [('legacy10-alternate-palette',fixture(geom=level(),version=0x41200000,alternate_palette_size=12292),'1',expected(2)),('new10.1-reject-legacy-palette',fixture(geom=level(),alternate_palette_size=12292),'1',None),('legacy10-reject-wrong-palette-size',fixture(geom=level(),version=0x41200000,alternate_palette_size=12289),'1',None)]
 cases.append(('no-stop-single',fixture(geom=level()),'1',expected(2)))
 cases.append(('illegal-stop-in-unflagged',fixture(geom=level(flags=8)),'1',None))
 report=[]
 for i,(name,data,scale,want) in enumerate(cases):
  path=out/f'{i}.cvr';path.write_bytes(data);cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(path),scale]
  r=subprocess.run(cmd,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,text=True);assert r.returncode==(0 if want else 20),(name,r.returncode,r.stdout,r.stderr)
  if want:assert dict(k.split('=')for k in r.stdout.split())['geometry']==want,(name,r.stdout,want)
  report.append(dict(name=name,rc=r.returncode,geometry=want))
 (out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(f'CVR LOD {len(report)} controls PASS')
if __name__=='__main__':main()
