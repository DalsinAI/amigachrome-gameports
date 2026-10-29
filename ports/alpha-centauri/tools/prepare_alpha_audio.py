#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owner-side preparation of the in-game audio media (WP10).

Reads the owner's original Alpha Centauri installation (or an extracted
Planetary Pack disc) and writes a NEW private directory to copy beside the
native executable:

  fx/*.wav          original effects; PCM / IMA / MS ADPCM kept byte-identical,
                    any other WAV codec converted to PCM16LE mono 22050
  voices/*.wav      every voices/*.mp3 decoded to PCM16LE mono 22050
  music/<set>.wav   only from --rendered-music (see AUDIO.md); the AMB scores
                    themselves are not playable yet
  alpha-audio.cfg.example
                    every event with its default, plus the local fx listing and
                    .wav names found in terranx.exe as mapping hints
  audio-receipt.json

Needs local ffmpeg. No downloads or uploads; never modifies the source tree.
Nothing produced here may be committed.
"""
import argparse,hashlib,json,re,struct,subprocess,sys
from pathlib import Path
EVENTS=['ui_click','menu_open','menu_close','panel_slide','command_menu','quit_prompt','unit_move','unit_attack',
        'combat_hit','psi_attack','unit_destroyed','psi_destroyed','base_built','facility_complete','tech_discovered',
        'turn_start','turn_complete']
SETS=['aset1','gset1','mset1','uset2','sset1','bset1']
def find_dir(root,name):
    for p in root.iterdir():
        if p.is_dir() and p.name.lower()==name:return p
    return None
def resolve(root,relative):
    # Case-insensitive like the original Windows and AmigaOS file systems.
    cur=root
    for part in relative.replace('\\','/').split('/'):
        if not part:continue
        nxt=cur/part
        if not nxt.exists():
            nxt=next((c for c in cur.iterdir() if c.name.lower()==part.lower()),None) if cur.is_dir() else None
            if nxt is None:return None
        cur=nxt
    return cur
def wav_tag(path):
    b=path.read_bytes()
    if len(b)<12 or b[:4]!=b'RIFF' or b[8:12]!=b'WAVE':return None
    pos=12
    while pos+8<=len(b):
        cid,n=b[pos:pos+4],struct.unpack('<I',b[pos+4:pos+8])[0]
        if cid==b'fmt ' and n>=16:return struct.unpack('<H',b[pos+8:pos+10])[0]
        pos+=8+n+(n&1)
    return None
def ffmpeg_pcm(src,dst):
    subprocess.run(['ffmpeg','-v','error','-xerror','-nostdin','-i',str(src),'-map','0:a:0','-ac','1','-ar','22050','-c:a','pcm_s16le',str(dst)],check=True)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('original',type=Path,help='original installation or extracted disc directory')
    ap.add_argument('output',type=Path,help='new private output directory')
    ap.add_argument('--rendered-music',type=Path,help='owner recordings named <set>[-N].<ext>, e.g. gset1-1.flac')
    a=ap.parse_args()
    if not a.original.is_dir():ap.error('original directory not found')
    a.output.mkdir(parents=True,exist_ok=False)
    receipt={'fx':[],'voices':[],'music':[],'amb':[],'exeWavNames':[]}
    fx=find_dir(a.original,'fx')
    if fx:
        (a.output/'fx').mkdir()
        for src in sorted(fx.glob('*')):
            if src.suffix.lower()!='.wav':continue
            dst=a.output/'fx'/src.name;tag=wav_tag(src)
            if tag in (1,2,0x11):dst.write_bytes(src.read_bytes());how='copied'
            else:ffmpeg_pcm(src,dst);how='converted from tag %s'%tag
            receipt['fx'].append({'name':src.name,'how':how,'sourceSHA256':sha(src)})
    voices=find_dir(a.original,'voices')
    if voices:
        (a.output/'voices').mkdir()
        for src in sorted(voices.glob('*')):
            if src.suffix.lower()!='.mp3':continue
            dst=a.output/'voices'/(src.stem.lower()+'.wav')
            try:ffmpeg_pcm(src,dst);receipt['voices'].append({'name':dst.name,'sourceSHA256':sha(src)})
            except subprocess.CalledProcessError:receipt['voices'].append({'name':src.name,'error':'strict decode failed; not prepared'})
    # AMB score definitions: list them and the sample files they reference.
    for amb in sorted(p for p in a.original.rglob('*') if p.suffix.lower()=='.amb'):
        refs=sorted({m.decode('latin-1') for m in re.findall(rb'[\x20-\x7e]{1,80}?\.wav',amb.read_bytes(),re.I)})
        present=[r for r in refs if resolve(a.original,r)]
        receipt['amb'].append({'file':str(amb.relative_to(a.original)),'sampleReferences':len(refs),'present':len(present)})
    if a.rendered_music:
        (a.output/'music').mkdir()
        for src in sorted(a.rendered_music.iterdir()):
            m=re.fullmatch(r'([a-z]set\d)(?:-(\d))?',src.stem.lower())
            if not m or m.group(1) not in SETS:continue
            dst=a.output/'music'/(src.stem.lower()+'.wav');ffmpeg_pcm(src,dst)
            receipt['music'].append({'name':dst.name,'sourceSHA256':sha(src)})
    exe=next((p for p in a.original.iterdir() if p.name.lower() in ('terranx.exe','terran.exe')),None)
    if exe:receipt['exeWavNames']=sorted({m.decode('latin-1').split('\\')[-1] for m in re.findall(rb'[\x20-\x7e]{2,64}?\.wav',exe.read_bytes(),re.I)})
    lines=['# Copy to alpha-audio.cfg beside the executable and edit.','# map.<event>=<fx file name>  |  none  |  default','sound_effects=on','music=on','voices=on','master_volume=100','effects_volume=100','music_volume=70','voice_volume=100','music_under_voice=40','voice_stops_on_close=on','']
    lines+=['# map.%s=default'%e for e in EVENTS]
    lines+=['','# Local fx files:']+['#   '+f['name'] for f in receipt['fx']]
    lines+=['','# .wav names in %s:'%(exe.name if exe else 'the executable (not found)')]+['#   '+n for n in receipt['exeWavNames']]
    (a.output/'alpha-audio.cfg.example').write_text('\n'.join(lines)+'\n')
    (a.output/'audio-receipt.json').write_text(json.dumps(receipt,indent=1)+'\n')
    print('fx %d, voices %d, music %d, AMB scores %d (not playable: no sequencer)'%(len(receipt['fx']),len(receipt['voices']),len(receipt['music']),len(receipt['amb'])))
if __name__=='__main__':sys.exit(main())
