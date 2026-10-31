#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
"""Build the pinned Enigma engine with existing OXP; no Xerces SDK is used."""
from pathlib import Path
import argparse
import io
import json
import os
import subprocess
import sys
import tarfile
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from build_sdl3_port import digest
from stage_toolchain import RELEASE, SHA256

REQUIRED = {
    'SDL2': ('lib/libSDL2.a', 'include/SDL2/SDL.h'),
    'SDL2_image': ('lib/libSDL2_image.a', 'include/SDL2/SDL_image.h'),
    'SDL2_ttf': ('lib/libSDL2_ttf.a', 'include/SDL2/SDL_ttf.h'),
    'SDL2_mixer': ('lib/libSDL2_mixer.a', 'include/SDL2/SDL_mixer.h'),
    'curl': ('lib/libcurl.a', 'include/curl/curl.h'),
    'nghttp2': ('lib/libnghttp2.a', 'include/nghttp2/nghttp2.h'),
    'zlib': ('lib/libz.a', 'include/zlib.h'),
}


def inventory(sdk, overlay=None):
    roots = ((overlay,) if overlay is not None else ()) + (sdk, sdk / 'port-deps')
    found, missing = {}, []
    for component, files in REQUIRED.items():
        entries = {}
        for name in files:
            path = next((r / name for r in roots if (r / name).is_file()), None)
            if path is None:
                missing.append(f'{component}: {name}')
            else:
                entries[name] = {'path': str(path), 'sha256': digest(path)}
        found[component] = entries
    return {'found': found, 'missing': missing}


def export(source, ref, destination):
    destination.mkdir()
    archive = subprocess.check_output(['git', '-C', str(source), 'archive', ref])
    with tarfile.open(fileobj=io.BytesIO(archive)) as tf:
        tf.extractall(destination, filter='data')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for arg in ['source', 'stove', 'sdk-overlay', 'oxp-source', 'out']:
        ap.add_argument('--' + arg, type=Path, required=True)
    ap.add_argument('--oxp-ref', default='HEAD', help='committed OXP revision to export')
    ap.add_argument('--jobs', type=int, default=4)
    ap.add_argument('--configure-only', action='store_true')
    a = ap.parse_args()
    source, stove, overlay, oxp, out = (p.resolve() for p in
                                      (a.source, a.stove, a.sdk_overlay, a.oxp_source, a.out))
    if out.exists() or a.jobs < 1:
        ap.error('Use a fresh output directory and a positive job count')
    compiler = json.loads((stove/'provenance.json').read_text())
    if compiler['release'] != RELEASE or compiler['archive_sha256'] != SHA256:
        ap.error('Stage the verified Open4 compiler with ports/enigma/stage_toolchain.py')
    if len(compiler['compiler_runtime_sha256']) < 100:
        ap.error('Incomplete compiler provenance')
    for relative, expected in compiler['compiler_runtime_sha256'].items():
        path=(stove/'prefix'/relative).resolve()
        if not path.is_relative_to((stove/'prefix').resolve()) or digest(path) != expected:
            ap.error('Compiler/runtime provenance mismatch: '+relative)
    sdk = stove / 'prefix/m68k-amigaos'
    dependencies = inventory(sdk, overlay)
    if dependencies['missing']:
        ap.error('Missing target SDK inputs: ' + ', '.join(dependencies['missing']))
    pin = json.loads((ROOT / 'gameports/catalog.json').read_text())['ports']['enigma']['source']['commit']
    subprocess.run(['git', '-C', str(source), 'cat-file', '-e', pin + '^{commit}'], check=True)
    oxp_pin = subprocess.check_output(['git', '-C', str(oxp), 'rev-parse', a.oxp_ref + '^{commit}'], text=True).strip()
    epoch = subprocess.check_output(['git', '-C', str(source), 'show', '-s', '--format=%ct', pin], text=True).strip()
    out.mkdir(parents=True)
    work = out / 'source'
    export(source, pin, work)
    oxp_work = out / 'oxp-source'
    export(oxp, oxp_pin, oxp_work)
    if not (oxp_work / 'oxp/src/oxp_schema.cpp').is_file():
        ap.error('Selected OXP commit does not contain the schema extension')
    patches = sorted((ROOT / 'ports/enigma/patches').glob('*.patch'))
    for patch in patches:
        with patch.open() as f:
            subprocess.run(['patch', '--batch', '--fuzz=0', '-p1'], cwd=work, stdin=f, check=True)
    host = out / 'host-tools'
    host.mkdir()
    (host / 'CMakeLists.txt').write_text(
        'cmake_minimum_required(VERSION 3.14)\nproject(EnigmaHostTools C)\n'
        f'add_subdirectory("{work}/lib-src/lua" lua)\n'
        f'add_executable(tolua "{work}/tools/tolua.c" "{work}/tools/toluabind.c")\n'
        'target_link_libraries(tolua lua m ${CMAKE_DL_LIBS})\n')
    env = dict(os.environ, SOURCE_DATE_EPOCH=epoch, PKG_CONFIG_PATH='', PKG_CONFIG_LIBDIR=str(sdk/'lib/pkgconfig'))
    # Native host tools must not inherit a target compiler environment.
    host_env = {k: v for k, v in env.items() if k not in {'CC', 'CXX', 'CFLAGS', 'CXXFLAGS', 'LDFLAGS', 'STOVE', 'AMIGA_STOVE'}}
    with (out / 'host-build.log').open('w') as log:
        subprocess.run(['cmake', '-S', str(host), '-B', str(host/'build')], env=host_env, stdout=log, stderr=subprocess.STDOUT, check=True)
        subprocess.run(['cmake', '--build', str(host/'build'), '-j', str(a.jobs)], env=host_env, stdout=log, stderr=subprocess.STDOUT, check=True)
    flags = (f'-m68040 -specs={oxp_work}/toolchain/m68k-soft-m68040.specs -mcrt=nix20 -pthread -fno-delete-null-pointer-checks '
             '-fno-tree-loop-distribute-patterns -fno-late-combine-instructions '
             f'-isystem "{overlay}/include" -ffile-prefix-map={out}=/enigma-build')
    command = ['cmake', '-S', str(work), '-B', str(out/'cmake'),
               f'-DCMAKE_TOOLCHAIN_FILE={ROOT}/ports/enigma/amigaos3.cmake',
               f'-DAMIGA_STOVE={stove}', f'-DAMIGA_SDK_OVERLAYS={overlay}',
               f'-DAMIGA_CPU_FLAGS=-m68040 -specs={oxp_work}/toolchain/m68k-soft-m68040.specs',
               '-DAMIGA_RUNTIME=-mcrt=nix20',
               f'-DOXP_SOURCE_DIR={oxp_work}/oxp', f'-DTOLUA_EXECUTABLE={host}/build/tolua',
               '-DCMAKE_BUILD_TYPE=Release', '-DHAVE_SDL=ON',
               f'-DCMAKE_C_FLAGS={flags}', f'-DCMAKE_CXX_FLAGS={flags}',
               '-DCMAKE_C_FLAGS_RELEASE=-O2 -DNDEBUG', '-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG',
               f'-DCMAKE_EXE_LINKER_FLAGS=-Wl,-Map,{out}/enigma.map']
    report = {'source_commit': pin, 'oxp_commit': oxp_pin,
              'compiler_release': compiler['release'], 'compiler_archive_sha256': compiler['archive_sha256'],
              'dependencies': dependencies, 'patches': {p.name: digest(p) for p in patches},
              'configure_command': command, 'state': 'attempting', 'runtime': 'not tested'}
    started = time.monotonic()
    try:
        with (out/'configure.log').open('w') as log:
            subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
        report['state'] = 'configured'
        if not a.configure_only:
            with (out/'build.log').open('w') as log:
                subprocess.run(['cmake', '--build', str(out/'cmake'), '--target', 'enigma', '-j', str(a.jobs)], env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
            binary = out/'cmake/src/enigma'
            symbols = subprocess.check_output([str(stove/'prefix/bin/m68k-amigaos-nm'), '-C', str(binary)], text=True)
            if 'xerces' in symbols.lower() or 'xerces' in (out/'enigma.map').read_text().lower():
                raise RuntimeError('Unexpected Xerces reference in final link')
            report.update(state='compiled', executable_sha256=digest(binary), xerces_symbols=False)
    finally:
        report['configure_build_seconds'] = round(time.monotonic() - started, 3)
        (out/'receipt.json').write_text(json.dumps(report, indent=2)+'\n')
    print(out/'receipt.json')


if __name__ == '__main__':
    main()
