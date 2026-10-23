# Amiga engineering lane

The recipe temporarily selects `OPENLOCO_DISABLE_AUDIO=ON`. The default upstream
OpenAL backend remains available when that option is off. The disabled backend
reports no device, no buffers, no voices and no playback; it does not claim to
provide audio. The common OpenAL.module / OpenAHI implementation remains Team 4's
work. The host and AC090 contract probes pass, including rejection of playback
handles and safe channel-volume access. This is not AHI or OpenAL qualification.

## Native networking

OpenLoco uses `bsdsocket.library` directly. The existing SDK's resolver calls,
datagram operations, native error codes, `CloseSocket` and `IoctlSocket` are
used instead of treating sockets as libnix file descriptors. The SDK lacks
POSIX `sockaddr_storage`; the adapter provides storage sized/aligned for the
SDK's native IPv4/IPv6 address structs, preserving their ABI. Const casts stay
at the older SDK's native call boundary.

Each C++ thread opens and closes its own socket-library base. This follows
the caller-owned SocketBase convention used by OpenTLS. No TLS layer is needed
for game UDP traffic; this port neither adds nor replaces OpenTLS.

The Amiga lane is **IPv4 only**, as requested. Protocol::any resolves IPv4 and
creates an IPv4 socket. Explicit IPv6 requests return an error. Desktop
protocol selection is preserved. AC090 testing with the lab's bsdsocket.library
4.2 passed UDP loopback, localhost resolution, non-blocking receive and explicit
IPv6 rejection. Remote multiplayer, interruption/recovery and threaded socket
lifetime remain unqualified.

To reproduce the actual game-socket probe after a game build has produced its
Core/Diagnostics/Platform/Utility/fmt libraries:

```sh
python3 ports/openloco/probes/build_native_probes.py \
  --stove /path/to/open3-stove --sdl3-sdk /path/to/sdl3-sdk \
  --port-build build/openloco-clean --out build/openloco-socket-probe
```

Run `bsdsocket-probe` in an isolated AC090 lab with a 2 MiB stack. Its IPv4
loopback port is 35871. The probe explicitly enables its assertions even when
the application build uses NDEBUG. A target link, by itself, is not a pass.

## Other game-side changes

The Amiga platform adapter supplies monotonic SDL ticks, program/data/user
paths, native environment lookup, DOS interactive-output detection and a DOS
file held exclusively for single-instance protection. It creates the user
folder before taking the lock. Actual path/config/persistence qualification
remains a full-application gate.

The classic SDK has `fseek`/`ftell`, rather than `fseeko`/`ftello`. The adapter
checks errors and rejects offsets beyond LONG_MAX instead of truncating them.
IPO and unsupported PIC are disabled for this HUNK application/dependencies.

A pinned source export must not discover Git metadata from the containing
recipe checkout. Its version now uses the exact upstream pin, with the source
path matching the compiler prefix map used by SourceLocation.

## open3 compiler defect

IndustryList.cpp, TerraForm.cpp and TownList.cpp expose an open3 GCC
internal compiler error: an unrecognised SI-mode `ge` instruction during RTL
CSE. For IndustryList, O3, O2, O1 and disabling rerun-CSE failed; O0 compiled.
TerraForm also compiled at O0. The explicit engineering recipe option applies
O0 to the UI translation units. Files outside src/Ui retain normal release
optimisation. This is a temporary compiler workaround,
not a source-logic or gameplay change. UI behaviour/performance still
needs runtime testing. No compiler or shared stove is modified.

## Data and conversion

The user-supplied Spanish PC CD was assembled from two hash-verified archive
parts, unpacked without executing its Windows programs, and 613 cabinet files
were extracted with unshield at feaa32f6227fb306a87f4249a6618aa2ddd49966 (MIT).
A further 38 graphics/music files were copied from the CD Data directory,
for 651 game files in total. Original archives/CD and a per-file hash receipt
stay in the local media repository. No assets are committed or bundled.

The Sawyer wire header/checksum fix is **not** a packed-payload conversion.
Object records, graphics metadata, scripts and S5/save structures still need
an endian audit and schema-aware native cache, with reference comparisons.
LBA2's converter must not be applied to Locomotion files. Clean-target gameplay,
OpenGPU/INDEX8 presentation, input, audio, persistence and release-policy gates
remain open even if the application compiles.

## Native runtime links

The full application needs the approved compiler's libatomic for its 32-bit
atomic operations. It is linked explicitly; no private atomic implementation
is introduced. The open3 libstdc++ filesystem object references a C++-linkage
statvfs that the sysroot declares but does not implement. A port-owned adapter
uses native DOS Lock/Info to supply disk-space fields, preserving 64-bit byte
counts and reporting failures. Unused file-count/identity fields remain zero.
An AC090 probe checked native DOS capacity/free space against C++ filesystem
space and checked ENOENT for a missing path.
