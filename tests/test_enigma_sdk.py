import importlib.util
from pathlib import Path
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('enigma_sdk', ROOT/'ports/enigma/configure_open3.py')
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)

class EnigmaSDKTests(unittest.TestCase):
    def test_empty_sdk_never_finds_host_libraries(self):
        with tempfile.TemporaryDirectory() as directory:
            result=MODULE.inventory(Path(directory))
            self.assertEqual(len(result['missing']),sum(map(len,MODULE.REQUIRED.values())))
            self.assertTrue(all(not v for v in result['found'].values()))
            self.assertNotIn('Xerces-C',MODULE.REQUIRED)

    def test_split_target_prefixes_and_partial_report(self):
        with tempfile.TemporaryDirectory() as directory:
            sdk=Path(directory)
            for name,files in MODULE.REQUIRED.items():
                for relative in files:
                    root=sdk/'port-deps' if name=='zlib' else sdk
                    p=root/relative;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'target fixture')
            self.assertEqual(MODULE.inventory(sdk)['missing'],[])
            (sdk/'lib/libnghttp2.a').unlink()
            result=MODULE.inventory(sdk)
            self.assertEqual(result['missing'],['nghttp2: lib/libnghttp2.a'])

    def test_overlay_curl_does_not_supply_missing_http2(self):
        with tempfile.TemporaryDirectory() as directory:
            sdk=Path(directory)/'sdk';overlay=Path(directory)/'overlay'
            for relative in MODULE.REQUIRED['curl']:
                p=overlay/relative;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'target curl fixture')
            result=MODULE.inventory(sdk,overlay)
            self.assertFalse(any(n.startswith('curl:') for n in result['missing']))
            self.assertIn('nghttp2: lib/libnghttp2.a',result['missing'])
            self.assertFalse(any('xerces' in n.lower() for n in result['missing']))
