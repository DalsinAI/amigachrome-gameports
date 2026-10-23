#!/usr/bin/env python3
"""Build the native IPv4 probe against a prepared OpenLoco build's libraries."""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'scripts'))
from build_sdl3_port import verify_stove,digest
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--stove',required=True,type=Path);p.add_argument('--port-build',required=True,type=Path)
p.add_argument('--sdl3-sdk',required=True,type=Path);p.add_argument('--out',required=True,type=Path)
a=p.parse_args();stove=a.stove.resolve();build=a.port_build.resolve();out=a.out.resolve();sdk=a.sdl3_sdk.resolve()
verify_stove(stove)
receipt=json.loads((build/'receipt.json').read_text())
if receipt['source']['commit']!='6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e':raise ValueError('wrong source pin')
if out.exists():raise ValueError('use a fresh output directory')
out.mkdir(parents=True)
flags={line.split(' = ',1)[0]:line.split(' = ',1)[1] for line in (build/'cmake/src/OpenLoco/CMakeFiles/OpenLoco.dir/flags.make').read_text().splitlines() if ' = ' in line}
cmd=[str(stove/'prefix/bin/m68k-amigaos-g++')]
for name in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']:cmd+=shlex.split(flags[name])
port=Path(__file__).resolve().parents[1]
cmd+=[str(port/'probes/bsdsocket.cpp'),str(build/'source/src/OpenLoco/src/Network/Socket.cpp'),str(port/'compat/AmigaSocket.cpp'),str(port/'compat/StatVfs.Amiga.cpp'),'-Wl,--start-group']
libs=[build/'cmake'/name for name in ['src/Diagnostics/libDiagnostics.a','src/Platform/libPlatform.a','src/Core/libCore.a','src/Utility/libUtility.a','_deps/fmt-build/libfmt.a']]+[sdk/'libSDL3.a']
cmd+=[str(x) for x in libs]+['-lamiga','-lm','-Wl,--end-group','-o',str(out/'bsdsocket-probe')]
with (out/'build.log').open('w') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
proof={'command':cmd,'input_library_sha256':{str(x):digest(x) for x in libs},'binary_sha256':digest(out/'bsdsocket-probe'),'runtime':'not tested'}
(out/'receipt.json').write_text(json.dumps(proof,indent=2)+'\n')
print(out/'bsdsocket-probe')
