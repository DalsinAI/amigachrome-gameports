#!/usr/bin/env python3
"""Build guest regression executables with the same ABI as the editor."""
import os
import subprocess
from pathlib import Path

base = Path(__file__).resolve().parent.parent
work = Path(os.environ.get('GRAFX2_WORK', base/'.work'))
src = work/'grafx2/src'
out = work/'target-tests'
out.mkdir(parents=True, exist_ok=True)
cc = str(Path(os.environ['OPENAMIGA_GCC'])/'bin/m68k-amigaos-gcc')
flags = ['-m68040','-m68881','-mcrt=nix20','-O2',
         '-fno-late-combine-instructions','-fno-delete-null-pointer-checks',
         '-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
         '-D__USE_NEW_TIMEVAL__','-DUSE_AMIGA_NATIVE','-DUSE_OPENUP',
         '-DNORECOIL','-DNOTTF=1','-D__no_pnglib__','-D__no_tifflib__',
         '-I'+os.environ['AMIGA_NDK'],'-I'+str(base/'sdk'),'-I'+str(src)]
targets = {
    'palette-target': [base/'tests/palette_target.c',src/'palette.c'],
    'openup-target': [base/'tests/openup_target.c'] + [src/(n+'.c') for n in
        ['amigaopenup','amigaexpand','amigafont','amigadatatype','zlibrary']],
    'workbench-target': [base/'tests/workbench_target.c'],
    'workbench-icon-target': [base/'tests/workbench_icon_target.c'],
    'make-icon': [base/'make_icon.c'],
    'input-agent': [base/'tests/input_agent.c'],
    'launch-editor': [base/'tests/launch_file.c'],
}
for name, files in targets.items():
    subprocess.run([cc,*flags,*map(str,files),str(src/'amigafpu.c'),'-lamiga','-lm','-o',str(out/name)],check=True)
print(out)
