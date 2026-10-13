"""Check Git's actual EOL behaviour using repository attributes and LF fixtures."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]

@unittest.skipUnless(shutil.which('git'), 'git required')
class EvidenceCheckoutTests(unittest.TestCase):
    def test_autocrlf_checkout_preserves_receipt_bytes(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            def git(*args):
                return subprocess.run(['git','-C',str(root),*args],check=True,capture_output=True)
            git('init','-q')
            git('config','core.autocrlf','false')
            shutil.copy2(ROOT/'.gitattributes',root/'.gitattributes')
            names=('evidence/open2-20261012/proof.txt',
                   'evidence/sdltext-20261012-recovered/text-static-checks.json',
                   'toolchain/port-dependencies.lock.json')
            original=b'first line\nsecond line\n'
            for name in names:
                p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(original)
            git('add','.')
            git('config','core.autocrlf','true')
            for name in names:(root/name).unlink()
            git('checkout-index','-a','-f')
            for name in names:
                self.assertEqual((root/name).read_bytes(),original,name)

if __name__=='__main__':unittest.main()
