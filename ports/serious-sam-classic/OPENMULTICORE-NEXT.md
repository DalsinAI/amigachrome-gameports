# Serious Sam TFE — OpenMulticore hand-off map

This file deliberately describes **future OpenMulticore work only**.  The
first playable OpenUp build remains correct on one AC090/68040+FPU CPU.

## First-light rule

CPU0 owns AmigaOS, DOS, OpenGPU, OpenAudio, OpenInput and OpenSocket.
The first-light Serious Engine build is single-threaded at the engine layer;
SDL/OpenUp may use its own platform implementation underneath.

## Candidate compute domains after the playable gate

The following are the first places to measure for OpenMulticore jobs because
they can be expressed as bounded work over explicit Fast-RAM buffers:

1. **World/visibility batches** — portal/sector visibility and render-list
   preparation.  Output is a CPU0-owned list consumed by OpenGPU.
2. **Collision/raycast batches** — world collision-grid and ray tests already
   have concentrated engine code and do not need Intuition or DOS while
   computing.
3. **Model/animation transforms** — skeletal/model transforms can operate over
   immutable pose inputs and write transformed vertex/bone buffers.
4. **Texture preparation/decompression** — archive inflate and texture
   conversion are natural buffer-in/buffer-out jobs.
5. **AI batches** — only after profiling.  AI that mutates the live world
   remains on CPU0; pure scoring/path probes may become jobs.
6. **Audio mixing** — only the pure mixing stage.  SDL/OpenAudio device access
   remains on CPU0.

## Explicitly not worker-core code

- SDL event pumping and OpenInput;
- OpenGPU/GL command submission or buffer swaps;
- OpenAudio/AHI device calls;
- DOS/filesystem access and .gro archive ownership;
- OpenSocket calls;
- allocation through the Amiga C runtime;
- any code holding or sharing CPU0 locks.

## Later whole-engine experiment

Once the m68k/OpenUp build is playable, Serious Sam becomes a candidate for
the separate OMC_LoadApp/native-ELF experiment.  That is not required for the
first release and must not distort the portable m68k port.
