# Original startup movies

The owner's Planetary Pack contains both original startup movies:

| Profile | Asset | Video | Audio | Duration |
|---|---|---|---|---|
| Alpha Centauri | `movies/opening.wve` | EA TQI, 400×192, 2,381 frames, 15 fps | EA ADPCM, 22,050 Hz stereo, 3,500,070 sample frames | 158.733 seconds |
| Alien Crossfire | `movies/openingx.wve` | EA TQI, 592×288, 1,093 frames, 15 fps | EA ADPCM, 22,050 Hz stereo, 1,606,710 sample frames | 72.867 seconds |

Both streams decoded completely during private host recovery. Every decoded
YUV frame and PCM sample survived an independent cache readback/hash check.
Original files remain unchanged and private. No replacement movie was downloaded.

The original base startup routine at `0x53f6e0` chooses `opening`; expansion
routine `0x52aa30` chooses `openingx`, before sound effects initialization and
the main menu. Playback is gated by `DisableOpeningMovie` and the movie preference.
Thinker's `control_game` independently selects those same two names.
`logo.wve` and `open.wve` also exist, but are **not** prepended to startup on
the assumption that their filenames imply an original sequence.

## Installation and player route

There is no qualified native TQI decoder in the current SDK. The implemented
route explicitly decodes the owner's WVE once during installation using local
FFmpeg, then streams an uncompressed YUV420/S16LE cache on AmigaOS. It does not
encode a replacement video, alter frame rate, resize the frames or resample
the audio. It needs approximately 288 MB for base and 286 MB for expansion.
This storage cost is temporary technical debt until a native decoder is qualified.

```
python3 prepare_startup_movie.py /owner/movies/opening.wve /private/opening.amov
python3 prepare_startup_movie.py /owner/movies/openingx.wve /private/openingx.amov
CXX=/normal/private/sdk/bin/m68k-amigaos-g++ sh build_startup_movie.sh /private/alpha-startup-movie
```

`prepare_startup_movie.py` refuses to overwrite existing caches/receipts. It
records source, decoded-stream and cache hashes plus decoder version. It never
accesses the network or modifies the original movie. Installers should retain
both the original file and receipt and keep all owner data out of public Git.

The original EA ADPCM final packets declare partial 28-sample codec blocks
(1,470 declared samples occupying 1,484 padded samples). FFmpeg rejects these
packets. Recovery validates the complete EA chunk framing, exposes only that
last padded count in a private temporary copy, decodes all compressed data,
then trims the padding back to the exact original declared sample count.
The payload bytes remain unchanged. Simply ignoring FFmpeg errors would lose
the final audio packet and is not this recovery path.

The standalone native command is `alpha-startup-movie opening.amov`.
The integration API is `alpha_startup_movie(path)` and returns completed `0`,
user skipped `5`, or unavailable/invalid `20`. Compile the player with
`ALPHA_EMBEDDED_MOVIE` when linking into a frontend. Invoke it **before** opening
menu music; it rejects a competing initialized SDL audio subsystem. The menu
must remain usable if optional movie data is absent or the user skips playback.
It opens its own window/device, uses bounded frame/ring buffers and an audio
sample clock, then closes its own resources. Enter, Escape, Space, Q, left/right
mouse button or window close skip playback. Native screen sizing is currently
one decoded pixel per screen pixel. Original fullscreen scaling, colour
conversion and preference persistence are not claimed.

## Validation status

- Host ASan/UBSan: 22 bounded header/pixel controls; 49,152 independently
  interpreted BT.601 RGB output values; three malformed/tail controls pass.
- Both complete original clips: original chunks end exactly at EOF; all video
  frames and original declared audio samples decoded and cache-roundtrip hashes
  match. A naïve FFmpeg final-audio failure is retained in private evidence.
- Normal OpenAmigaGCC 16.2.0-open3 SDK builds the native player without toolchain
  overlays or replacement library stubs.
- Native playback/skip/PCM/normal exit and integration with the frontend are
  **pending shared-lab allocation**. Independent big-endian controls are wired
  into CI but must actually run before merge. Compilation is not runtime proof.

The public code and tests contain no original assets or proprietary decoder code.
The public presentation conversion is explicit BT.601 limited-range arithmetic;
exact equivalence to the original Windows player's displayed colours is not
asserted. The cache is decoded content, not native WVE playback.
