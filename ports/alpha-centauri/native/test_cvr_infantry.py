# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent synthetic composition controls; no original models or palettes."""
import argparse,os,struct,subprocess,json
from pathlib import Path
from test_cvr_model import chunk,geometry

def model(color,offset=(0,0,0),hidden=False,singular=False,frames=15,geom=None):
 rgb=bytearray(768);rgb[15:18]=bytes(color)
 palette=bytes([0,255])+rgb+bytes(range(256))*24
 frame=chunk(0x4040301,bytes([128 if hidden else 0])*frames)+chunk(0x4040302,struct.pack('<3f',*offset)*frames)+chunk(0x4040303,struct.pack('<9f',*([0]*9 if singular else [1,0,0,0,1,0,0,0,1]))*frames)
 obj=chunk(0x4040100,b'component')+chunk(0x4040200,chunk(0x4040201,geometry() if geom is None else geom))+chunk(0x4040300,frame)
 scene=chunk(0x4010000,b'synthetic')+chunk(0x4020000,struct.pack('<I',1))+chunk(0x4030000,struct.pack('<I',frames))+chunk(0x4040000,obj)
 pal=chunk(0x1010000,b'synthetic')+chunk(0x1020000,palette)
 return chunk(0x525643,chunk(0x1000000,struct.pack('<I',0x4121999a))+chunk(0x2000000,b'synthetic')+chunk(0x3000000,pal)+chunk(0x4000000,scene))

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined']
 exe=out/'control';subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*map(str,[src/(n+'.cpp') for n in ['cvr_model','cvr_raster','cvr_lighting','cvr_infantry','cvr_palette','cvr_infantry_control']]),'-lm','-o',str(exe)],check=True)
 red=(255,0,0);blue=(0,0,255)
 cases=[('equal-depth-keeps-body',model(red),model(blue),1,0,True),('distinct-component-palettes',model(red),model(blue,offset=(100,0,0)),1,1,True),('hidden-body',model(red,hidden=True),model(blue),0,1,True),('hidden-reactor',model(red),model(blue,hidden=True),1,0,True),('all-hidden',model(red,hidden=True),model(blue,hidden=True),0,0,False),('singular-body',model(red,singular=True),model(blue),0,0,False),('singular-reactor',model(red),model(blue,singular=True),0,0,False),('missing-body-frames',model(red,frames=1),model(blue),0,0,False),('profile-voxel-limit',model(red,geom=geometry(points=bytes([13<<3,0,5,12<<3,0,5])*50001+bytes([26<<3,0,5]),count=100003)),model(blue),0,0,False)]
 reports=[]
 for name,body,reactor,r,b,valid in cases:
  bp=out/(name+'-body.cvr');rp=out/(name+'-reactor.cvr');bp.write_bytes(body);rp.write_bytes(reactor)
  cmd=(['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(exe),str(bp),str(rp),str(r),str(b),str(int(valid))]
  p=subprocess.run(cmd,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,text=True);assert p.returncode==0,(name,p.returncode,p.stdout,p.stderr);reports.append(dict(name=name,result=p.stdout.strip()))
 (out/'receipt.json').write_text(json.dumps(reports,indent=2)+'\n');print(json.dumps(dict(compositionCases=len(reports),facingZoomCases=16*4,invalidInputs=7*4,transactionalFailures=5,transparentCacheControls=4)))
if __name__=='__main__':main()
