#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write a synthetic stand-in for an Alpha Centauri install, for testing the
browser build without original files. Nothing here is Firaxis data:

- alphax.txt, labels.txt and faction files: the synthetic playable rules of
  native/test_play_session.py (expansion row layout);
- openinga.pcx / xopeninga.pcx (1024x768), texture.pcx, Units.pcx
  (1024x768), console.pcx (800x257) and faction art new1..new7.pcx:
  computed 8-bit PCX patterns;
- ALPHC___.TTF, arialn.ttf, Arial.ttf: copies of a free system font (Liberation
  Sans or DejaVu Sans) under the names the program asks for;
- sound: "opening menu.wav", fx/ok.wav, fx/menu up.wav, fx/menu down.wav
  (PCM sine tones), alpha-audio.cfg mapping turn_start and unit_move to
  ok.wav, and, made with local ffmpeg, music/aset1.mp3 and voices/tech<ID>.mp3
  (compressed, so the page has to convert them as it does the original MP3s);
- opening.amov / openingx.amov: a 3 second YUV420 + stereo PCM movie cache in
  the native startup_movie.h layout.

  make_fixture.py OUTDIR
"""
import math,shutil,struct,subprocess,sys
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

def wav(seconds,freq,rate=22050,channels=1,amp=0.4):
 n=int(seconds*rate);data=bytearray()
 for i in range(n):
  v=int(amp*32767*math.sin(2*math.pi*freq*i/rate))
  data+=struct.pack('<h',v)*channels
 return (b'RIFF'+struct.pack('<I',36+len(data))+b'WAVEfmt '+struct.pack('<IHHIIHH',16,1,channels,rate,rate*2*channels,2*channels,16)
  +b'data'+struct.pack('<I',len(data))+bytes(data))

def mp3(path,seconds,freq):
 subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i',f'sine=frequency={freq}:duration={seconds}:sample_rate=44100',
  '-ac','2','-c:a','libmp3lame','-b:a','64k',str(path)],check=True)

def amov(width,height,frames):
 """startup_movie.h: AMOVYUV1 header, then per frame YUV420 + 1470 stereo S16LE frames."""
 audio_frames=frames*1470
 out=bytearray(b'AMOVYUV1'+struct.pack('<8I',width,height,frames,22050,2,1470,audio_frames,0))
 for f in range(frames):
  out+=bytes(((x+y+f*8)*2)&255 for y in range(height) for x in range(width))
  out+=bytes([90+f*3&255])*(width*height//4)+bytes([200-f*2&255])*(width*height//4)
  for i in range(1470):
   v=int(0.3*32767*math.sin(2*math.pi*262*(f*1470+i)/22050));out+=struct.pack('<hh',v,v)
 return bytes(out)

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
 # Faction base art (new1..new7.pcx): squares on the magenta key colour.
 for f in range(1,8):
  fp=palette(13+f);fp[3*254:3*255]=[255,0,255]
  (out/f'new{f}.pcx').write_bytes(pcx(1024,768,lambda x,y,f=f:(20+f*25+(y//76)*3) if (abs(x%101-50)<22 and abs(y%76-40)<16) else 254,fp))
 (out/'console.pcx').write_bytes(pcx(800,257,lambda x,y:30+(y//32)*4+(x//200),palette(11)))
 fonts=[Path('/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf'),Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')]
 font=next((f for f in fonts if f.exists()),None)
 if not font:sys.exit('no free system font found for the fixture')
 for name in ('ALPHC___.TTF','arialn.ttf','Arial.ttf'):shutil.copyfile(font,out/name)
 (out/'opening menu.wav').write_bytes(wav(2.0,330))
 (out/'fx').mkdir();(out/'voices').mkdir();(out/'music').mkdir()
 for name,freq in (('ok.wav',880),('menu up.wav',660),('menu down.wav',520)):(out/'fx'/name).write_bytes(wav(0.25,freq))
 (out/'alpha-audio.cfg').write_text('# fixture: make turn starts and moves audible\nmap.turn_start=ok.wav\nmap.unit_move=ok.wav\nmusic_volume=80\n')
 if shutil.which('ffmpeg'):
  mp3(out/'music'/'aset1.mp3',6,440)
  mp3(out/'voices'/'tech0.mp3',1.5,600)
  for i in range(1,89):shutil.copyfile(out/'voices'/'tech0.mp3',out/'voices'/f'tech{i}.mp3')
 else:print('ffmpeg not found: no compressed music/voice fixture')
 for name in ('opening.amov','openingx.amov'):(out/name).write_bytes(amov(160,96,45))
 print(f'fixture: {sum(1 for p in out.rglob("*") if p.is_file())} files in {out} (font {font.name})')
if __name__=='__main__':main()
