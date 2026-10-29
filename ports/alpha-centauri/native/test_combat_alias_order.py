# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,subprocess,json,hashlib,os
from test_battle_aftermath import SOURCES
from test_original_world import valid_fixture
P=argparse.ArgumentParser();P.add_argument('--out',type=Path,required=True);P.add_argument('--cc',default='g++');P.add_argument('--m68k-linux',action='store_true');P.add_argument('--expect-rejection-bug',action='store_true');a=P.parse_args();D=a.out.resolve();D.mkdir(parents=True,exist_ok=False);S=Path(__file__).resolve().parent;sources=[s for s in SOURCES if s!='battle_aftermath_probe.cpp']+['combat_alias_order_probe.cpp'];flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);subprocess.run([a.cc,*flags,*[str(S/s)for s in sources],'-o',str(D/'probe')],check=True)
expected=b''.join(bytes([stage,j,0,0])for stage in range(3)for j in range(4));total=0;digest=hashlib.sha256()
for version in [7,12]:
 p=D/('v%d.SC'%version);p.write_bytes(valid_fixture(version));r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(D/'probe'),str(p)],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(D/('v%d.actual'%version)).write_bytes(r.stdout);(D/('v%d.stderr'%version)).write_bytes(r.stderr)
 if a.expect_rejection_bug:
  assert r.returncode and b'AddressSanitizer'in r.stderr,(r.returncode,r.stderr[:1000]);receipt={'diagnostic':'Expected old alias-order bug reproduced; not a passing candidate','exitCode':r.returncode,'stderrSHA256':hashlib.sha256(r.stderr).hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest()};break
 assert not r.returncode,(version,r.returncode,r.stderr[:1500]);assert r.stdout==expected,(version,r.stdout);total+=12;digest.update(r.stdout)
else:receipt={'cases':total,'poisonedInputNoRead':not a.m68k_linux,'inputBytesUnchanged':True,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest(),'native':'UNRUN'}
(D/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
