# OpenLoco S5 checks, 24 October 2026

**Metadata/boundary checks pass; OpenLoco is not playable.**

Upstream remains `6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e`.
Patch 0014 decodes S5 header, save-preview and complete scenario-options
metadata explicitly as little-endian. It checks state/tile copy boundaries
and refuses unsupported big-endian simulation import and save export.
See [scope and reproduction](../../S5_ENDIAN.md).

63 repository tests and four host CTest probes pass. Structure checks pass
on the locally supplied title file and 45 scenarios. The HUNK probe returned
0 on AC090/68040+FPU for synthetic checks, the title and one scenario.
Both target save-export entry points compile to `clrb d0; rts`.

The full application compiled/linked with verified open3 and the frozen
OpenGPU SDL3 SDK. S5 files were refined before their translation unit compiled;
the final tree matches an independent exact-pin/final-patch export. No second
clean binary build is claimed. Known linker warnings remain. Detailed local
logs are retained privately; this public summary includes no environment logs.

The version command printed the pinned identity. Subsequent 512 MiB startup
showed an allocation failure before S5 loading; no gameplay or clean exit is
claimed. A private 1 GiB restart did not reach the guest command. No minimum
RAM requirement is established. Both runs were stopped and the copy restored.

Next: complete legacy/modern S5 simulation import and inverse export, including
active entity variants, packed map/order fields and required object headers;
compare host/68k reference state before removing guards. Investigate startup
allocation separately. Audio, gameplay, persistence and release gates remain.
