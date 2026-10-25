# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic owned CVR controls, with explicit coordinate/animation expectations."""
import argparse,os,struct,subprocess,json
from pathlib import Path

def chunk(tag,data):return struct.pack('<II',tag,len(data)+8)+data
def geometry(mode=8,**changes):
 points=changes.get('points',bytes([13<<3,0,5,26<<3,0,5]))
 body=points if mode==8 else struct.pack('<12hII',*([0]*12),2,changes.get('jump_flags',0))+points
 total=42+len(body)+5
 return struct.pack('<IIfII9hI',changes.get('flags',mode),total,changes.get('units',2.),total,total-1,*changes.get('scale',(3,3,3)),*changes.get('position',(-1,-1,-1)),*changes.get('center',(0,0,0)),changes.get('count',2 if mode==8 else 1))+body+b'\0'*5

def fixture(mode=8,geom=None,frame=None,**changes):
 if geom is None:geom=geometry(mode)
 palette=bytearray(768);palette[15:18]=bytes([100,150,200]);palette=bytes([0,255])+palette+bytes(range(256))*24
 if frame is None:frame=chunk(0x4040301,b'\0')+chunk(0x4040302,struct.pack('<3f',1,2,3))+chunk(0x4040303,struct.pack('<9f',1,0,0,0,1,0,0,0,1))
 obj=chunk(0x4040100,b'test')+chunk(0x4040200,chunk(0x4040201,geom))+chunk(0x4040300,frame)
 scene=chunk(0x4010000,b'test')+chunk(0x4020000,struct.pack('<I',changes.get('objects',1)))+chunk(0x4030000,struct.pack('<I',changes.get('frames',1)))+chunk(0x4040000,obj)
 pal=chunk(0x1010000,b'palette')+chunk(0x1020000,palette)
 root=chunk(0x1000000,struct.pack('<I',changes.get('version',0x4121999a)))+chunk(0x2000000,b'test')+chunk(0x3000000,pal)+chunk(0x4000000,scene)
 return chunk(0x525643,root)

def digest(b):
 h=2166136261
 for x in b:h=((h^x)*16777619)&0xffffffff
 return f'{h:08x}'
def expected():
 geometry=b'test\0'+struct.pack('<fI',2,1)+struct.pack('<4IB',0,0,1,0,5)
 animation=b'\0'+struct.pack('<12f',1,2,3,1,0,0,0,1,0,0,0,1)
 palette=bytearray(768);palette[15:18]=bytes([100,150,200])
 return dict(objects='1',frames='1',voxels='1',geometry=digest(geometry),animation=digest(animation),palette=digest(palette*24),owned='1',transactional='1',rc='0')

def cases():
 good=fixture();out=[('single-header',good,True),('jump-header',fixture(10),True),('version10.2',fixture(version=0x41233333),True)]
 for n in [0,1,7,8,19,100,len(good)-1]:out.append((f'truncated-{n}',good[:n],False))
 out += [('surplus',good+b'\0',False),('unknown-version',fixture(version=0x41220000),False),('too-many-objects',fixture(objects=257),False),('too-many-frames',fixture(frames=257),False),('missing-object',fixture(objects=2),False),('wrong-frame-count',fixture(frames=2),False)]
 for name,kw in [('chained',{'flags':9}),('no-stop',{'points':bytes([13<<3,0,5,13<<3,0,5])}),('early-stop',{'points':bytes([26<<3,0,5,26<<3,0,5])}),('bad-direction',{'points':bytes([31<<3,0,5,26<<3,0,5])}),('count-zero',{'count':0}),('count-overflow',{'count':0xffffffff}),('zero-scale',{'scale':(0,3,3)}),('outside-bounds',{'position':(2,2,2)}),('units-zero',{'units':0}),('units-negative',{'units':-1}),('units-nan',{'units':float('nan')}),('units-infinite',{'units':float('inf')})]:out.append((name,fixture(geom=geometry(**kw)),False))
 out.append(('offset-jump',fixture(10,geom=geometry(10,jump_flags=1)),False))
 frame=chunk(0x4040301,b'\0')+chunk(0x4040302,struct.pack('<3f',float('nan'),0,0))
 out += [('nan-animation',fixture(frame=frame),False),('duplicate-animation-field',fixture(frame=chunk(0x4040301,b'\0')*2),False),('unknown-animation',fixture(frame=chunk(0xdeadbeef,b'')),False)]
 return out

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');args=ap.parse_args();out=Path(args.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe'
 flags=['-m68040','-static'] if args.m68k_linux else ['-g','-fsanitize=address,undefined']
 subprocess.run([args.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'cvr_model.cpp'),str(src/'cvr_probe.cpp'),'-o',str(exe)],check=True)
 report=[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');want=expected()
 for index,(name,data,valid) in enumerate(cases()):
  path=out/f'{index}.cvr';path.write_bytes(data);cmd=(["qemu-m68k",'-cpu','m68040'] if args.m68k_linux else [])+[str(exe),str(path)]
  p=subprocess.run(cmd,env=env,text=True,capture_output=True);assert p.returncode==(0 if valid else 20),(name,p.returncode,p.stdout,p.stderr)
  if valid:assert dict(x.split('=') for x in p.stdout.split())==want,(name,p.stdout,want)
  report.append(dict(name=name,valid=valid,rc=p.returncode))
 (out/'receipt.json').write_text(json.dumps(dict(cases=report,expected=want),indent=2)+'\n');print(f'{len(report)} owned CVR cases passed')
if __name__=='__main__':main()
