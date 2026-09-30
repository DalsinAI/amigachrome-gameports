# ACGame AROS/AGA backend

The first native backend deliberately uses the operating system to open the
display, while keeping the actual frame planar.

- Intuition opens a 320x200 (or caller-selected) custom screen at depth 8.
- The game renders an 8-bit chunky frame in normal memory.
- `acgame_c2p8_ref` converts the frame into the eight planes owned by the
  custom screen BitMap.
- `LoadRGB32` updates the 256-colour ViewPort palette only when it changes.
- `WaitTOF` provides the first safe presentation boundary.
- A borderless backdrop window supplies raw keyboard/mouse IDCMP events.

This is intentionally a bring-up backend rather than the final fast path.
The first field gate is `aga_smoke.c`. Once that is visible and stable in an
AmigaChrome AGA instance, the same platform API is used by the game ports.

## Current limitations

- reference C2P rather than the planned 68040-specialized converter;
- direct write into the displayed BitMap rather than ScreenBuffer double
  buffering;
- keyboard is implemented; joystick/gameport.device is not yet wired;
- PCM audio and HAM8 presentation are still stubs;
- the first backend assumes the selected custom screen exposes eight planar
  BitMap planes. That is expected on the native AGA target and should fail
  closed on an incompatible RTG-only screen.

[executed on device: daletop (557d2ffd-2bd4-42f8-8777-9a5024193e1e)]