# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic check of tools/prepare_alpha_audio.py, then of the prepared files
with the native WAV reader (host build). Needs ffmpeg with an MP3 encoder."""
import argparse,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--out',required=True);a=p.parse_args()
out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
orig=out/'original';(orig/'FX').mkdir(parents=True);(orig/'voices').mkdir();(orig/'music').mkdir()
tone='sine=frequency=440:sample_rate=22050:duration=0.3'
def ff(*args):subprocess.run(['ffmpeg','-v','error','-nostdin','-f','lavfi','-i',tone,*args],check=True)
ff('-c:a','pcm_s16le',str(orig/'FX'/'ok.wav'))
ff('-c:a','adpcm_ima_wav',str(orig/'FX'/'ima.wav'))
ff('-c:a','pcm_f32le',str(orig/'FX'/'float.wav'))
ff('-ac','1','-c:a','libmp3lame','-b:a','32k',str(orig/'voices'/'gaians.mp3'))
ff('-ac','1','-c:a','libmp3lame','-b:a','32k',str(orig/'voices'/'tech3.mp3'))
(orig/'voices'/'bad.mp3').write_bytes(b'\xff\xfb\x00garbage')
(orig/'music'/'gset1.amb').write_bytes(b'\x01\x02fx\\OK.WAV\x00amb\\missing.wav\x00')
(orig/'terranx.exe').write_bytes(b'MZ\x00\x00fx\\psi blow.wav\x00cpu turn complete.wav\x00')
ren=out/'rendered';ren.mkdir();subprocess.run(['ffmpeg','-v','error','-nostdin','-f','lavfi','-i',tone,'-c:a','flac',str(ren/'gset1-1.flac')],check=True)
(ren/'notaset.flac').write_bytes(b'')
dst=out/'prepared'
r=subprocess.run(['python3',str(src.parent/'tools'/'prepare_alpha_audio.py'),str(orig),str(dst),'--rendered-music',str(ren)],capture_output=True,text=True)
assert r.returncode==0,r.stderr
rec=json.loads((dst/'audio-receipt.json').read_text())
fx={f['name']:f['how'] for f in rec['fx']}
assert fx['ok.wav']=='copied' and fx['ima.wav']=='copied' and fx['float.wav'].startswith('converted'),fx
assert (dst/'fx'/'ok.wav').read_bytes()==(orig/'FX'/'ok.wav').read_bytes()
assert sorted(v['name'] for v in rec['voices'] if 'error' not in v)==['gaians.wav','tech3.wav'] and any('error' in v for v in rec['voices'])
assert [m['name'] for m in rec['music']]==['gset1-1.wav'] and rec['amb']==[{'file':'music/gset1.amb','sampleReferences':2,'present':1}]
assert rec['exeWavNames']==['cpu turn complete.wav','psi blow.wav'] and 'map.unit_move=default' in (dst/'alpha-audio.cfg.example').read_text()
# Every prepared WAV must load with the native reader.
probe=out/'probe.cpp'
probe.write_text('#include "audio_wav.h"\n#include <stdio.h>\n#include <stdlib.h>\nint main(int c,char**v){int bad=0;for(int i=1;i<c;++i){FILE*f=fopen(v[i],"rb");static unsigned char b[1<<22];size_t n=fread(b,1,sizeof b,f);fclose(f);alpha::WavInfo w;size_t k=0;static short o[1<<21];bool ok=alpha::parse_wav(b,n,n,w)==alpha::WavError::ok&&alpha::decode_wav_mono(b,n,w,o,1<<21,k)==alpha::WavError::ok&&k>1000;printf("%s %d %lu\\n",v[i],ok,(unsigned long)k);bad|=!ok;}return bad;}\n')
subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-I',str(src),str(probe),str(src/'audio_wav.cpp'),'-o',str(out/'probe')],check=True)
files=sorted(str(f) for f in dst.rglob('*.wav'))
r=subprocess.run([str(out/'probe'),*files],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr
print('AUDIO PREPARE %d prepared WAVs load; receipt checks PASS'%len(files))
