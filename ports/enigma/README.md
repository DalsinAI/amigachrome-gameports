# Enigma — source-pinned, SDK dependency block

Source pin: `6f0c51b26cc013cb6c935046d7d1e6a20bea4179`,
[upstream](https://github.com/Enigma-Game/Enigma). GPL-2.0-or-later engine,
SDL2/image/ttf/mixer, Xerces-C, curl, zlib, bundled Lua/ENet/oxydlib/tinygettext.
Normal levels are in `data/`; no retail Oxyd upload is needed. Audit level,
sound and font notices before distributing the data tree.

```sh
python3 scripts/game_port_prepare.py prepare --game enigma --allow-network
python3 ports/enigma/configure_open3.py --source build/game-ports/sources/enigma \
  --stove /path/to/verified-open3 --out build/enigma-sdk-gate
```

This is an offline dependency/configure gate, not a finished build recipe.
It exports the exact pin, inventories only the selected target SDK and its
port-deps prefix, records hashes, and exits 2 when required inputs are absent.
It never substitutes host libraries or deploys a build.

On 24 October the selected open3 SDK had all four SDL2 components and zlib,
but lacked `libxerces-c.a`, `xercesc/util/PlatformUtils.hpp`, `libcurl.a`
and `curl/curl.h`. A direct cross-CMake run also failed at Xerces-C discovery.
No Enigma engine has compiled or run in this intake.

Curl itself is already ported in
[OpenAmigaCurl](https://github.com/DalsinAI/openamigacurl). Its build disables
the command-line executable and supplies static libcurl for applications.
The missing SDK integration is distinct from porting curl anew. Stage the
matching curl, TLS/socket dependencies and headers with provenance, then
cross-link a real consumer. Xerces-C remains a separate dependency gate.
After configuration: compile/link, level/input, persistence, audio, clean exit
and intended-ROM repeat. No build or RELEASE claim is made while blocked.
