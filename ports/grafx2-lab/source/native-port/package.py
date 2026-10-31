#!/usr/bin/env python3
"""Package the built editor, data, classic tool icon and corresponding source.

No operating-system assets or automatically installed runtime libraries.
Generate the icon in a guest with target-tests/make-icon before packaging.
"""
import argparse
import hashlib
import os
import shutil
import struct
import tempfile
import zipfile
from pathlib import Path

base=Path(__file__).resolve().parent
repo=base.parent.parent
work=Path(os.environ.get('GRAFX2_WORK',base/'.work'))
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--icon',type=Path,required=True)
parser.add_argument('--output',type=Path,default=repo/'.build/dist/GrafX2-2.9-OpenUp-rc1.zip')
args=parser.parse_args()
binary=work/'build/GrafX2'
if binary.read_bytes()[:4]!=b'\0\0\x03\xf3':raise SystemExit('Expected a native Amiga HUNK executable')
icon=args.icon.read_bytes()
if len(icon)<78 or icon[:2]!=b'\xe3\x10' or icon[48]!=3:
    raise SystemExit('Expected a classic Workbench tool icon')
if struct.unpack_from('>I',icon,74)[0]<524288:
    raise SystemExit('The Workbench tool icon must request at least 512 KiB stack')
args.output.parent.mkdir(parents=True,exist_ok=True)
if args.output.exists():raise SystemExit('Output already exists; choose another name')
with tempfile.TemporaryDirectory(prefix='grafx2-package-') as tmp:
    root=Path(tmp)/'GrafX2-2.9-OpenUp-rc1'
    root.mkdir()
    shutil.copy2(binary,root/'GrafX2')
    shutil.copy2(args.icon,root/'GrafX2.info')
    shutil.copytree(work/'grafx2/share/grafx2',root/'data',ignore=shutil.ignore_patterns('scripts'))
    shutil.copytree(work/'grafx2',root/'source/grafx2',ignore=shutil.ignore_patterns('.git','*.o','*.sha256','__pycache__'))
    shutil.copytree(base,root/'source/native-port',ignore=shutil.ignore_patterns('.work','__pycache__'))
    shutil.copytree(repo/'licences',root/'licences')
    for name in ['LICENSE','COPYRIGHT.txt']:
        shutil.copy2(repo/name,root/name)
    shutil.copy2(work/'build/build-info.json',root/'BUILD-INFO.json')
    shutil.copytree(repo/'validation',root/'validation')
    if (repo/'docs').exists():shutil.copytree(repo/'docs',root/'docs')
    shutil.copy2(repo/'README-RELEASE.txt',root/'README-FIRST.txt')
    (root/'Start-GrafX2').write_text('FailAt 21\nStack 524288\nGrafX2\n')
    lines=[]
    for p in sorted(root.rglob('*')):
        if p.is_file():lines.append(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.relative_to(root).as_posix())
    (root/'SHA256SUMS').write_text('\n'.join(lines)+'\n')
    with zipfile.ZipFile(args.output,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for p in sorted(root.rglob('*')):
            if not p.is_file():continue
            info=zipfile.ZipInfo(p.relative_to(root.parent).as_posix(),date_time=(2026,10,31,0,0,0))
            info.compress_type=zipfile.ZIP_DEFLATED
            mode=0o755 if p.name in ['GrafX2','Start-GrafX2'] else 0o644
            info.external_attr=(0o100000|mode)<<16
            z.writestr(info,p.read_bytes())
digest=hashlib.sha256(args.output.read_bytes()).hexdigest()
args.output.with_suffix('.zip.sha256').write_text(digest+'  '+args.output.name+'\n')
print(args.output)
print(digest)
