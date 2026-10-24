import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("nx_import", ROOT / "ports/nxengine-evo/import_data.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class NXImportTests(unittest.TestCase):
    def fixture(self, root, nested=True, extra=None, omit=None):
        archive = root / "input.zip"
        engine = root / "engine"
        engine.mkdir()
        (engine / "sprites.sif").write_bytes(b"current engine sprites")
        (engine / "font.png").write_bytes(b"engine font")
        original = "data/data/" if nested else "data/"
        entries = {original + "npc.tbl": b"npc", original + "Loading.pbm": b"bitmap",
                   original + "Stage/Start.pxm": b"map", "data/tilekey.dat": b"tiles",
                   "data/sprites.sif": b"old sprites", "data/pxt/fx01.pxt": b"sound"}
        if omit:
            entries.pop(original + omit)
        entries.update(extra or {})
        with zipfile.ZipFile(archive, "w") as out:
            for name, data in entries.items():
                out.writestr(name, data)
        return archive, engine

    def test_nested_and_flat_data_and_engine_overlay(self):
        for nested in (True, False):
            with self.subTest(nested=nested), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                archive, engine = self.fixture(root, nested)
                before = hashlib.sha256(archive.read_bytes()).digest()
                receipt = MODULE.prepare(archive, engine, root / "out")
                data = root / "out/data"
                self.assertEqual((data / "npc.tbl").read_bytes(), b"npc")
                self.assertEqual((data / "Stage/Start.pxm").read_bytes(), b"map")
                self.assertEqual((data / "sprites.sif").read_bytes(), b"current engine sprites")
                self.assertEqual((data / "pxt/fx01.pxt").read_bytes(), b"sound")
                self.assertFalse((data / "data").exists())
                self.assertEqual(receipt["nested_original_data_flattened"], nested)
                self.assertEqual(hashlib.sha256(archive.read_bytes()).digest(), before)

    def test_music_aliases_require_exact_size_and_crc_and_preserve_original(self):
        data = b"synthetic org test data"
        files = {"org/old.org": ("org/old.org", data)}
        identities = {"new.org": (len(data), zlib.crc32(data)),
                      "wrong.org": (len(data), 0)}
        aliases = MODULE.add_music_aliases(files, identities)
        self.assertEqual(aliases, {"org/new.org": "org/old.org"})
        self.assertEqual(files["org/new.org"][1], data)
        self.assertNotIn("org/wrong.org", files)
        files["org/new.org"] = ("org/new.org", b"keep existing")
        self.assertEqual(MODULE.add_music_aliases(files, identities), {})
        self.assertEqual(files["org/new.org"][1], b"keep existing")

    def test_bad_archives_rejected_before_output(self):
        for extra in ({"data/../../escape": b"bad"}, {"data/PXT/FX01.PXT": b"conflict"},
                      {"data/other:escape": b"bad"}, {"data/pxt": b"collision"}):
            with self.subTest(extra=extra), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                archive, engine = self.fixture(root, extra=extra)
                with self.assertRaises(ValueError):
                    MODULE.prepare(archive, engine, root / "out")
                self.assertFalse((root / "out").exists())
                self.assertFalse((root / "escape").exists())

    def test_missing_required_assets_and_existing_output_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive, engine = self.fixture(root, omit="Loading.pbm")
            with self.assertRaisesRegex(ValueError, "Required"):
                MODULE.prepare(archive, engine, root / "out")
            self.assertFalse((root / "out").exists())
            (root / "out").mkdir()
            (root / "out/keep").write_text("original")
            with self.assertRaisesRegex(ValueError, "existing data"):
                MODULE.prepare(archive, engine, root / "out")
            self.assertEqual((root / "out/keep").read_text(), "original")

    def test_dangling_output_symlink_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive, engine = self.fixture(root)
            (root / "out").symlink_to(root / "outside")
            with self.assertRaisesRegex(ValueError, "existing"):
                MODULE.prepare(archive, engine, root / "out")
            self.assertFalse((root / "outside").exists())
