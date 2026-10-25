#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original saved faction state/definitions vs separately transcribed wire layouts."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_game import cases,decode,FORMATS,SRC
from test_original_map import fnv,word
SCHEMA=json.loads((SRC/'original_faction_wire.json').read_text())['records']

def record_hash(data,kind,legacy=False):
 h=2166136261;old={f['name']:f for f in SCHEMA['LegacyFactionV10']['fields']}
 for f in SCHEMA[kind]['fields']:
  source=old[f['name']] if legacy and kind=='Faction' else f;off=source['offset'];count=f['count'];typ=f['type']
  if typ=='char':h=fnv(data[off:off+count],h);continue
  if typ=='Goal':
   for i in range(count):
    for n in struct.unpack_from('<hhiii',data,off+i*16):h=word(h,n)
   continue
  fmt=FORMATS[typ];width=struct.calcsize(fmt)
  for i in range(count):
   value=0 if i>=source['count'] else struct.unpack_from('<'+fmt,data,off+i*width)[0]
   h=word(h,value)
 return h

def model(data):
 info,_,spans=decode(data);legacy=int(info['version'])<11;size=0x20bc if legacy else 0x20cc;states=defs=2166136261
 for i in range(8):
  states=word(states,record_hash(spans['factions'][i*size:(i+1)*size],'Faction',legacy));defs=word(defs,record_hash(spans['faction_definitions'][i*0x59c:(i+1)*0x59c],'MFaction'))
 return dict(version=info['version'],states=f'{states:08x}',definitions=f'{defs:08x}',rc='0')

def controls():
 # Include full parent format corpus: malformed containers cannot expose records.
 result=cases()
 # Targeted all-zero/all-ones/alternating data in both faction blocks exercises
 # every signed width, padding byte, matrix row, goal and legacy extension.
 for version in [7,10,11,12]:
  from test_original_game import fixture
  data=fixture(version);_,offsets,spans=decode(data)
  for pattern in [b'\0',b'\xff',b'\x80\x7f\xff\x00']:
   b=bytearray(data)
   for name in ['factions','faction_definitions']:
    n=len(spans[name]);b[offsets[name]:offsets[name]+n]=(pattern*((n+len(pattern)-1)//len(pattern)))[:n]
   result.append((f'faction-v{version}-pattern{pattern.hex()}',bytes(b),True))
 return result

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'faction-state-probe';cmd=[a.cc,*flags,*[str(SRC/n) for n in ['original_map.cpp','original_game.cpp','original_faction.cpp','original_faction_probe.cpp']],'-o',str(exe)];subprocess.run(cmd,check=True)
 corpus=controls()+[('original-'+p.name,p.read_bytes(),True) for p in a.original];records=[]
 for i,(name,data,valid) in enumerate(corpus):
  path=out/(str(i)+'.SC');path.write_bytes(data);r=subprocess.run([*runner,str(exe),str(path)],capture_output=True,text=True,timeout=30,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  assert r.returncode==(0 if valid else 20),(name,r.returncode,r.stdout,r.stderr)
  expected=None
  if valid:
   expected=model(data);actual=dict(s.split('=') for s in r.stdout.split());assert actual==expected,(name,actual,expected)
  records.append(dict(name=name,valid=valid,passed=True,sha256=hashlib.sha256(data).hexdigest(),result=expected))
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,cases=records,binarySHA256=hashlib.file_digest(exe.open('rb'),'sha256').hexdigest()),indent=2)+'\n');print(f'{len(records)} original-faction-state cases passed')
if __name__=='__main__':main()
