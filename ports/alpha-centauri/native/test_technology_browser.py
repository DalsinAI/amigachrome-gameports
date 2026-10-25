# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned technology graph and original-description boundary controls; no assets."""
import argparse,json,os,subprocess
from pathlib import Path
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined'];prefix=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'rules_text.cpp'),str(src/'technology_browser.cpp'),str(src/'technology_browser_control.cpp'),'-o',str(out/'control')],check=True)
p=subprocess.run(prefix+[str(out/'control')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert p.returncode==0 and p.stdout==b'TECHNOLOGY BROWSER 17 checks PASS\n',(p.returncode,p.stdout,p.stderr)
(out/'receipt.json').write_text(json.dumps(dict(controls=17,returnCode=0,architecture='big-endian68040' if a.m68k_linux else 'host-ASan-UBSan'),indent=2)+'\n');print(p.stdout.decode(),end='')
