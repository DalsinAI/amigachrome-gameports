#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import struct,json,argparse,subprocess,hashlib,os
from test_vehicle_removal import cases,SOURCES as STORAGE_SOURCES
from test_original_game import decode
SOURCES=[x for x in STORAGE_SOURCES if x!='vehicle_removal_probe.cpp']+['world_vehicle_removal.cpp','world_vehicle_removal_probe.cpp']
def expected(data,victim,old):
 info,offsets,sp=decode(data);n=int(info['vehicles']);nt=int(info['width'])//2*int(info['height']);actors=bytearray(old[12:12+n*52]);target=(victim+1)%n
 if target!=victim:
  i=target-int(target>victim);actors[i*52+16]^=0x5a;struct.pack_into('<I',actors,i*52+4,struct.unpack_from('<I',actors,i*52+4)[0]^0x400)
 tiles=bytearray(old[12+n*52:12+n*52+nt*44]);tiles[1]^=1
 removed=bytearray(sp['vehicles'][victim*52:(victim+1)*52])
 if target==victim:removed[16]^=0x5a;struct.pack_into('<I',removed,4,struct.unpack_from('<I',removed,4)[0]^0x400)
 return struct.pack('<III',0,n-1,20)+actors[:(n-1)*52]+tiles+old[12+n*52+nt*44:]+removed
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--reuse',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 if a.reuse:
  for f in ('inputs.bin','expected.bin'):(out/f).symlink_to(a.reuse.resolve()/f)
  total=struct.unpack('<I',(out/'inputs.bin').open('rb').read(4))[0];want=(out/'expected.bin').read_bytes()
 else:
  rows=list(cases());total=len(rows);want=b''.join(expected(data,id,old)for data,id,var,old in rows);(out/'inputs.bin').write_bytes(struct.pack('<I',total)+b''.join(struct.pack('<III',len(data),id,var)+data for data,id,var,_ in rows));(out/'expected.bin').write_bytes(want)
 flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all']);exe=out/'probe';subprocess.run([a.cc,*flags,*[str(src/x)for x in SOURCES],'-o',str(exe)],check=True)
 r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(out/'inputs.bin')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(out/'actual.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert not r.returncode,(r.returncode,len(r.stdout),r.stderr[:1200]);assert r.stdout==want,(len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));receipt={'cases':total,'noWriteGuards':total*20,'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'scope':'Current-record clone/removal/count, unchanged caller/runtime/movementRNG/immutable baseline, all other decoded records/current monuments/history preserved; independent second removal and replacement; survives source destruction.'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
