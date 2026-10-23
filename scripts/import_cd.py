#!/usr/bin/env python3
"""Import a supported single-BIN CUE without discarding its original track layout.

Copyright (c) 2026 Dalsin Limited. MIT licensed.
Extraction is a derivative, never a replacement for the disc image.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import tempfile
import wave

SECTOR = 2352


def sha256(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def frames(text):
    m, s, f = map(int, text.split(':'))
    if s >= 60 or f >= 75:
        raise ValueError('invalid CUE time')
    return (m * 60 + s) * 75 + f


def read_cue(path):
    """Strict supported subset; reject rather than silently lose CUE metadata."""
    tracks, files = [], []
    for line in Path(path).read_text(encoding='utf-8-sig').splitlines():
        words = line.strip().split(maxsplit=1)
        if not words or words[0].upper() in {'REM', 'TITLE', 'PERFORMER', 'CATALOG', 'ISRC'}:
            continue
        command = words[0].upper()
        if command == 'FILE':
            match = re.fullmatch(r'"([^"\r\n]+)"\s+BINARY', words[1], re.I)
            if not match or files:
                raise ValueError('only one FILE BINARY is supported; original was not modified')
            name = match[1]
            if Path(name).name != name or '\\' in name or name in {'.', '..'}:
                raise ValueError('CUE filename must be a sibling basename')
            files.append(Path(path).resolve().parent / name)
        elif command == 'TRACK':
            n, mode = words[1].upper().split()
            if not files or mode not in {'MODE1/2352', 'MODE2/2352', 'AUDIO'}:
                raise ValueError('unsupported track mode')
            number = int(n)
            if number != len(tracks) + 1:
                raise ValueError('tracks must be consecutive starting at 1')
            tracks.append({'number': number, 'mode': mode, 'indices': {}})
        elif command == 'INDEX':
            index, time = words[1].split()
            if not tracks or index not in {'00', '01'} or index in tracks[-1]['indices']:
                raise ValueError('unsupported or duplicate index')
            tracks[-1]['indices'][index] = frames(time)
        elif command in {'PREGAP', 'POSTGAP'}:
            if not tracks or command.lower() in tracks[-1]:
                raise ValueError('misplaced gap')
            tracks[-1][command.lower()] = frames(words[1])
        else:
            raise ValueError(f'unsupported CUE directive {command}')
    if not files or not tracks or any('01' not in t['indices'] for t in tracks):
        raise ValueError('incomplete CUE')
    binary = files[0]
    if binary.resolve() == Path(path).resolve():
        raise ValueError('CUE cannot name itself as the binary image')
    size = binary.stat().st_size
    if size % SECTOR:
        raise ValueError('BIN is not a whole number of 2352-byte sectors')
    total = size // SECTOR
    previous = -1
    for i, track in enumerate(tracks):
        start = track['indices']['01']
        begin = track['indices'].get('00', start)
        following = tracks[i+1]['indices'] if i+1 < len(tracks) else {}
        end = following.get('00', following.get('01', total))
        if not previous <= begin <= start < end <= total:
            raise ValueError('overlapping, reversed or empty track extents')
        previous = end
        track.update(start_sector=start, end_sector=end)
    return binary, tracks


class ISO:
    def __init__(self, binary, track):
        self.f = Path(binary).open('rb')
        self.start, self.end = track['start_sector'], track['end_sector']
        self.header = {'MODE1/2352': 16, 'MODE2/2352': 24}[track['mode']]
        pvd = self.read(16, 2048)
        if pvd[:7] != b'\x01CD001\x01':
            raise ValueError('track is not a supported ISO9660 primary volume')
        if self.both(pvd, 128, 2) != 2048:
            raise ValueError('unsupported logical block size')
        self.root = (self.both(pvd, 158, 4), self.both(pvd, 166, 4))
        self.label = pvd[40:72].decode('ascii').strip()

    @staticmethod
    def both(data, off, size):
        le = int.from_bytes(data[off:off+size], 'little')
        if le != int.from_bytes(data[off+size:off+size*2], 'big'):
            raise ValueError('ISO dual-endian fields disagree')
        return le

    def read(self, lba, size):
        if lba < 0 or size < 0 or size > 256 * 1024 * 1024 or lba + (size+2047)//2048 > self.end-self.start:
            raise ValueError('ISO extent outside data track or exceeds 256 MiB limit')
        out = bytearray()
        for sector in range((size+2047)//2048):
            self.f.seek((self.start+lba+sector)*SECTOR)
            raw = self.f.read(SECTOR)
            if len(raw) != SECTOR:
                raise ValueError('truncated sector')
            if self.header == 24 and (raw[18] & 0x20 or raw[16:20] != raw[20:24]):
                raise ValueError('MODE2 Form2 or mismatched XA subheaders cannot be extracted as 2048-byte data')
            out.extend(raw[self.header:self.header+2048])
        return bytes(out[:size])

    def files(self, lba=None, size=None, prefix=(), seen=None):
        if lba is None:
            lba, size = self.root
        seen = set() if seen is None else seen
        if lba in seen or len(prefix) > 16 or len(seen) > 4096:
            raise ValueError('cyclic or excessive ISO directory tree')
        seen.add(lba)
        directory = self.read(lba, size)
        pos = 0
        while pos < len(directory):
            length = directory[pos]
            if not length:
                pos = (pos//2048+1)*2048
                continue
            record = directory[pos:pos+length]
            pos += length
            if len(record) < 34 or len(record) != length or 33+record[32] > length:
                raise ValueError('invalid ISO directory record')
            name = record[33:33+record[32]]
            if name in {b'\0', b'\1'}:
                continue
            if record[25] & 0x80 or record[1] or record[26] or record[27]:
                raise ValueError('multi-extent, extended-attribute or interleaved ISO file unsupported')
            name = name.decode('ascii').split(';')[0]
            if name in {'', '.', '..'} or any(c in name for c in '/\\:'):
                raise ValueError('unsafe ISO name')
            extent = self.both(record, 2, 4), self.both(record, 10, 4)
            path = prefix+(name,)
            if record[25] & 2:
                yield from self.files(*extent, prefix=path, seen=seen)
            else:
                yield Path(*path), *extent


def audio_order(pcm, requested):
    if requested != 'auto':
        return requested, 'explicit'
    if len(pcm) < 16384:
        raise ValueError('short CD audio: select --audio-byte-order explicitly')
    # Conservative version of the engine's CDDA probe: require a decisive margin.
    samples = bytearray()
    stride = (len(pcm)-8192)//16
    for i in range(16):
        off = (stride*i)&~1
        samples.extend(pcm[off:off+8192])
    le = sum(abs(x[0]) for x in struct.iter_unpack('<h', samples))
    be = sum(abs(x[0]) for x in struct.iter_unpack('>h', samples))
    if le == be == 0:
        return 'little', 'digital silence; byte orders identical'
    if be*2 < le:
        return 'big', 'amplitude probe (heuristic; listening qualification required)'
    if le*2 < be:
        return 'little', 'amplitude probe (heuristic; listening qualification required)'
    raise ValueError('ambiguous CD audio byte order: select --audio-byte-order explicitly')


def import_disc(cue, out, extract=True, wav=True, byte_order='auto'):
    cue, out = Path(cue).resolve(), Path(out).resolve()
    binary, tracks = read_cue(cue)
    if out.exists():
        raise ValueError('output already exists; use a fresh import directory')
    out.parent.mkdir(parents=True, exist_ok=True)
    tmp = Path(tempfile.mkdtemp(prefix=out.name+'.', dir=out.parent))
    try:
        receipt = {'schema': 'amigachrome-cd-import', 'version': 1, 'originals': {},
                   'tracks': tracks, 'derivatives': [], 'qualification': 'not runtime-qualified',
                   'importer_source_sha256': sha256(__file__)}
        (tmp/'disc').mkdir()
        for source in (cue, binary):
            before = sha256(source)
            shutil.copyfile(source, tmp/'disc'/source.name)
            if sha256(tmp/'disc'/source.name) != before or sha256(source) != before:
                raise ValueError('disc changed while copying')
            receipt['originals'][source.name] = {'sha256': before, 'bytes': source.stat().st_size}
        # All derivatives use the verified copy, never a changing source.
        copied_bin = tmp/'disc'/binary.name
        used = set()
        if extract:
            data_tracks = [t for t in tracks if t['mode'] != 'AUDIO']
            if len(data_tracks) != 1:
                raise ValueError('filesystem extraction needs exactly one data track')
            iso = ISO(copied_bin, data_tracks[0])
            try:
                receipt['volume_label'] = iso.label
                for path, lba, size in iso.files():
                    if str(path).casefold() in used:
                        raise ValueError('case-colliding ISO paths')
                    used.add(str(path).casefold())
                    dest = tmp/'data'/path
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    dest.write_bytes(iso.read(lba, size))
                    receipt['derivatives'].append({'path':str(dest.relative_to(tmp)), 'bytes':size, 'sha256':sha256(dest)})
            finally:
                iso.f.close()
        if wav:
            with copied_bin.open('rb') as source:
                for track in tracks:
                    if track['mode'] != 'AUDIO':
                        continue
                    source.seek(track['start_sector']*SECTOR)
                    pcm = source.read((track['end_sector']-track['start_sector'])*SECTOR)
                    order, evidence = audio_order(pcm, byte_order)
                    if order == 'big':
                        swapped = bytearray(pcm)
                        swapped[0::2], swapped[1::2] = pcm[1::2], pcm[0::2]
                        pcm = swapped
                    dest = tmp/'audio'/f"track{track['number']:02d}.wav"
                    dest.parent.mkdir(exist_ok=True)
                    with wave.open(str(dest), 'wb') as w:
                        w.setparams((2, 2, 44100, 0, 'NONE', 'not compressed'))
                        w.writeframes(pcm)
                    receipt['derivatives'].append({'path':str(dest.relative_to(tmp)), 'sha256':sha256(dest),
                        'source_byte_order':order, 'byte_order_evidence':evidence, 'frames':len(pcm)//4,
                        'pcm_sha256':hashlib.sha256(pcm).hexdigest(), 'track':track['number']})
        (tmp/'import.json').write_text(json.dumps(receipt, indent=2)+'\n')
        if out.exists():
            raise ValueError('output appeared during import')
        os.rename(tmp, out)
        return receipt
    except BaseException:
        # Failed imports remain inspectable, with no completed import.json marker.
        marker = tmp/'import.json'
        if marker.exists():
            marker.rename(tmp/'incomplete-import.json')
        raise


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('cue', type=Path); p.add_argument('--out',type=Path,required=True)
    p.add_argument('--no-extract',action='store_true');p.add_argument('--no-wav',action='store_true')
    p.add_argument('--audio-byte-order',choices=['auto','little','big'],default='auto')
    a=p.parse_args()
    result=import_disc(a.cue,a.out,not a.no_extract,not a.no_wav,a.audio_byte_order)
    print(f"Imported {len(result['tracks'])} tracks, {len(result['derivatives'])} derivatives to {a.out}")

if __name__ == '__main__':
    main()
