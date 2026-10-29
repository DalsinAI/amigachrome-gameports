# Prototype retirement queue stage

This is the complete callback-free **queue/deactivation suffix** of original
prototype retirement, not a complete retirement command or discovery award.
`retirement_queue_stage` creates an independently owned checked proposal for all
supplied bases and the resulting prototype flags. It preserves all unrelated
decoded fields. It does not mutate an OriginalWorld or serialize a save.

Original semantics retained:

- Predefined IDs0..63 do not alter queues or clear active status.
- Custom IDs64..511 scan every base, including other factions' bases.
- queue_size is the final active queue index, not the entry count.
- A matching current item becomes raw production ID-38 in base Alpha, -69 in
  Alien Crossfire, without reducing queue size or resetting minerals.
- A later matching entry shifts subsequent active entries left and decrements
  queue_size. The scan still increments: an adjacent shifted duplicate can
  remain. Tail entries are not cleared. This preserves original behavior.
- Clear only active bit1 of the custom prototype's flags after all queues.

The bounded owned API accepts at most512 bases (an implementation capacity, not
an assertion about original engine maximums), queue indices-1..9, and exact
edition/ID domains. Bad indices, null/wrapped input ranges, or overlap with the
proposal object fail before writes. Existing output is preserved on errors;
reload from the proposal's own previous array is supported transactionally.
Allocation failure also preserves output. These are proposal safety guards,
not original malformed-input behavior claims.

Validation:

```
python3 test_prototype_retirement_queue.py --out /ABSOLUTE/NEW/HOST
python3 test_prototype_retirement_queue.py --out /ABSOLUTE/NEW/BE --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ sh build_prototype_retirement_queue_probe.sh /ABSOLUTE/OUTPUT
```

516 cases pass host address/undefined sanitizers and actual independent Linux
68040/QEMU with identical output SHA256
f39cd7ab1ebf324e5677ba6dae39f27c27b630d1b08447e29a925111ec7d6fdd.
They include empty/512-base inputs, full queues, repeated entries, dormant tails,
all owners, malformed queues and transactional alias/wrapped/null failures.
The normal SDK build is build-only; actual AmigaOS execution is UNRUN.

Private instruction comparisons execute256 valid contexts per edition against
unchanged original suffixes: base0x595cff..0x595d9f and SMAX0x57f410..0x57f4a8.
Each compares720896 global bytes, including all raw base records and16-bit flags.
Only a documented observation jump replaces excluded epilogue bytes. Explicit
post-live-unit register/frame snapshots are supplied; no helper prefix or
callback is simulated. Original retirement history, replacement selection,
unit conversion/kills, base salvage and production dialogs were NOT executed.
No proprietary bytes, raw decompile, owner assets or private receipts are here.

A full discovery-triggered reactor update remains incomplete: propose_proto can
retire occupied designs, build/name a replacement, call prune_protos internally,
change production queues and trigger actual live-unit upgrades with energy and
modal/network obligations. The proposal cannot be committed as a substitute for
those preceding effects. Recursive Datalinks/TECHSHARE must also execute their
complete ordered effects; none is granted by this API.
