#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write a synthetic stand-in for an Alpha Centauri install, for testing the
browser build without original files. Nothing here is Firaxis data:

- alphax.txt, labels.txt and faction files: the synthetic playable rules of
  native/test_play_session.py (expansion row layout);
- openinga.pcx / xopeninga.pcx (1024x768), texture.pcx, Units.pcx
  (1024x768) and console.pcx (800x257): computed 8-bit PCX patterns;
- ALPHC___.TTF, arialn.ttf, Arial.ttf: copies of a free system font (Liberation
  Sans or DejaVu Sans) under the names the program asks for.

  make_fixture.py OUTDIR
"""
import struct,sys,shutil
from pathlib import Path
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent/'native'))
import test_play_session as tps  # noqa: E402

def pcx(width,height,pixel,palette):
 """8-bit RLE PCX with a trailing 256-colour palette (pcx_image.cpp layout)."""
 stride=width+(width&1)
 head=struct.pack('<BBBBHHHHHH',10,5,1,8,0,0,width-1,height-1,72,72)+bytes(48)+struct.pack('<BBHHHH',0,1,stride,1,width,height)
 head+=bytes(128-len(head));body=bytearray()
 for y in range(height):
  row=[pixel(x,y) for x in range(width)]+([0] if stride>width else [])
  i=0
  while i<len(row):
   v=row[i];n=1
   while i+n<len(row) and row[i+n]==v and n<63:n+=1
   if n>1 or v>=0xc0:body+=bytes((0xc0|n,v))
   else:body.append(v)
   i+=n
 return head+bytes(body)+b'\x0c'+bytes(palette)

def palette(seed):
 p=[]
 for i in range(256):p+=[(i*seed)%256,(i*3+seed*40)%256,(255-i+seed*17)%256]
 p[0:3]=[8,12,20]
 return p

def title(x,y):
 # Bands and a ring: clearly not the original art.
 cx,cy=x-760,y-300
 if 140*140<cx*cx+cy*cy<160*160:return 200
 return 1+(y*5//768)*16+((x//64)%16)

def main():
 out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=False)
 tps.test_engine_wp0b.make_dir(out/'rules','expansion',rules=tps.playable_rules('expansion'))
 for f in (out/'rules').iterdir():f.rename(out/f.name)
 (out/'rules').rmdir()
 (out/'openinga.pcx').write_bytes(pcx(1024,768,title,palette(3)))
 (out/'xopeninga.pcx').write_bytes(pcx(1024,768,title,palette(7)))
 # Terrain atlas: every 56x56 cell a distinct colour; units sheet: blobs on the transparent key.
 tex_pal=palette(5)
 (out/'texture.pcx').write_bytes(pcx(1024,768,lambda x,y:16+((x//57)*7+(y//57)*13)%200,tex_pal))
 up=palette(9);up[3*250:3*251]=[152,24,228]
 (out/'Units.pcx').write_bytes(pcx(1024,768,lambda x,y:(40+(x//102)*9) if ((x%102-50)**2+(y%77-40)**2)<18*18 else 250,up))
 (out/'console.pcx').write_bytes(pcx(800,257,lambda x,y:30+(y//32)*4+(x//200),palette(11)))
 fonts=[Path('/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf'),Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')]
 font=next((f for f in fonts if f.exists()),None)
 if not font:sys.exit('no free system font found for the fixture')
 for name in ('ALPHC___.TTF','arialn.ttf','Arial.ttf'):shutil.copyfile(font,out/name)
 print(f'fixture: {len(list(out.iterdir()))} files in {out} (font {font.name})')
if __name__=='__main__':main()
