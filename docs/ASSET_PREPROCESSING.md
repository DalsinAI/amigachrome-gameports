# Asset preprocessing for ports

Local asset preprocessing is a standard porting step for the AmigaOS 3.x,
68040 + FPU / AC090 target. Assess endian conversion, decompression, pixel
layout and other repeated loading work before leaving those operations on the
game's critical path. Prefer a reusable native-format cache when it reduces
loading work at an acceptable storage and memory cost.

## Conversion contract

- Generate caches locally from data the player is entitled to use. Never alter
  the original installation or assume that converted commercial assets may be
  redistributed. Converters and synthetic fixtures may be distributed under
  their own licences.
- Convert structured fields according to their schema. Never swap every word
  in a file: text, byte arrays, compressed streams, pixels and embedded formats
  have their own layouts. Decode compressed containers before converting their
  structured payloads.
- Use an explicit cache format with fixed-width fields and offsets. A cache is
  not a dump of live C/C++ structs, pointers, compiler padding or host ABI data.
- Identify the source files by hashes and record the converter revision, cache
  schema version and relevant target representation. Reject and regenerate
  caches when those identities change. Detect truncated or corrupt output.
- Write temporary output, validate it, then publish it atomically. Conversion
  failure must leave the original data and any valid previous cache usable.
  Report disk-space and permission errors clearly.
- Keep user saves and exchange formats compatible unless a separate,
  explicitly documented migration is approved. A native asset cache does not
  silently change the save format.

## Qualification

Compare decoded values against the original loader using synthetic fixtures
on little-endian hosts and big-endian m68k. Include signed fields, unaligned
fields, nested containers, malformed lengths and stale-cache rejection.
Exercise representative legal user-supplied data privately; retain hashes and
test results without committing that data.

Measure initial conversion time, cache size, peak memory, cold/warm loading
time and startup time on the intended target. Record what work remains at
load time. A converter does not establish a performance improvement until
those measurements support it.

Each port's notes must say whether preprocessing is implemented, unnecessary,
or blocked by an incomplete format audit. Where an original-data loader is
available, retain it as a correctness reference and recovery path. Runtime and
release qualification still follow [the release policy](../RELEASE_POLICY.md).

## Initial applications

OpenLoco is a candidate for a versioned big-endian asset cache. Its Sawyer
containers, packed object records and save structures need separate audits;
fixing a container's length and checksum fields alone does not convert its
payload. This document defines the practice, not a completed converter.

LBA2 already contains endian-aware loading helpers. Review their costs and
coverage before adding a cache, and preserve the engine's exact decoded
values and original gameplay behaviour.
