# SPDX-License-Identifier: GPL-3.0-or-later
"""Count 68040 instructions spent in the WP10 decoders and mixer under
qemu-m68k user mode (exact executed-instruction counts per function, from
qemu's in_asm + exec trace), then convert to an estimated 68040 CPU share.
Synthetic fixtures only (ffmpeg-generated). The cycle figure is an estimate:
qemu does not model 68040 timing, so instructions are weighted with
68040 manual figures (MULS/MULU.L 20, DIVS/DIVU 44, other 1.5 cycles)."""
import argparse,collections,json,re,shutil,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--mhz',type=float,default=25.0);a=p.parse_args()
out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
assert shutil.which('ffmpeg') and shutil.which('qemu-m68k'),'needs ffmpeg and qemu-m68k'
units=['audio_wav.cpp','audio_mixer.cpp','audio_events.cpp','audio_music.cpp','leader_speech.cpp','audio_control.cpp']
binary=out/'control'
subprocess.run(['m68k-linux-gnu-g++','-std=c++17','-O2','-m68040','-static','-fno-exceptions','-fno-rtti',*[str(src/u) for u in units],'-o',str(binary)],check=True)
syms=[]
for line in subprocess.run(['m68k-linux-gnu-nm','-C','-S','--defined-only',str(binary)],capture_output=True,text=True,check=True).stdout.splitlines():
    f=line.split(None,3)
    if len(f)==4 and f[2] in 'tTwW':syms.append((int(f[0],16),int(f[0],16)+int(f[1],16),f[3]))
syms.sort()
def owner(pc):
    lo,hi=0,len(syms)
    while lo<hi:
        mid=(lo+hi)//2
        if syms[mid][0]<=pc:lo=mid+1
        else:hi=mid
    if lo and syms[lo-1][0]<=pc<syms[lo-1][1]:return syms[lo-1][2]
    return '?'
seconds=4.0
tone='aevalsrc=0.6*sin(2*PI*440*t)+0.2*sin(2*PI*3000*t):s=22050:d=%g'%seconds
cases={'ima_mono':['-ac','1','-c:a','adpcm_ima_wav'],'ms_mono':['-ac','1','-c:a','adpcm_ms'],'pcm16_mono':['-ac','1','-c:a','pcm_s16le'],'pcm8_mono':['-ac','1','-c:a','pcm_u8']}
def trace(args):
    log=out/'trace.log'
    subprocess.run(['qemu-m68k','-cpu','m68040','-d','in_asm,exec,nochain','-D',str(log),str(binary),*args],check=True,capture_output=True)
    tb={};cur=None;execs=collections.Counter()
    with open(log,errors='replace') as f:
        for line in f:
            if line.startswith('IN:'):cur=[];continue
            m=re.match(r'^0x([0-9a-f]+):\s+(\S+)',line)
            if cur is not None and m:
                if not cur:start=int(m.group(1),16)
                cur.append(m.group(2));continue
            if cur is not None and line.strip()=='' and cur:tb[start]=cur;cur=None;continue
            m=re.match(r'^Trace \d+: \S+ \[[0-9a-f]+/([0-9a-f]+)/',line)
            if m:execs[int(m.group(1),16)]+=1
    if cur:tb[start]=cur
    per=collections.Counter();cyc=collections.Counter()
    for pc,n in execs.items():
        ins=tb.get(pc,[]);fn=owner(pc);per[fn]+=n*len(ins)
        for i in ins:cyc[fn]+=n*(20 if i.startswith(('muls','mulu')) else 44 if i.startswith(('divs','divu')) else 1.5)
    log.unlink();return per,cyc
results={}
for name,args in cases.items():
    wav=out/(name+'.wav');subprocess.run(['ffmpeg','-v','error','-nostdin','-f','lavfi','-i',tone,*args,str(wav)],check=True)
    per,cyc=trace(['--bench','decode',str(wav)])
    fn=[k for k in per if 'decode_wav_mono' in k or 'convert_pcm_frames' in k]
    ins=sum(per[k] for k in fn);cycles=sum(cyc[k] for k in fn)
    results[name]={'instructionsPerAudioSecond':round(ins/seconds),'estimatedCpuPercentAt%gMHz'%a.mhz:round(100*cycles/seconds/(a.mhz*1e6),2)}
for kind,label in (('mix','mixer_typical_2sfx_voice_music70'),('mix-worst','mixer_worst_3live_resampled_sfx_voice_ducked_music')):
    per,cyc=trace(['--bench',kind,'-'])
    fn=[k for k in per if 'AudioMixer' in k]
    ins=sum(per[k] for k in fn);cycles=sum(cyc[k] for k in fn)
    results[label]={'instructionsPerAudioSecond':ins,'estimatedCpuPercentAt%gMHz'%a.mhz:round(100*cycles/(a.mhz*1e6),2)}
receipt={'tool':'qemu-m68k -cpu m68040 in_asm+exec+nochain trace','compiler':'m68k-linux-gnu-g++ -O2 -m68040','cycleModel':'MUL 20, DIV 44, other 1.5 cycles (estimate, not measured timing)','results':results}
(out/'cost.json').write_text(json.dumps(receipt,indent=1)+'\n');print(json.dumps(receipt,indent=1))
