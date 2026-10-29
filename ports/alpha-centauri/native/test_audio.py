# SPDX-License-Identifier: GPL-3.0-or-later
"""WP10 audio controls. No original assets: synthetic WAVs are written by the
control itself; ADPCM/PCM oracle fixtures are generated here with local ffmpeg
(and decoded by ffmpeg as the reference) when it is available."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++')
p.add_argument('--m68k-linux',action='store_true');p.add_argument('--no-oracle',action='store_true');a=p.parse_args()
out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
(out/'scratch'/'music').mkdir(parents=True)
flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=undefined']
units=['audio_wav.cpp','audio_mixer.cpp','audio_events.cpp','audio_music.cpp','leader_speech.cpp','audio_control.cpp']
subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/u) for u in units],'-o',str(out/'control')],check=True)
oracle=None;fixtures=[]
if not a.no_oracle and shutil.which('ffmpeg'):
    oracle=out/'oracle';oracle.mkdir()
    tone='aevalsrc=0.6*sin(2*PI*440*t)+0.3*sin(2*PI*1375*t)*sin(2*PI*3*t)|0.5*sin(2*PI*660*t):s=22050:d=1.3'
    cases=[('ima_mono',['-ac','1','-c:a','adpcm_ima_wav']),('ima_stereo',['-ac','2','-c:a','adpcm_ima_wav']),
           ('ms_mono',['-ac','1','-c:a','adpcm_ms']),('ms_stereo',['-ac','2','-c:a','adpcm_ms']),
           ('u8_stereo',['-ac','2','-ar','11025','-c:a','pcm_u8']),('s16_44k',['-ac','1','-ar','44100','-c:a','pcm_s16le'])]
    for name,args in cases:
        wav=oracle/(name+'.wav')
        subprocess.run(['ffmpeg','-v','error','-nostdin','-f','lavfi','-i',tone,*args,str(wav)],check=True)
        subprocess.run(['ffmpeg','-v','error','-nostdin','-i',str(wav),'-f','s16le','-c:a','pcm_s16le',str(oracle/(name+'.s16'))],check=True)
        fixtures.append(name)
prefix=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
cmd=prefix+[str(out/'control'),str(out/'scratch')]+([str(oracle)] if oracle else [])
r=subprocess.run(cmd,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1'),capture_output=True)
text=r.stdout.decode()
assert r.returncode==0 and text.rstrip().endswith('checks PASS'),(r.returncode,text,r.stderr.decode())
checks=int(text.rstrip().split('\n')[-1].split()[1])
receipt={'checks':checks,'oracleFixtures':fixtures,'architecture':'big-endian-68040-qemu' if a.m68k_linux else 'host-asan-ubsan','result':'PASS'}
(out/'receipt.json').write_text(json.dumps(receipt)+'\n');print(text,end='')
