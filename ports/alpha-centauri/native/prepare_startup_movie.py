#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Decode an owner's original EA/TQI movie into a lossless installation cache.

Requires a local FFmpeg with EA/TQI and EA ADPCM support. No network access,
replacement assets, video encoder, frame resizing or audio resampling is used.
The original WVE is retained. This is an explicit host installation dependency;
the native player does not pretend to decode TQI itself.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile


def digest(path):
    h = hashlib.sha256()
    with open(path, 'rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def read_exact(stream, size):
    value = bytearray()
    while len(value) < size:
        block = stream.read(size - len(value))
        if not block:
            break
        value.extend(block)
    return bytes(value)


def normalize_ea_tail(source, output):
    """Expose padded final ADPCM block to FFmpeg, then trim its padding.

    Original stereo EA packet: 12-byte predictor header then 30 bytes per
    28 stereo samples. Original movies declare a partial final block, whereas
    FFmpeg requires a multiple of 28. Only the final count field is changed
    in a PRIVATE temporary copy; every compressed payload byte is preserved.
    """
    samples = 0
    partial_seen = False
    frames = 0
    corrections = []
    with open(source, 'rb') as stream, open(output, 'wb') as staged:
        while True:
            header = stream.read(8)
            if not header:
                break
            if len(header) != 8:
                raise ValueError('Incomplete EA chunk header')
            tag, length = struct.unpack('<4sI', header)
            if tag not in (b'SCHl', b'SCCl', b'SCDl', b'pIQT', b'SCEl') or not (8 <= length <= 8 * 1024 * 1024):
                raise ValueError('Unsupported EA chunk')
            body = bytearray(read_exact(stream, length - 8))
            if len(body) != length - 8:
                raise ValueError('Truncated EA chunk')
            if tag == b'pIQT':
                frames += 1
            if tag == b'SCDl':
                if partial_seen or len(body) < 12 or (len(body) - 12) % 30:
                    raise ValueError('Unsupported EA stereo block framing')
                declared = struct.unpack_from('<I', body)[0]
                padded = ((declared + 27) // 28) * 28
                if not declared or padded != (len(body) - 12) // 30 * 28:
                    raise ValueError('Audio block payload/count mismatch')
                samples += declared
                if declared % 28:
                    partial_seen = True
                    struct.pack_into('<I', body, 0, padded)
                    corrections.append(dict(declared=declared, padded=padded))
            staged.write(header)
            staged.write(body)
    if not frames or not samples or not partial_seen:
        raise ValueError('Unrecognised original startup movie profile')
    return frames, samples, corrections


def prepare(source, target, ffmpeg='ffmpeg', ffprobe='ffprobe'):
    source, target = Path(source).resolve(), Path(target).resolve()
    if target.exists() or target.with_suffix(target.suffix + '.json').exists():
        raise ValueError('Refusing to replace an existing cache or receipt')
    probe = json.loads(subprocess.check_output([ffprobe, '-v', 'error', '-show_streams', '-of', 'json', str(source)]))
    video = [s for s in probe['streams'] if s['codec_type'] == 'video']
    audio = [s for s in probe['streams'] if s['codec_type'] == 'audio']
    if len(video) != 1 or len(audio) != 1:
        raise ValueError('Exactly one video and one audio stream required')
    v, a = video[0], audio[0]
    width, height = v['width'], v['height']
    if v['codec_name'] != 'tqi' or v['pix_fmt'] != 'yuv420p' or v['r_frame_rate'] != '15/1' or width % 2 or height % 2 or not (2 <= width <= 640 and 2 <= height <= 480):
        raise ValueError('Unsupported video profile')
    if a['codec_name'] != 'adpcm_ea' or a['sample_rate'] != '22050' or a['channels'] != 2:
        raise ValueError('Unsupported audio profile')
    target.parent.mkdir(parents=True, exist_ok=True)
    original_hash = digest(source)
    with tempfile.TemporaryDirectory(prefix='.alpha-movie-', dir=target.parent) as temporary:
        normalized = Path(temporary) / 'decoder-input.wve'
        declared_frames, declared_samples, corrections = normalize_ea_tail(source, normalized)
        pcm = Path(temporary) / 'audio.s16le'
        subprocess.run([ffmpeg, '-nostdin', '-v', 'error', '-xerror', '-threads', '1', '-i', str(normalized), '-map', '0:a:0', '-af', f'atrim=end_sample={declared_samples}', '-c:a', 'pcm_s16le', '-f', 's16le', str(pcm)], check=True)
        if pcm.stat().st_size % 4:
            raise ValueError('Incomplete stereo audio frame')
        samples = pcm.stat().st_size // 4
        if samples != declared_samples:
            raise ValueError('Decoded audio count differs from original declarations')
        staged = Path(temporary) / 'movie.amov'
        command = [ffmpeg, '-nostdin', '-v', 'error', '-xerror', '-threads', '1', '-i', str(source), '-map', '0:v:0', '-fps_mode', 'passthrough', '-c:v', 'rawvideo', '-pix_fmt', 'yuv420p', '-f', 'rawvideo', '-']
        decoder = subprocess.Popen(command, stdout=subprocess.PIPE)
        frames = 0
        video_hash, audio_hash = hashlib.sha256(), hashlib.sha256()
        try:
            with open(staged, 'wb') as output, open(pcm, 'rb') as sound:
                output.write(bytes(40))
                while True:
                    image = read_exact(decoder.stdout, width * height * 3 // 2)
                    if not image:
                        break
                    if len(image) != width * height * 3 // 2 or frames >= 18000:
                        raise ValueError('Truncated or excessive video')
                    wave = sound.read((2940 if frames + 1 == declared_frames else 1470) * 4)
                    output.write(image)
                    output.write(wave)
                    video_hash.update(image)
                    audio_hash.update(wave)
                    frames += 1
                if sound.read(1) or frames != declared_frames or not (abs(frames * 1470 - samples) < 1470):
                    raise ValueError('Unsupported audio/video tail difference')
                if decoder.wait() != 0:
                    raise ValueError('Video decoding failed')
                output.seek(0)
                output.write(b'AMOVYUV1' + struct.pack('<8I', width, height, frames, 22050, 2, 1470, samples, 0))
            if digest(source) != original_hash:
                raise ValueError('Original changed during conversion')
            # Hard-link publication is exclusive: a racing installer cannot be overwritten.
            os.link(staged, target)
        finally:
            decoder.stdout.close()
            if decoder.poll() is None:
                decoder.kill()
                decoder.wait()
    receipt = dict(format='AMOVYUV1', source_name=source.name, source_sha256=original_hash,
                   width=width, height=height, frames=frames, video_seconds=frames / 15,
                   audio_frames=samples, audio_seconds=samples / 22050,
                   video_yuv420_sha256=video_hash.hexdigest(), audio_s16le_sha256=audio_hash.hexdigest(),
                   cache_bytes=target.stat().st_size, cache_sha256=digest(target),
                   private_decoder_tail_normalization=corrections,
                   ffmpeg=subprocess.check_output([ffmpeg, '-version'], text=True).splitlines()[0],
                   limits='Host FFmpeg decode cache; no native TQI decoder or original display-colour equivalence claimed.')
    with open(str(target) + '.json', 'x') as output:
        json.dump(receipt, output, indent=2)
        output.write('\n')
    return receipt


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original_wve')
    parser.add_argument('cache_amov')
    args = parser.parse_args()
    print(json.dumps(prepare(args.original_wve, args.cache_amov), indent=2))
