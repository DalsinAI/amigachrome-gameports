#!/usr/bin/env python3
"""Experimental pinned Civ II wasm2c -> native 68040 build, without game data."""
import argparse
import concurrent.futures
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys

PIN = 'e3c40da88689ed79903debf4bff436bd140b79fa'
HERE = pathlib.Path(__file__).resolve().parent

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=pathlib.Path)
    parser.add_argument('--stove', required=True, type=pathlib.Path)
    parser.add_argument('--output', required=True, type=pathlib.Path)
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--skip-generation', action='store_true')
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    source, stove, output = args.source.resolve(), args.stove.resolve(), args.output.resolve()
    if args.jobs < 1 or args.jobs > 8:
        parser.error('--jobs must be between 1 and 8')
    if output == source or source in output.parents or output in source.parents:
        parser.error('output must be separate from the source checkout')
    revision = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    if revision != PIN:
        parser.error('source revision does not match reviewed pin')
    cc = stove/'bin/m68k-amigaos-gcc'
    version = subprocess.check_output([str(cc), '--version'], text=True).splitlines()[0]
    if '16.2.0-open3' not in version:
        parser.error('this prototype requires the qualified GCC 16.2.0-open3 stove')
    if not args.skip_generation:
        # The upstream generator otherwise uses every build-machine CPU.
        driver = ('import os,runpy,sys; os.cpu_count=lambda: int(sys.argv[1]); '
                  'sys.argv=["tools/build_port.py","module","c"]; '
                  'runpy.run_path("tools/build_port.py",run_name="__main__")')
        subprocess.run([sys.executable, '-c', driver, str(args.jobs)], cwd=source, check=True)
    generated = source/'build/wasm/c'
    if not (generated/'civ2_wasm-impl.h').is_file():
        parser.error('generated C missing: install upstream-pinned WASI SDK 25 and WABT 1.0.41 first')
    output.mkdir(parents=True, exist_ok=True)
    for path in generated.iterdir():
        if path.is_file():
            shutil.copy2(path, output/path.name)
    shutil.copytree(source/'src/host', output/'host', dirs_exist_ok=True)
    (output/'platform').mkdir(exist_ok=True)
    shutil.copy2(source/'src/platform/win32_vk.h', output/'platform/win32_vk.h')
    for name in ['prepare_forward_memory.py', 'prepare_civ2_host.py', 'finish_amiga_host.py',
                 'fix_amiga_environment.py', 'add_native_launch_options.py']:
        subprocess.run([sys.executable, str(HERE/'compat'/name), str(output)], check=True)
    if args.prepare_only:
        return
    objects = output/'obj'
    objects.mkdir(exist_ok=True)
    flags = ['-std=c99', '-O1', '-m68040', '-m68881', '-mcrt=nix20',
             '-fno-strict-aliasing', '-DWABT_BIG_ENDIAN=0', '-DCIV2_WASM_SINGLE_THREAD=1',
             '-DWASM_RT_USE_MMAP=0', '-DWASM_RT_MEMCHECK_BOUNDS_CHECK=1',
             '-I'+str(output), '-I'+str(output/'host'),
             '-I'+str(stove/'m68k-amigaos/include/SDL2')]
    sources = sorted(output.glob('*.c')) + [output/'host'/name for name in
                                         ['host_sdl.c', 'port_main.c', 'wasi.c']]
    def compile_one(path):
        obj = objects/(path.name+'.o')
        result = subprocess.run([str(cc), *flags, '-c', str(path), '-o', str(obj)],
                                capture_output=True, text=True)
        (objects/(path.name+'.log')).write_text(result.stdout+result.stderr)
        print(path.name, result.returncode, flush=True)
        if result.returncode:
            raise RuntimeError('Compilation failed; see '+str(objects/(path.name+'.log')))
        return str(obj)
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        compiled = list(pool.map(compile_one, sources))
    binary = output/'OpenCiv2'
    subprocess.run([str(cc), '-m68040', '-m68881', '-mcrt=nix20', *compiled,
                    '-lSDL2', '-lm', '-o', str(binary)], check=True)
    receipt = {'source': PIN, 'compiler': version, 'cpu': '68040+FPU',
               'sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
               'bytes': binary.stat().st_size, 'flags': flags,
               'qualification': 'experimental build; gameplay and clean exit not qualified'}
    (output/'build-receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(receipt, indent=2))

if __name__ == '__main__':
    main()
