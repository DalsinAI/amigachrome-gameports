"""Exercise source identity, partial preparation, reruns and drift rejection offline."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('acprepare', ROOT/'ports/assaultcube/prepare_source.py')
mod = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(mod)


class PreparationTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = Path(self.tmp.name)/'repo'
        self.source = self.repo/'source'
        self.source.mkdir(parents=True)
        self.file = self.source/'input.txt'
        self.file.write_text('original\n')
        self.git('init', '-q')
        self.git('add', '.')
        self.git('-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid', 'commit', '-qm', 'fixture')
        self.pin = self.git('rev-parse', 'HEAD').strip()
        self.patch = Path(self.tmp.name)/'change.patch'
        self.patch.write_text('diff --git a/source/input.txt b/source/input.txt\n'
                              '--- a/source/input.txt\n+++ b/source/input.txt\n'
                              '@@ -1 +1 @@\n-original\n+prepared\n')

    def git(self, *args):
        return subprocess.check_output(['git', '-C', str(self.repo), *args], text=True)

    def prepare(self, **kwargs):
        mod.prepare(self.source, pin=kwargs.pop('pin', self.pin), patches=[self.patch], **kwargs)

    def test_fresh_and_repeated_preparation(self):
        self.prepare()
        self.prepare()
        self.assertEqual(self.file.read_text(), 'prepared\n')

    def test_wrong_pin_refuses_before_mutation(self):
        with self.assertRaisesRegex(ValueError, 'revision'):
            self.prepare(pin='0'*40)
        self.assertEqual(self.file.read_text(), 'original\n')

    def test_partial_preparation_resumes(self):
        self.prepare()
        second = Path(self.tmp.name)/'second.patch'
        second.write_text(self.patch.read_text().replace('input.txt', 'other.txt'))
        (self.source/'other.txt').write_text('original\n')
        mod.prepare(self.source, pin=self.pin, patches=[self.patch, second])
        self.assertEqual((self.source/'other.txt').read_text(), 'prepared\n')

    def test_patch_drift_refuses(self):
        self.file.write_text('unexpected\n')
        with self.assertRaisesRegex(ValueError, 'does not match'):
            self.prepare()
        self.assertEqual(self.file.read_text(), 'unexpected\n')

    def test_wrong_source_directory_refuses(self):
        other = self.repo/'elsewhere'
        other.mkdir()
        with self.assertRaisesRegex(ValueError, 'source/'):
            mod.prepare(other, pin=self.pin, patches=[self.patch])
