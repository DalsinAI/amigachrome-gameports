import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("enigma_sdk", ROOT / "ports/enigma/configure_open3.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class EnigmaSDKTests(unittest.TestCase):
    def test_empty_target_sdk_does_not_find_host_libraries(self):
        with tempfile.TemporaryDirectory() as directory:
            result = MODULE.inventory(Path(directory))
            self.assertEqual(len(result["missing"]), sum(map(len, MODULE.REQUIRED.values())))
            self.assertTrue(all(not v for v in result["found"].values()))

    def test_split_target_prefixes_and_partial_dependency_report(self):
        with tempfile.TemporaryDirectory() as directory:
            sdk = Path(directory)
            for name, files in MODULE.REQUIRED.items():
                for relative in files:
                    root = sdk / "port-deps" if name == "zlib" else sdk
                    p = root / relative
                    p.parent.mkdir(parents=True, exist_ok=True)
                    p.write_bytes(b"target fixture")
            self.assertEqual(MODULE.inventory(sdk)["missing"], [])
            (sdk / "lib/libxerces-c.a").unlink()
            result = MODULE.inventory(sdk)
            self.assertEqual(result["missing"], ["Xerces-C: lib/libxerces-c.a"])
            self.assertIn("include/xercesc/util/PlatformUtils.hpp", result["found"]["Xerces-C"])
