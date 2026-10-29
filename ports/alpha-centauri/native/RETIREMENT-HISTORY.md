# Retirement history stage

`retirement_history_stage` is the complete callback-free history append called
first by original prototype retirement. It returns an owned4096-byte table and
next selected-faction cursor, preserving all unaffected bytes. It writes the
selected design's ability flags asLE32 followed by chassis/weapon/armor/reactor
bytes into the original eight-byte ring slot; the cursor advances modulo64.
There is no name, plan, armor normalization, timestamp or discovery flag here.

The API accepts factions0..7, a supplied actual next-slot cursor0..63, the
selected original design and an exact4096-byte table. Malformed cursor/size/null
and output overlap with any part of the table/prototype reject before writes.
Signed-negative/out-of-range original cursor behavior is not qualified. The
original save block is `bunglist`; the runtime faction cursor is0x208c current
and0x207c legacy. These facts do not themselves provide a committed World update.

519 synthetic controls pass host ASan/UBSan and independent Linux68040/QEMU,
including every slot of all eight factions and transactional invalid/alias/
wrapped-range guards. Complete output SHA256:
570430bf584d025d26af30ef603be713f60d5bc6d3ce68626746229b3fc8f408.

```
python3 test_retirement_history.py --out /ABSOLUTE/NEW/HOST
python3 test_retirement_history.py --out /ABSOLUTE/NEW/BE --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ sh build_retirement_history_probe.sh /ABSOLUTE/OUTPUT
```

Private unchanged original helper runs cover512 valid contexts per edition,
all eight factions/all64 slots and all512 selected design IDs. Complete720896
runtime-global bytes compare per case. Base0x594210 (135bytes) and SMAX0x57df30
(130bytes) execute through their real returns, with no byte patches, imports,
callbacks or simulated return values. Both yield identical table/cursor outputs.
No original instructions, executable tables, assets or private receipts are
included. Normal SDK build is build-only: native AmigaOS remains UNRUN.

This stage does not perform replacement selection, live unit conversion/kills,
base salvage, queue/deactivation, pruning, production choices, recursive sharing
or a complete discovery award. Real current history/cursor/design must be bound
by the eventual complete transaction and persisted with explicit endian codecs.
