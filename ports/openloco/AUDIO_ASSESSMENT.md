# OpenLoco audio: reuse assessment

Assessment date: 23 October 2026. Scope: OpenLoco
`6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e`, OpenRTG
`0c98fe780e6531946be44bebbb3a9db0c7bb7ab2`, SDL 3.4.14 and SDL2_mixer
2.8.2. Platform changes reported separately by Team 4 are not presumed shipped
or qualified here.

## Recommendation

Use the existing native OpenAHI service as the shared audio foundation. Put a
reusable OpenAL-compatible module above AHI, so OpenLoco retains its OpenAL
calls and later OpenAL games reuse the same implementation. Native Amiga
applications continue using AHI. SDL2 and SDL3 remain clients of AHI through
their existing drivers; neither SDL becomes the dependency of the native
service.

The earlier game-specific SDL3 proposal is not selected. Although feasible,
it would replace one game's backend rather than provide the durable Amiga
platform capability requested. No replacement backend or platform module is
implemented by this assessment.

The native implementation is in amigachrome's tracked vendor tree, not the
current standalone amigachrome-guest tree. Verified source:
`DalsinAI/amigachrome` at `476d01e07c373b12c7d0d2d67c932b6d75e9b3ec`,
`vendor/amigachrome-guest/devices/ahi.device` and `devices/acaudio.audio`.

### What already exists natively

OpenAHI is the Team's MIT-licensed `ahi.device`. Its public low-level AHI
interface already supplies loaded sounds, channels, sound selection,
frequency/pitch, volume/pan, looping/end hooks and channel-position reporting.
`ahi_mixer.c` supplies the software mixing path. The ACAHI Host mix mode
hands per-voice sample buffers, pitch, volume and pan to the board/runtime
rather than making SDL mix a single stream on the 68040.

The inspected ACAHI protocol offers 32 voices and 1024 sound slots, and its
current driver permits one low-level audio allocation at a time. The current
software mixer uses 16-bit linear interpolation. Echo/reverb and other DSP
support are incomplete. Historical repository reports say native AHI checks
and voice demos passed; this assessment does not independently repeat those
runtime tests.

### Work to do once in the common layer

1. Keep the documented AHI interface as the stable foundation. Build a real
   OpenAL/ALC adapter for buffer/source lifetime, contexts, queries, errors,
   mono/stereo formats, playback state, looped and queued buffers, volume,
   pitch and spatial parameters. Do not label a game-specific symbol shim as
   OpenAL 1.1 conformance. Advertise optional extensions only when implemented.
2. Resolve shared ownership and arbitration of the native channels. A
   reusable multi-client module needs a shared owner/broker rather than each
   client's private state separately claiming the one allocation. Negotiate
   voice limits, prevent one client from stopping another, isolate contexts,
   and reclaim resources on orderly close and client failure.
3. Define per-voice pause/resume, offsets, completion and underrun behaviour
   against the standard API. A unit-wide stop pauses other requests and is not
   a substitute for pausing one OpenAL source. Preserve cursor and loop state.
4. Keep host mixing and software fallback behind the same public contract;
   report available resources and effects truthfully. Map spatial/Doppler
   calculations to native voice frequency/gain/pan where correct, with tests.
   Native AHI panning is not automatically identical to OpenAL spatialization.
5. Close effects, resampling fidelity, click prevention and concurrency gaps
   in the common service where needed. Add capability queries/versioning for
   extensions; do not require per-game knowledge of board registers.

Use Exec ownership/signals and keep AHI/device calls on CPU0. OpenMulticore
jobs may perform compute on explicitly granted buffers but must not call AHI
or other AmigaOS services. ACAHI host-mixed voices are a separate existing
offload path, not evidence that every effect is an OpenMulticore job.

For the game port, the work is then normal OpenAL client linkage and format/
resource qualification. Improving the common adapter benefits future games.
No AHI, SDL3 or platform repository is changed here; this is the recommendation
for the platform owners.

## What the existing mixer supplies

`SDL2_mixer.module` is real, with a client archive, per-program SDL jump table,
and module callback/A4 handling. Its upstream mixer and API are Zlib licensed;
the Team's module wrapper is MIT. Reusing upstream code requires retaining its
own notices rather than describing the whole result as newly written MIT code.

| Requirement | SDL2_mixer 2.8.2 | SDL3 core / remaining adapter |
| --- | --- | --- |
| Concurrent sources, volume | Channels and volume API; volume uses a 0–128 scale | Multiple bound streams, floating-point per-stream gain |
| Runtime pitch / playback rate | No public per-channel pitch API | `SDL_SetAudioStreamFrequencyRatio`; matches the required speed-and-pitch operation |
| Stereo panning | `Mix_SetPanning` | Needs per-source left/right gain handling; channel remapping alone is not panning |
| Play, loop, pause, stop, completion | Channel APIs | Adapter must manage cursors, loop refill, queued data, pause and completion |
| Reverb | No matching built-in OpenAL EFX reverb | Separate optional effect; do not claim parity before implementing and testing |
| SDL3.module integration | Existing binary calls SDL2's jump table | Already native to SDL3.module; existing callout patches cover audio callbacks |
| AHI output | Through SDL2's audio driver | Through SDL3's existing Amiga AHI driver |

The SDL2 mixer advances each channel by the mixed byte count. Its effect
callbacks process buffers of that length; registering an effect does not by
itself provide a variable source cursor or a correct pitch-changing channel.
Extracting that core would still need pitch work, output adaptation and a new
module integration. Linking the existing binary would introduce an SDL2 audio
dependency alongside this SDL3 game. Neither supplies the native reusable service selected here.

## OpenLoco's actual requirements

- Buffers contain decoded PCM: 8/16-bit, mono/stereo, with a sample rate.
  The pinned music loader also reads whole PCM WAV tracks into buffers.
- Sources use play/stop/pause/resume, looping, per-source volume and pan,
  changing pitch, and logical category/master volume. Pitch is expressed as
  frequency divided by 22000, with the existing near-zero default preserved.
- The implementation uses source-relative positioning with rolloff disabled
  for its pan calculation. There are no AL streaming-queue, listener, velocity
  or Doppler calls in this backend. Those are requirements for a general
  OpenAL library, not prerequisites for this port.
- Reverb is already conditional on EFX availability upstream. An unavailable
  effect must remain explicit; a no-op is not a reverb qualification pass.
- The backend requests 1024 mono and 32 stereo sources but uses the capacity
  returned by the implementation. Those requested numbers are not a measured
  simultaneous-voice workload or a safe AC090 performance target.

Preserve the existing pan/gain behaviour using reference comparisons. Keep a
shared physical output path; pausing one voice must not pause the shared
device. Use bounded refill buffers and retain immutable sample data instead
of duplicating complete music tracks for each voice.

## AHI and multicore boundaries

The inspected SDL3 Amiga driver opens `ahi.device` using `OpenDevice`; game
code does not need a new `ahi.library` loading stub. Its native output supports
8/16-bit mono/stereo. SDL3 mixes multiple streams in floating point before
converting to the output format. Measure that cost on 68040/AC090.

The existing SDL2 satellite's `media.decode/1` path offloads compressed-format
decoding and prefetches music. It does not establish that source mixing,
panning or resampling run on OpenMulticore. OpenLoco's already-decoded PCM
does not automatically benefit from those compressed-audio decoders.

Apply [the local asset preprocessing standard](../../docs/ASSET_PREPROCESSING.md)
to PCM endian/format preparation. Native S16 and native float caches have
different storage/memory costs; benchmark them. Variable vehicle pitch and
final mixing remain runtime work even with preconverted assets.

## Evidence and remaining tests

`probes/sdl3_audio.c` was built against a host build of the same patched SDL
3.4.14 source, with `SDL_AUDIODRIVER=dummy`. It passed two-stream mixing at
independent gains, a changed frequency-ratio setting, mono/stereo conversion,
detaching one stream while the other continues, and clean close. It observes
the postmix samples. The constant-sample probe verifies acceptance of the
pitch setting, not spectral correctness of pitched audio.

Before qualifying a backend: compare pitched tones and variable-rate engine
sounds with the reference; verify pan/gain, loop boundaries, pause position,
completion and handle reuse; check source exhaustion, callback lifetime/A4,
device failure/reopen, native PCM byte order and shutdown. Measure dropouts,
latency, CPU and memory under representative voice counts on clean 68040 and
AC090 targets. Test actual AHI/ACAHI output. Host dummy output proves none of
those hardware results.

No SDL2, SDL3, AHI or OpenAL platform implementation is changed by this
assessment. The current full-game recipe still stops at the missing OpenAL
dependency until the tested shared OpenAL-to-OpenAHI layer supplies it.

## Source references

- [OpenLoco audio interface](https://github.com/OpenLoco/OpenLoco/blob/6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e/src/Audio/include/OpenLoco/Audio/AudioEngine.h)
- [OpenLoco OpenAL implementation](https://github.com/OpenLoco/OpenLoco/blob/6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e/src/Audio/src/AudioEngine.cpp)
- [OpenLoco PCM loaders](https://github.com/OpenLoco/OpenLoco/blob/6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e/src/OpenLoco/src/Audio/Audio.cpp)
- [Existing SDL2 mixer module](https://github.com/DalsinAI/openamigartg/blob/0c98fe780e6531946be44bebbb3a9db0c7bb7ab2/library/modules/sdl2/satellites/module/sdl2_mixer_module.c)
- [Existing SDL2 mixer API](https://github.com/DalsinAI/openamigartg/blob/0c98fe780e6531946be44bebbb3a9db0c7bb7ab2/library/modules/sdl2/satellites/abi/SDL_mixer_procs.h)
- [SDL3 source/backend pins](https://github.com/DalsinAI/openamigartg/blob/0c98fe780e6531946be44bebbb3a9db0c7bb7ab2/library/modules/sdl3/UPSTREAM.json)

- [OpenAHI implementation and documented gaps](https://github.com/DalsinAI/amigachrome/blob/476d01e07c373b12c7d0d2d67c932b6d75e9b3ec/vendor/amigachrome-guest/devices/ahi.device/README.md)
- [Native ACAHI host-mixing modes](https://github.com/DalsinAI/amigachrome/blob/476d01e07c373b12c7d0d2d67c932b6d75e9b3ec/vendor/amigachrome-guest/devices/acaudio.audio/README.md)

## Result (31 October)

Done as recommended: OpenRTG's OpenAL.module (OpenAL 1.1 over AHI, the Team's
own code) is the backend. `source.json` pins OpenRTG
`8b6d09ff1023edc3fb1e62e3db0490ec385820b2`; `build-amiga.sh` builds the module
and its link stub with the GCC 16 stove and links OpenLoco with
`-DOPENLOCO_DISABLE_AUDIO=OFF`. OpenRTG's OpenAL has no EFX, so patch 0017
supplies the EFX names and reverb stays off. In the lab OpenLoco reports
"OpenAL 1.1 OpenAL.module 2, Vendor: AmigaChrome Team, Renderer: AHI Voices,
initialized" with 992 mono and 32 stereo sources. Labs are muted: listening
on an unmuted instance is still to do.
