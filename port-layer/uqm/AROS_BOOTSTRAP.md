# The Ur-Quan Masters 0.8.0 AROS/m68k bootstrap

The canonical build input remains the official UQM 0.8.0 SourceForge source
archive pinned by SHA-256 in AmigaChrome's game-port catalog. A public mirror
whose source identifies itself as 0.8.0 was used only to inspect the build
contract.

The first AROS build keeps UQM's own build system and pre-seeds its normal
configuration state:

- SDL2 graphics;
- internal MixSDL sound engine;
- no Ogg/Vorbis codec for first light;
- internal MikMod;
- internal Lua;
- network SuperMelee disabled;
- joystick enabled;
- direct file I/O (no ZIP content dependency for first light);
- plain-C acceleration;
- SDL thread backend;
- release build.

The cross-build patch adds AROS as an explicit host type and points SDL2,
libpng and zlib at the prepared AmigaChrome AROS Developer sysroot. The
Kitchen supplies a private PATH where `gcc` resolves to the pinned
`m68k-aros-gcc`, matching UQM's documented cross-build model.

Game content is deliberately separate. The resulting binary can be launched
with UQM's own `-n/--contentdir` option pointing at user-supplied/extracted
0.8 content. No UQM content pack is bundled by this build lane.

This is the SDL2 bootstrap gate. Once it reaches the menu/solar system, its
graphics backend can be replaced by the shared ACGame indexed AGA presenter;
conversation portraits remain a strong HAM8 presentation candidate.

[executed on device: daletop (557d2ffd-2bd4-42f8-8777-9a5024193e1e)]