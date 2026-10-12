"""No-network fixture tests for Open2 stove staging and runtime isolation."""
import hashlib
import importlib.util
import io
from pathlib import Path
import tarfile
import tempfile
import unittest
from unittest.mock import patch

SPEC=importlib.util.spec_from_file_location('stage_open2',Path(__file__).resolve().parents[1]/'scripts/stage_open2_stove.py')
module=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)

class StagingTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.root=Path(self.tmp.name)
        self.addCleanup(self.tmp.cleanup)
        self.ndk=self.root/'ndk'; self.sdk=self.root/'sdk'; self.dest=self.root/'new-stove'
        for name,data in (
            ('m68k-amigaos/ndk-include/proto/exec.h',b'new ndk'),):
            p=self.ndk/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        for name,data in (
            ('bin/sdl2-config',b'old sdk config'),
            ('m68k-amigaos/include/SDL2/SDL.h',b'sdk header'),
            ('m68k-amigaos/include/stdlib.h',b'old c header'),
            ('m68k-amigaos/lib/libSDL2.a',b'sdk SDL archive'),
            ('m68k-amigaos/lib/libc.a',b'wrong old libc'),
            ('m68k-amigaos/lib/libpthread.a',b'wrong old pthread')):
            p=self.sdk/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        self.archive=self.root/'open2.tar.xz'
        with tarfile.open(self.archive,'w:xz') as tf:
            for name,data,mode in (
                ('bin/m68k-amigaos-g++',b'#!/bin/sh\necho fixture-open2\n',0o755),
                ('m68k-amigaos/include/stdlib.h',b'new c header',0o644),
                ('m68k-amigaos/lib/libc.a',b'new libc',0o644),
                ('m68k-amigaos/lib/libpthread.a',b'new pthread',0o644),
                ('lib/gcc/m68k/16/libstdc++.a',b'new libstdc++',0o644)):
                info=tarfile.TarInfo(module.TOP+'/'+name);info.mode=mode;info.size=len(data)
                tf.addfile(info,io.BytesIO(data))
        self.hash=module.sha256(self.archive)
    def run_stage(self):
        with patch.object(module,'ARCHIVE_SHA256',self.hash):
            return module.stage(self.archive,self.ndk,self.sdk,self.dest)
    def test_sdk_overlay_never_replaces_compiler_runtime_or_headers(self):
        receipt=self.run_stage();p=self.dest/'prefix'
        self.assertEqual((p/'m68k-amigaos/lib/libc.a').read_bytes(),b'new libc')
        self.assertEqual((p/'m68k-amigaos/lib/libpthread.a').read_bytes(),b'new pthread')
        self.assertEqual((p/'m68k-amigaos/include/stdlib.h').read_bytes(),b'new c header')
        self.assertEqual((p/'m68k-amigaos/lib/libSDL2.a').read_bytes(),b'sdk SDL archive')
        self.assertEqual(receipt['runtime_state'],'not_tested')
        self.assertIn('not rebuilt',receipt['sdk_status'])
        self.assertTrue((self.dest/'provenance.json').exists())
    def test_wrong_checksum_refuses_before_populating_stove(self):
        with self.assertRaisesRegex(ValueError,'SHA-256'):
            module.stage(self.archive,self.ndk,self.sdk,self.dest)
        self.assertFalse(self.dest.exists())
    def test_existing_stove_cannot_be_overwritten(self):
        self.dest.mkdir();(self.dest/'keep').write_text('user data')
        with self.assertRaisesRegex(ValueError,'overwrite'):self.run_stage()
        self.assertEqual((self.dest/'keep').read_text(),'user data')
    def test_missing_ndk_is_explicit_and_non_destructive(self):
        (self.ndk/'m68k-amigaos/ndk-include/proto/exec.h').unlink()
        with self.assertRaisesRegex(ValueError,'NDK'):self.run_stage()
        self.assertFalse(self.dest.exists())
    def add_release_header(self, name, data):
        replacement=self.root/'replacement.tar.xz'
        with tarfile.open(self.archive,'r:xz') as old, tarfile.open(replacement,'w:xz') as new:
            for member in old.getmembers():
                new.addfile(member,old.extractfile(member) if member.isfile() else None)
            info=tarfile.TarInfo(module.TOP+'/'+name);info.size=len(data);info.mode=0o644
            new.addfile(info,io.BytesIO(data))
        replacement.replace(self.archive);self.hash=module.sha256(self.archive)
    def test_conflicting_release_ndk_header_refuses_without_installing(self):
        self.add_release_header('m68k-amigaos/ndk-include/proto/exec.h',b'release own ndk')
        with self.assertRaisesRegex(ValueError,'overwrite a release file'):self.run_stage()
        self.assertFalse(self.dest.exists())
    def test_identical_release_ndk_header_is_preserved_and_fingerprinted(self):
        self.add_release_header('m68k-amigaos/ndk-include/proto/exec.h',b'new ndk')
        receipt=self.run_stage()
        self.assertIn('m68k-amigaos/ndk-include/proto/exec.h',receipt['compiler_runtime_sha256'])
    def test_supplemental_ndk_headers_are_fingerprinted(self):
        receipt=self.run_stage()
        row=next(x for x in receipt['supplemental_files'] if x['destination']=='m68k-amigaos/ndk-include/proto/exec.h')
        self.assertEqual(row['sha256'],hashlib.sha256(b'new ndk').hexdigest())
    def test_missing_tar_safety_filter_is_explicit(self):
        with patch.object(module,'hasattr',return_value=False,create=True):
            with self.assertRaisesRegex(ValueError,'data_filter'):self.run_stage()
        self.assertFalse(self.dest.exists())
    def test_missing_sdl_archive_is_not_a_usable_stove(self):
        (self.sdk/'m68k-amigaos/lib/libSDL2.a').unlink()
        with self.assertRaisesRegex(ValueError,'SDL2'):self.run_stage()
        self.assertFalse(self.dest.exists())

if __name__=='__main__':unittest.main()
