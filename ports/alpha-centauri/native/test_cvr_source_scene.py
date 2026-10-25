# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free source-scene vectors, exact independent numeric goldens and cache checks."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_cvr_model import chunk
from test_cvr_speeder import NAMES

def component(palette,offset,hidden,transparent,lod=False):
 directions=[13,21,4,12,0,25];normals=[0,89,1000,2047,512,1777];colors=[5,17,9,33,5,19]
 triples=b''.join(bytes([(d<<3)|(n>>8),n&255,c])for d,n,c in zip(directions,normals,colors))+bytes([26<<3,0,0]);size=42+len(triples)+5
 geometry=struct.pack('<IIfII9hI',8,size,32.,size,size-1,32,32,32,-16,-16,-16,0,0,0,len(directions)+1)+triples+b'\0'*5
 if lod:
  first=bytearray(geometry);second=bytearray(geometry);struct.pack_into('<I',first,0,9);struct.pack_into('<I',first,4,size*2);struct.pack_into('<f',first,8,8.);struct.pack_into('<f',second,8,16.);geometry=bytes(first+second)
 shades=bytes([9])*6144 if transparent else bytes((c+7*s)&255 for s in range(24)for c in range(256))
 pal=chunk(0x1010000,b'synthetic')+chunk(0x1020000,bytes([0,255])+palette+shades)
 frames=15;animation=chunk(0x4040301,bytes([128 if hidden else 0])*frames)+chunk(0x4040302,struct.pack('<3f',*offset)*frames)+chunk(0x4040303,struct.pack('<9f',1,0,0,0,1,0,0,0,1)*frames)
 obj=chunk(0x4040100,b'synthetic')+chunk(0x4040200,chunk(0x4040201,geometry))+chunk(0x4040300,animation)
 scene=chunk(0x4010000,b'synthetic')+chunk(0x4020000,struct.pack('<I',1))+chunk(0x4030000,struct.pack('<I',frames))+chunk(0x4040000,obj)
 return chunk(0x525643,chunk(0x1000000,struct.pack('<I',0x4121999a))+chunk(0x2000000,b'synthetic')+chunk(0x3000000,pal)+chunk(0x4000000,scene))

def make_fixture(root,panel,transparent,lod=False):
 root.mkdir();inf=root/'infantry';rover=root/'rover';inf.mkdir();rover.mkdir();palette=bytes(v for i in range(256)for v in [i,(i*3)&255,(i*7)&255]);tint=bytes([255,255,255]if transparent else [100,220,30]);(root/'palette.bin').write_bytes(palette);(root/'tint.bin').write_bytes(tint)
 for name,offset in [('VI.cvr',(400,-200,0)),('Viptr00.cvr',(400,-200,10))]:(inf/name).write_bytes(component(palette,offset,False,transparent,lod))
 for i,name in enumerate(NAMES):(rover/name).write_bytes(component(palette,(20,0,0),i!=panel,transparent,lod))
 return inf,rover

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'control';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'];names=['cvr_source_unit','cvr_palette','cvr_model','cvr_map_anchor','cvr_projection','cvr_fixed_transform','cvr_lighting','cvr_raster','cvr_scene','cvr_scene_blit','pcx_image','cvr_source_unit_probe'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti',*flags,*[str(src/(n+'.cpp'))for n in names],'-lm','-o',str(exe)],check=True)
 expected=json.loads((src/'SOURCE-SYNTHETIC-GOLDENS-20261025.json').read_text());records=[]
 for name,panel,transparent,lod in [('panel2',2,False,False),('panel4',4,False,False),('key9',2,True,False),('lod-switch',2,False,True)]:
  root=out/name;inf,rover=make_fixture(root,panel,transparent,lod);command=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(rover),str(inf),str(root/'palette.bin'),str(root/'tint.bin')];r=subprocess.run(command,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=240);(root/'output.bin').write_bytes(r.stdout);(root/'stderr.log').write_bytes(r.stderr);assert r.returncode==0,(name,r.returncode,r.stderr);assert len(r.stdout)==32*196608,(name,len(r.stdout));digests=[hashlib.sha256(r.stdout[i*196608:(i+1)*196608]).hexdigest()for i in range(32)];assert digests==expected['cases'][name]['sceneSHA256'],(name,[(i,x,y)for i,(x,y)in enumerate(zip(digests,expected['cases'][name]['sceneSHA256']))if x!=y]);records.append(dict(name=name,scenes=32,outputSHA256=hashlib.sha256(r.stdout).hexdigest(),rgbComposites=90,checks=r.stderr.decode().strip()));print(name,records[-1]['outputSHA256'])
 (out/'receipt.json').write_text(json.dumps(dict(cases=records,independentOriginalGoldens=True,ownerAssetsUsed=False,originalBytesUsed=False,bigEndian=a.m68k_linux),indent=2)+'\n');print('SOURCE SCENE128 exact vectors +360RGB/cache/border/key/LOD controls PASS')
if __name__=='__main__':main()
