# GrafX2 for OpenUp

Native 68040 soft-float port of GrafX2 2.9, built with GCC open3.
Upstream pin: 678a0f54ea7f3eeebc3fa4135213ea63f45a28bd.
Target: AmigaOS 3.2.3 / AC090. Runtime testing uses FS-UAE / AROS m68k and AmigaOS 3.2.3 / AGA.

| Service | Integration |
| --- | --- |
| OpenRTG | Dirty indexed rectangles through ORTG_WritePixels; native WriteChunkyPixels fallback. OpenRTG owns bitmap recognition, clipping and GPU fences/source lifetime. |
| OpenGPU / OpenGfx | OpenRTG dispatches supported drawing to OpenGPU. The reference OpenGPU contains OpenGfx. The app queries capability without installing or changing global graphics patches. Hardware acceleration requires a suitable driver/card. |
| OpenDatatypes / OpenPicture | Standard picture methods for imports. datatypes.library V47 is preferred, with V45 fallback. The reference stack uses OpenDatatypes and OpenPicture. |
| OpenPNG | V47.4 OIM_ENCODEPNG path accepts ARGB pixels directly; generic PNG DataTypes DTM_WRITE remains a fallback. Temporary-file and backup/rollback commit logic protects an existing destination. |
| OpenFont | TrueType/OpenType text brushes through face opening, shaping, fallback-face selection and A8 rasterization. Existing bitmap skin fonts remain. Latin-1 text-field input is converted to UTF-8. Multiline text, antialiasing and synthetic bold/oblique are supported. |
| OpenMulticore | Pure indexed-to-ARGB conversion in 32-row blocks at 16,384 pixels or more. OMC_RunRange, private aligned/page allocations, strict grants, and completion before publishing pixels. Failed jobs run the same worker serially. |
| OpenZlib | z.library supplies GPX decompression. No static zlib. Missing OpenZlib affects that handler; its error is reported. |

OpenCompress is an archiver application, not a shared zlib replacement. OpenAudio,
OpenTLS and OpenPrint have no corresponding feature in this paint-port slice.

On AC090 (a lab copy of Instance-32) the range conversion ran on the cores
board's extra cores and OpenGPU reported its hardware backend; see the RC2
validation record. AmigaOS 3.2.3 AGA and Workbench startup are covered by the
RC1 validation record.

PNG exports preserve RGB and binary transparency. Palette-index ordering, private
PNG metadata, general partial-alpha import, layers and animation are not promised.
Use GrafX2's layered formats for editable masters. Lua, RECOIL and Workbench AppWindow drops remain outside this candidate.
An original classic tool icon, selected-project startup and a 512 KiB stack are
included. The Workbench loader segment is retained for strict OpenMulticore
code grants; workers still make no OS calls.

The OS4 adaptation is the platform lineage; this build is a native 68k backend.
Preserve GrafX2's GPL-2.0-only licence, upstream notices, the 6502 core's notices,
and the bundled SDK/runtime MIT and Unicode/font licences.
