#!/usr/bin/env python3
"""Fetch verified source inputs and apply the native patch without changing an existing tree."""
from pathlib import Path
import subprocess,os,hashlib,urllib.request,tarfile
base=Path(__file__).resolve().parent
work=Path(os.environ.get('GRAFX2_WORK',base/'.work'))
work.mkdir(parents=True,exist_ok=True)
def git(*args):subprocess.run(['git',*map(str,args)],check=True)
def fetch(url,path,revision):
 if not path.exists():git('clone','--depth','1',url,path)
 git('-C',path,'fetch','origin',revision)
 if subprocess.check_output(['git','-C',str(path),'status','--porcelain']):
  raise SystemExit(f'{path} has changes; refusing to replace them')
 git('-C',path,'checkout','--detach',revision)
src=work/'grafx2'
patches=sorted((base/'patches').glob('*.patch'))
stamp=work/'prepared.sha256'
digest=hashlib.sha256(b''.join(p.name.encode()+b'\0'+p.read_bytes() for p in patches)).hexdigest()
if stamp.exists():
 if stamp.read_text()!=digest:raise SystemExit('Patch changed: choose a fresh GRAFX2_WORK directory')
 print('Existing prepared sources retained')
else:
 fetch('https://gitlab.com/GrafX2/grafX2.git',src,'678a0f54ea7f3eeebc3fa4135213ea63f45a28bd')
 for patch in patches:
  git('-C',src,'apply','--check',patch)
  git('-C',src,'apply',patch)
 archive=work/'6502-v0.1.tar.xz'
 if not archive.exists():urllib.request.urlretrieve('https://github.com/redcode/6502/releases/download/v0.1/6502-v0.1.tar.xz',archive)
 if hashlib.sha256(archive.read_bytes()).hexdigest()!='bba50f327163d40b1014f5affc8042137f37b200dae50a4f0fe56b5769b3940f':raise SystemExit('6502 checksum mismatch')
 with tarfile.open(archive) as t:t.extractall(src/'3rdparty',filter='data')
 subprocess.run(['patch','-p1','-i',str(src/'3rdparty/6502-illegal-opcode.patch')],cwd=src/'3rdparty/6502',check=True)
 import shutil
 shutil.copyfile(src/'3rdparty/6502/sources/6502.c',src/'src/6502.c')
 shutil.copyfile(src/'3rdparty/6502/API/emulation/CPU/6502.h',src/'src/6502.h')
 (src/'src/version.c').write_text('const char SVN_revision[]="native-amiga-openup-rc1";\n')
 stamp.write_text(digest)
print(work)
