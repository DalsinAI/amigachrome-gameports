# SPDX-License-Identifier: GPL-3.0-or-later
"""Host ASan/UBSan control of the SDL audio service (device, cache, music
thread, voice, suspend/resume, settings) through SDL's disk output driver.
Synthetic WAVs only; needs host SDL2 development files."""
import argparse,json,os,struct,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');a=p.parse_args()
out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
data=out/'data'
for d in ('fx','voices','music'):(data/d).mkdir(parents=True)
def wav(path,value,frames,rate=22050):
    body=struct.pack('<%dh'%frames,*([value]*frames))
    fmt=struct.pack('<HHIIHH',1,1,rate,rate*2,2,16)
    path.write_bytes(b'RIFF'+struct.pack('<I',4+8+16+8+len(body))+b'WAVE'+b'fmt '+struct.pack('<I',16)+fmt+b'data'+struct.pack('<I',len(body))+body)
wav(data/'fx'/'ok.wav',1000,2205)
wav(data/'voices'/'gaians.wav',3000,11025)
wav(data/'music'/'gset1.wav',500,22050*2)
(data/'alpha-audio.cfg').write_text('music_volume=100\nmap.unit_move=missing.wav\n')
cflags=subprocess.run(['sdl2-config','--cflags'],capture_output=True,text=True,check=True).stdout.split()
libs=subprocess.run(['sdl2-config','--libs'],capture_output=True,text=True,check=True).stdout.split()
units=['audio_wav.cpp','audio_mixer.cpp','audio_events.cpp','audio_music.cpp','leader_speech.cpp','audio_service.cpp','audio_service_control.cpp']
subprocess.run([a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=undefined',*[f for f in cflags if not f.startswith('-I/usr/include/SDL2')],*[str(src/u) for u in units],*libs,'-o',str(out/'control')],check=True)
raw=out/'disk.raw'
env=dict(os.environ,SDL_AUDIODRIVER='disk',SDL_DISKAUDIOFILE=str(raw),ASAN_OPTIONS='detect_leaks=1')
r=subprocess.run([str(out/'control'),str(data),str(raw)],env=env,capture_output=True,timeout=120)
text=r.stdout.decode()
assert r.returncode==0 and text.rstrip().endswith('checks PASS'),(r.returncode,text,r.stderr.decode())
checks=int(text.rstrip().split('\n')[-1].split()[2])
(out/'receipt.json').write_text(json.dumps({'checks':checks,'driver':'SDL disk','architecture':'host-asan-ubsan','result':'PASS'})+'\n');print(text,end='')
