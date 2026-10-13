"""Exercise CMake's actual stove selection without needing target binaries."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

@unittest.skipUnless(shutil.which('cmake'), 'CMake is required')
class StoveSelectionTests(unittest.TestCase):
    def select(self, *, stove=None, amiga=None, explicit=None):
        env = {k:v for k,v in os.environ.items() if k not in ('STOVE','AMIGA_STOVE')}
        env['HOME'] = '/test-home'
        if stove is not None: env['STOVE'] = stove
        if amiga is not None: env['AMIGA_STOVE'] = amiga
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'select.cmake'
            path.write_text('include("'+str(ROOT/'toolchain/amigaos3-opengpu.cmake')+'")\n'
                            'file(WRITE "'+str(Path(directory)/'result.txt')+'" '
                            '"${AMIGA_STOVE}\n${CMAKE_C_COMPILER}\n${CMAKE_CXX_COMPILER}\n")\n')
            cmd = ['cmake']
            if explicit is not None: cmd += ['-DAMIGA_STOVE='+explicit]
            subprocess.run(cmd+['-P',str(path)],env=env,check=True,capture_output=True,text=True)
            return (Path(directory)/'result.txt').read_text().splitlines()
    def check(self, selected, expected):
        self.assertEqual(selected,[expected,expected+'/prefix/bin/m68k-amigaos-gcc',
                                   expected+'/prefix/bin/m68k-amigaos-g++'])
    def test_stove_selects_compiler_and_headers_consistently(self):
        self.check(self.select(stove='/opt/open2'),'/opt/open2')
    def test_amiga_environment_alone_is_supported(self):
        self.check(self.select(amiga='/selected'),'/selected')
    def test_conflicting_environment_names_fail_closed(self):
        with self.assertRaises(subprocess.CalledProcessError) as error:
            self.select(stove='/old',amiga='/selected')
        self.assertIn('Conflicting STOVE',error.exception.stderr)
    def test_explicit_cache_overrides_both_environment_names(self):
        self.check(self.select(stove='/old',amiga='/other',explicit='/explicit'),'/explicit')
    def test_empty_amiga_environment_does_not_mask_stove(self):
        self.check(self.select(stove='/opt/open2',amiga=''),'/opt/open2')
    def test_legacy_default_is_unchanged(self):
        self.check(self.select(),'/test-home/AmigaChrome/stoves/os32-gcc16')
    def test_space_in_stove_path(self):
        self.check(self.select(stove='/opt/Open GCC 16 open2'),'/opt/Open GCC 16 open2')

if __name__ == '__main__': unittest.main()
