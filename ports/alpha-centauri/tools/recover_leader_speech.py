#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Privately recover named selection speeches from an owner's Planetary Pack ISO.
Needs local 7z and ffmpeg. No downloads/uploads; never modifies source media.
WAVs are preparation artifacts, not a claim of native MP3 decoding.
"""
import argparse,hashlib,json,subprocess
from pathlib import Path
STEMS='gaians hive univ morgan spartans believe peace cyborg pirates drone angels fungboy caretake usurper'.split()
def main():
 p=argparse.ArgumentParser();p.add_argument('iso',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
 if not a.iso.is_file():p.error('Original ISO not found')
 a.output.mkdir(parents=True,exist_ok=False)
 subprocess.run(['7z','x','-y','-o'+str(a.output),str(a.iso),*['voices/'+s+'.mp3' for s in STEMS]],check=True)
 records=[]
 for i,s in enumerate(STEMS):
  mp3=a.output/'voices'/(s+'.mp3');wav=mp3.with_suffix('.wav')
  subprocess.run(['ffmpeg','-v','error','-xerror','-nostdin','-i',str(mp3),'-map','0:a:0','-ac','1','-ar','22050','-c:a','pcm_s16le',str(wav)],check=True)
  records.append(dict(stem=s,expansion=i>=7,mp3SHA256=hashlib.sha256(mp3.read_bytes()).hexdigest(),wavSHA256=hashlib.sha256(wav.read_bytes()).hexdigest()))
 (a.output/'leader-speech-receipt.json').write_text(json.dumps({'scope':'faction-selection speeches, not dynamic diplomacy','records':records},indent=2)+'\n')
if __name__=='__main__':main()
