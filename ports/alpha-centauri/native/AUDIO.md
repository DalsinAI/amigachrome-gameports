# In-game audio (WP10)

The world view now has sound. One service mixes three things on the single
SDL audio device:

- event sound effects;
- voice-over;
- a music playlist.

Original media are read at run time from the game's data directory. No
original asset is in Git.

## Files

| File | Role |
|---|---|
| `audio_wav.*` | Portable RIFF/WAVE reader. Handles PCM8 and PCM16, mono and stereo (stereo is averaged to mono), IMA ADPCM and MS ADPCM. Also resamples linearly. Has no SDL dependency and does not allocate. |
| `audio_mixer.*` | Integer-only mixer, 22050 Hz mono S16. Has 6 effect slots (the oldest is stolen when all are busy), one voice channel and a music ring buffer. Gains ramp to avoid clicks. Music ducks under voice. A muted music bus pauses the stream. |
| `audio_music.*` | Streaming producer for music tracks. On AmigaOS it reads through dos.library; elsewhere through stdio. |
| `audio_events.*` | Event catalog and settings file. Also holds the original preference bits, the voice-over paths and the faction music sets. |
| `audio_service.*` | The SDL service: device, music feeder thread, effect cache (4 MB, least recently used out), voice, suspend and resume. |
| `audio_world.*` | Glue for the world view: entering and leaving, faction music, and the M/N/J keys. |
| `../tools/prepare_alpha_audio.py` | Owner-side preparation of a private media directory. |

## Device ownership

The native SDL backend cannot keep two devices open: see
`TECHNOLOGY-BROWSER.md`. The roles are:

- The frontend menu releases its device before it opens the world (unchanged).
- The world opens the service.
- The world's `T` key wraps the technology browser in `audio_suspend()` and
  `audio_resume()`. The music stream position, the playlist and the settings
  survive the suspend.
- Every call is a no-op when no device could be opened, so the game simply
  runs silent.

## Sound effects by event

Defaults name only fx files whose use is attested by an open upstream: GLSMAC
`0fe9ef16` data, file names only.

There is no attested original file name for the unattested events in the
table below. Neither the original executable's sound table nor an fx
directory listing was available here. Those events stay silent until the
owner maps them in `alpha-audio.cfg`.

`prepare_alpha_audio.py` gives the owner what is needed for that mapping. It:

- lists the local fx folder;
- lists every `.wav` string in `terranx.exe`;
- writes both lists into `alpha-audio.cfg.example`.

| Event | Default file | Source |
|---|---|---|
| `ui_click` | `ok.wav` | GLSMAC popup and base-screen buttons |
| `menu_open` / `menu_close` | `menu up.wav` / `menu down.wav` | GLSMAC popup |
| `panel_slide` | `menu out.wav` | GLSMAC sliding panel |
| `command_menu` | `mmenu.wav` | GLSMAC bottom bar |
| `quit_prompt` | `CPU please don't go.wav` | GLSMAC |
| `psi_attack` / `psi_destroyed` | `psi attack.wav` / `psi blow.wav` | GLSMAC unit animations |
| `turn_complete` | `cpu turn complete.wav` | GLSMAC turn-complete button |
| `unit_move`, `unit_attack`, `combat_hit`, `unit_destroyed`, `base_built`, `turn_start` | none | unattested |
| `facility_complete`, `tech_discovered` | none (voice-over below) | unattested |

Effects are looked up first as `fx/<name>`. If that fails, `<name>` is tried
in the data directory. Effects not at 22050 Hz are resampled once at load, so
the mixer can use its cheap path.

### Calls for the engine packages

The calls are plain functions in `audio_service.h`. Each engine package
should call them as follows:

- **WP3 (orders):** `audio_event(unit_move)` after a step that changes the tile.
- **WP4 (combat):**
  - `unit_attack` when combat starts;
  - `combat_hit` for each round lost;
  - `unit_destroyed` or `psi_destroyed` when a unit dies;
  - `psi_attack` for psi combat.
- **WP2 (bases):**
  - `base_built`;
  - `audio_facility_complete(id, first_announcement)` in the same place as
    Thinker's `base.cpp` facility popup.
- **WP5 (research):** `audio_tech_discovered(id, human)`.
- **WP1 (turns):** `turn_start` and `turn_complete`.
- **WP8 and interludes:**
  - `audio_interlude(id)` and `audio_interlude_closed()`;
  - `audio_popup_closed()`.

The world view already uses these calls:

- `turn_start` on entering;
- `menu_open` and `menu_close` for base details;
- `unit_move` for a local move;
- `ui_click` for a confirmed production order and for the audio keys.

## Voice-over

Recordings are the original MP3s, pre-converted by the owner to PCM16LE mono
22050 Hz WAV. The voice-over hooks are:

- **Faction speech:** `voices/<faction>.wav`, through the `leader_speech.cpp`
  catalog (Thinker `game.cpp`).
- **Technology discovery:**
  - The file is `voices/tech<ID>.wav`, for IDs 0 to 88.
  - ID 87, the disabled user technology, is excluded.
  - The source is `NewTechWin_exec`.
  - It plays for the human discoverer only.
- **Facility completion:**
  - The file is `voices/fac<N>[0].wav`.
  - Facilities 65 to 69 use `id-31`.
  - The expansion facilities 34 to 41 add a `0`.
  - Only ordinary facilities (ID below 70) are voiced.
  - It plays on the first announcement for the human player.
  - The source is Thinker `base.cpp`.
- **Interludes:** these are text-and-art popups. The hook fades the current
  voice over 2000 ms (as the original `Sound_fade_2` calls do) and holds music
  at the voice-duck level. No interlude recording is invented.
- **Popup close:** fades the voice when `voice_stops_on_close` is on. This is
  original `MPREF_AV_VOICEOVER_STOP_CLOSE_POPUP`.

**MP3 decoding:** there is no MP3 decoder on the target. The plan allowed
minimp3 (CC0) only after a 68040 CPU-budget measurement. That measurement was
not possible in this environment: building downloaded third-party code was
not permitted. So voice-over stays pre-converted, which matches the
technology browser and the leader speech audition.

## Music

The original game plays procedural AMB score sets, chosen per faction. The
choices below are from Thinker `game.cpp` `load_music`:

| Faction | Score set |
|---|---|
| Default | `aset1` |
| gaians | `gset1` |
| hive | `mset1` |
| univ | `uset2` |
| morgan | `mset1` |
| spartans | `sset1` |
| believe | `bset1` |
| peace | `gset1` |
| Expansion factions | `aset1` (the default) |

**The AMB sequencer is not implemented. This is the documented fallback.** The
reasons:

- No AMB file or AMB format description was available in this environment, so
  a parser could not be written or checked.
- The owner's recovery already found that 60 of the 220 sample paths the eight
  scores reference are absent from both the disc and the GOG listing
  (`TECHNOLOGY-BROWSER.md`).

Instead, the service plays owner-prepared PCM tracks for each set:

- `music/<set>.wav`
- `music/<set>-1.wav` to `music/<set>-9.wav`

The playlist order is:

1. the faction's own set;
2. otherwise `aset1`;
3. otherwise the first set that has any prepared tracks.

The playlist then cycles. Tracks stream from disk through a 1.5 s ring buffer
and are gapless when consecutive tracks share a rate. With no prepared tracks
the world stays silent. The opening-menu loop is still never used as a
substitute.

`prepare_alpha_audio.py --rendered-music DIR` converts the owner's own
recordings of each set to that format. Example recording names:
`gset1-1.flac`, `aset1.ogg`. The tool also lists every `.amb` file and how
many of its referenced samples exist. That list is the starting point for a
real sequencer.

## Settings

The settings file is `alpha-audio.cfg`, beside the executable. It holds
`key=value` lines; `#` and `;` start comments.

```
sound_effects=on        music=on        voices=on       voice_stops_on_close=on
master_volume=100       effects_volume=100      music_volume=70
voice_volume=100        music_under_voice=40
map.unit_move=<fx file name> | none | default
```

How settings are chosen and changed:

- **Bad file:** a bad line rejects the whole file, with its line number, and
  the defaults are used.
- **No file:** the save's original preference bits are used:
  - `PREF_AV_VOLUME_SFX_TOGGLE` 0x400;
  - `PREF_AV_VOLUME_MUSIC_TOGGLE` 0x800;
  - `MPREF_AV_VOLUME_VOICE_TOGGLE` 0x4000;
  - `MPREF_AV_VOICEOVER_STOP_CLOSE_POPUP` 0x20000.
- **World keys:** M toggles music, N toggles effects and J toggles voices. A
  changed setting is saved on leaving the world.
- **Mapped names:** a mapped name must be a plain `.wav` basename. Paths and
  drive names are refused.

## Owner preparation

```
python3 tools/prepare_alpha_audio.py ORIGINAL_DIR NEW_PRIVATE_DIR [--rendered-music DIR]
```

The tool needs local ffmpeg. Its handling of each kind of media:

- **fx:** PCM, IMA and MS ADPCM files are copied byte-identical. Other WAV
  codecs are converted.
- **voices:** every `voices/*.mp3` is strictly decoded to PCM16 mono 22050 Hz.
  Undecodable files are reported, not substituted.
- **Receipt:** hashes are written to `audio-receipt.json`.
- **Privacy:** nothing it produces may be committed.

## CPU cost (68040, measured under qemu)

`measure_audio_cost.py` builds with `m68k-linux-gnu-g++ -O2 -m68040`. It
counts the exact executed instructions per function from a `qemu-m68k -cpu
m68040` `in_asm`+`exec`+`nochain` trace.

qemu does not model 68040 timing, so cycles are an estimate. Each instruction
is weighted with 68040 manual figures:

- MUL: 20 cycles;
- DIV: 44 cycles;
- any other instruction: 1.5 cycles.

| Work | Instructions per audio second | Estimated share of a 25 MHz 68040 |
|---|---|---|
| IMA ADPCM decode (once, at load) | 876,696 | 5.3% |
| MS ADPCM decode (once, at load) | 870,952 | 11.9% |
| PCM16 decode | 110,264 | 0.7% |
| PCM8 decode | 154,363 | 0.9% |
| Mixer, typical (2 effects, voice, music at 70%) | 1,437,965 | 10.3% |
| Mixer, worst (3 effects resampled live, voice, ducked music) | 3,258,250 | 28.6% |

The decoders run once per clip at load, never in the audio callback. A 30 s
MS ADPCM voice line would take about 3.6 s to decode, so voices are prepared
as PCM. The live-resampling worst case cannot happen with the service as
written, because effects are resampled at load. It only applies to tracks or
clips whose rate the conversion could not change.

## Tests

| Command | Result |
|---|---|
| `python3 test_audio.py --out DIR` (host ASan+UBSan) | 129 checks pass. 6 ffmpeg-made fixtures (IMA and MS ADPCM, mono and stereo; u8 stereo 11025 Hz; s16 44100 Hz) decode bit-exactly against ffmpeg's own decode. |
| `python3 test_audio.py --out DIR --cc m68k-linux-gnu-g++ --m68k-linux` | 129 checks pass on big-endian 68040 under qemu, including the same 6 oracles. |
| `python3 test_audio_service.py --out DIR` (host ASan+UBSan, SDL disk driver) | 17 checks pass. |
| `python3 test_audio_prepare.py --out DIR` | Synthetic tree. Copy, convert and strict-reject cases pass; 6 prepared WAVs load in the native reader. |
| `python3 measure_audio_cost.py --out DIR` | Produces the table above. |

The service test checks that real output contains:

- the music;
- music plus click;
- voice plus ducked music.

It also checks suspend and resume, the missing-file count, the negative cache,
the interlude voice fade and saved settings.

## Not done

- AMB procedural music sequencer. The fallback above is in place instead.
- Original fx names for move, attack, hit, destroyed, base built and turn
  start. These need the owner's fx listing or the executable sound table. The
  hook points and the `map.*` settings are in place.
- Native MP3 decoding: the measurement was blocked in this environment.
- Native AmigaOS qualification. This means a full SDK build and an FS-UAE run
  that captures PCM. In this container the audio sources and the map window
  compile with the AmigaOS cross compiler only against OpenNDK headers and
  stand-in SDL2/CyberGraphX headers; nothing was linked.
- Callers in combat, base and research. Those UIs and engine paths do not
  exist yet; the calls are listed above.
