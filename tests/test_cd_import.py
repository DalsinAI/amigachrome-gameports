import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import wave

ROOT=Path(__file__).resolve().parents[1]
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
cd=load('import_cd',ROOT/'scripts/import_cd.py')
assets=load('convert_assets',ROOT/'ports/lba2-classic-community/convert_assets.py')

class CDImportTests(unittest.TestCase):
    def test_iso_filesystem_extraction(self):
        def both(value,width):
            return value.to_bytes(width,'little')+value.to_bytes(width,'big')
        def record(name,lba,size,flags=0):
            out=bytearray(33+len(name)+(len(name)%2==0))
            out[0]=len(out);out[2:10]=both(lba,4);out[10:18]=both(size,4)
            out[25]=flags;out[28:32]=both(1,2);out[32]=len(name);out[33:33+len(name)]=name
            return out
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);image=bytearray(24*2352)
            pvd=bytearray(2048);pvd[:7]=b'\x01CD001\x01';pvd[40:72]=b'TEST'+b' '*28
            pvd[128:132]=both(2048,2);pvd[156:190]=record(b'\0',20,2048,2)
            directory=record(b'\0',20,2048,2)+record(b'\1',20,2048,2)+record(b'FILE.DAT;1',21,4)
            for lba,data in [(16,pvd),(20,directory),(21,b'data')]:
                image[lba*2352+16:lba*2352+16+len(data)]=data
            (root/'disc.bin').write_bytes(image)
            (root/'disc.cue').write_text('FILE "disc.bin" BINARY\n TRACK 01 MODE1/2352\n INDEX 01 00:00:00\n')
            result=cd.import_disc(root/'disc.cue',root/'out',wav=False)
            self.assertEqual((root/'out/data/FILE.DAT').read_bytes(),b'data')
            self.assertEqual(result['volume_label'],'TEST')
            self.assertEqual(len(result['derivatives']),1)

    def test_track_extents_and_pregap(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);(p/'disc.bin').write_bytes(bytes(2352*12))
            cue=p/'disc.cue';cue.write_text('FILE "disc.bin" BINARY\n TRACK 01 MODE1/2352\n INDEX 01 00:00:00\n TRACK 02 AUDIO\n INDEX 00 00:00:05\n INDEX 01 00:00:07\n PREGAP 00:00:02\n')
            _,tracks=cd.read_cue(cue)
            self.assertEqual((tracks[0]['end_sector'],tracks[1]['start_sector'],tracks[1]['end_sector']),(5,7,12))
            self.assertEqual(tracks[1]['pregap'],2)
    def test_reject_path_escape_and_unknown_flags(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'bad.cue'
            for cue in ['FILE "../disc.bin" BINARY\n','FILE "disc.bin" BINARY\n FLAGS PRE\n']:
                p.write_text(cue)
                with self.assertRaises(ValueError):cd.read_cue(p)
    def test_audio_big_endian_preserves_samples(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);pcm=struct.pack('>1176h',*([100,-200]*588))
            (p/'disc.bin').write_bytes(pcm)
            (p/'disc.cue').write_text('FILE "disc.bin" BINARY\n TRACK 01 AUDIO\n INDEX 01 00:00:00\n')
            result=cd.import_disc(p/'disc.cue',p/'out',extract=False,byte_order='big')
            with wave.open(str(p/'out/audio/track01.wav'),'rb') as w:
                self.assertEqual(w.getparams()[:3],(2,2,44100))
                self.assertEqual(w.readframes(588),struct.pack('<1176h',*([100,-200]*588)))
            self.assertEqual((p/'out/disc/disc.bin').read_bytes(),pcm)
            self.assertEqual(result['derivatives'][0]['frames'],588)
            with self.assertRaises(ValueError):cd.import_disc(p/'disc.cue',p/'out',False,False)
    def test_ambiguous_order_refused(self):
        with self.assertRaises(ValueError):cd.audio_order(b'\1\2'*10000,'auto')
    def test_animation_and_hqr_lz(self):
        animation=struct.pack('<4H',1,0,0,0)+struct.pack('<4h',4,-2,3,7)
        expected=struct.pack('>4H',1,0,0,0)+struct.pack('>4h',4,-2,3,7)
        self.assertEqual(assets.convert(animation,'anim'),expected)
        # Literal A, backreference three A's (method 2 gives minimum 3).
        hqr=struct.pack('<2I',8,21)+struct.pack('<IIH',4,3,2)+b'\1A\0\0'
        hqr=struct.pack('<2I',8,len(hqr))+hqr[8:]
        # Four compressed bytes, including the flags.
        hqr=hqr[:12]+struct.pack('<I',4)+hqr[16:]
        self.assertEqual(assets.decode_entry(hqr,0),b'AAAA')
    def test_bad_lz_and_bad_animation_refused(self):
        hqr=struct.pack('<2I',8,21)+struct.pack('<IIH',4,3,1)+b'\0\0\0'
        with self.assertRaises(ValueError):assets.decode_entry(hqr,0)
        with self.assertRaises(ValueError):assets.convert(struct.pack('<4H',20,20,0,0),'anim')
    def test_body_schema_preserves_tail(self):
        h=bytearray(96)
        struct.pack_into('<Ihh',h,0,0x12345678,96,0)
        # all eight sections empty, polygons and lines share an empty extent.
        struct.pack_into('<i',h,68,96);struct.pack_into('<i',h,76,96)
        original=bytes(h)+b'pixelbytes'
        native=assets.convert(original,'body')
        self.assertEqual(native[:4],b'\x12\x34\x56\x78')
        self.assertEqual(native[96:],b'pixelbytes')

if __name__=='__main__':unittest.main()
