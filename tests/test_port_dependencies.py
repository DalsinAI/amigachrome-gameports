"""Fixture tests for pinned codec inputs, fail-closed recipes and relocatability."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('portdeps', ROOT/'scripts/port_dependencies.py')
mod = importlib.util.module_from_spec(SPEC); SPEC.loader.exec_module(mod)


class DependencyTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name); self.source = self.root/'donor'; self.stove = self.root/'stove'
        self.stove.mkdir(); self.lock = self.root/'deps.json'; entries = []
        for group, names in mod.GROUPS.items():
            for name in sorted(names):
                p = self.source/name; p.parent.mkdir(parents=True, exist_ok=True)
                p.write_bytes(('fixture '+name).encode())
                entries.append({'group':group, 'path':name, 'sha256':mod.digest(p)})
        self.lock.write_text(json.dumps({'schema':1, 'status':'reused_not_rebuilt', 'files':entries}))
    def stage(self):
        return mod.stage(self.stove, self.source, self.lock)
    def verify(self):
        return mod.verify(self.stove, ['zlib','png','jpeg','freetype'], self.lock)
    def test_stage_and_verify_without_source_tree(self):
        dest = self.stage(); shutil.rmtree(self.source)
        self.assertEqual(self.verify(), dest)
        self.assertEqual(json.loads((dest/'manifest.json').read_text())['status'], 'reused_not_rebuilt')
    def test_bad_source_hash_refuses_before_destination_exists(self):
        (self.source/'include/zlib.h').write_text('changed')
        with self.assertRaisesRegex(ValueError, 'SHA-256'): self.stage()
        self.assertFalse((self.stove/mod.RELATIVE_DEST).exists())
    def test_missing_source_archive_refuses(self):
        (self.source/'lib/libz.a').unlink()
        with self.assertRaisesRegex(ValueError, 'Missing'): self.stage()
    def test_existing_destination_is_not_overwritten(self):
        self.stage()
        with self.assertRaisesRegex(ValueError, 'overwrite'): self.stage()
    def test_changed_staged_header_refuses(self):
        dest=self.stage(); (dest/'include/png.h').write_text('wrong header')
        with self.assertRaisesRegex(ValueError, 'SHA-256'): self.verify()
    def test_changed_staged_library_refuses(self):
        dest=self.stage(); (dest/'lib/libz.a').write_text('wrong lib')
        with self.assertRaisesRegex(ValueError, 'SHA-256'): self.verify()
    def test_injected_runtime_header_refuses(self):
        dest=self.stage(); (dest/'include/stdio.h').write_text('foreign stdio')
        with self.assertRaisesRegex(ValueError, 'Unrecorded'): self.verify()
    def test_staged_header_symlink_refuses(self):
        dest=self.stage(); p=dest/'include/zlib.h'; p.unlink(); p.symlink_to(self.source/'include/zlib.h')
        with self.assertRaisesRegex(ValueError, 'Symlink'): self.verify()
    def test_parent_symlink_cannot_escape_stove(self):
        target=self.root/'outside'; target.mkdir(); (self.stove/'prefix').symlink_to(target, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, 'escapes'): self.stage()
    def test_missing_manifest_refuses(self):
        dest=self.stage(); (dest/'manifest.json').unlink()
        with self.assertRaisesRegex(ValueError, 'Missing'): self.verify()
    def test_modified_manifest_is_not_a_new_authority(self):
        dest=self.stage(); p=dest/'manifest.json'; data=json.loads(p.read_text()); data['files']=[]; p.write_text(json.dumps(data))
        with self.assertRaisesRegex(ValueError, 'manifest'): self.verify()
    def test_dependency_lock_cannot_admit_a_runtime_header(self):
        data=json.loads(self.lock.read_text()); data['files'][0]['path']='include/stdio.h'; self.lock.write_text(json.dumps(data))
        with self.assertRaisesRegex(ValueError, 'allow-list'): self.stage()
    def test_relocated_stove_verifies_without_original_source(self):
        self.stage(); shutil.rmtree(self.source)
        new=self.root/'moved with spaces'; self.stove.rename(new); self.stove=new
        self.verify()
    def test_missing_dependencies_block_all_five_recipes_before_build(self):
        env=os.environ.copy();env['STOVE']=str(self.stove)
        env.pop('AMIGA_STOVE',None);env.pop('ZLIB_ROOT',None);env.pop('UQM_DEPS',None)
        for name in ('assaultcube/build-amiga-client.sh','assaultcube/build-amiga-server.sh',
                     'uqm/build-amigaos3.sh','nxengine-evo/build-amigaos3.sh','astromenace/build-amigaos3.sh'):
            with self.subTest(recipe=name):
                done=subprocess.run(['sh',str(ROOT/'ports'/name)],env=env,capture_output=True,text=True)
                self.assertNotEqual(done.returncode,0)
                self.assertIn('DEPENDENCY_GATE_FAILED',done.stderr)
    def test_recipes_do_not_reference_workstation_donor(self):
        for name in ('assaultcube/build-amiga-client.sh','assaultcube/build-amiga-server.sh',
                     'uqm/build-amigaos3.sh','nxengine-evo/build-amigaos3.sh','astromenace/build-amigaos3.sh'):
            source=(ROOT/'ports'/name).read_text()
            self.assertNotIn('webkit-os32',source)
            self.assertNotIn('FALLBACK',source)
            self.assertIn('port_dependencies.py" verify --stove',source)


if __name__ == '__main__': unittest.main()
