# Complete unused custom-design retirement proposal

`world_unused_prototype_retirement` binds the real owning World: selected custom
design, decoded faction history cursor, owned imported `bunglist` bytes, every
base queue and every live vehicle reference. It stages the complete original
retirement gameplay changes for a deliberately bounded domain: **no live vehicle
references the retired design**. Any reference, including a foreign unit,
returns `live_unit_effects_required` before result changes. It does not call a
fake kill/conversion/stack/salvage or presentation/network callback.

Supported inputs are faction1..7 and its custom design64..511, corresponding
base legacy or expansion current record profile, a valid current cursor0..63,
and at most512 bases (our explicit proposal capacity). Both active and inactive
designs retain the original history append behavior. No auto-design preference,
no-advances/blind flag or owner data is altered to reach this domain.

The owned result contains the proposed complete history table/next cursor,
cloned bases and resulting design flags, identified by faction/design ID. It
composes the independently qualified history and queue/deactivation stages. The
queue operation is the final fallible step; successful byte/scalar assignment
then finishes the result. All errors preserve previous results. Output aliases
with any World object, raw/monument buffer or decoded allocation reject before
writes. The World and every input byte remain unchanged.

This is a proposal, not a commit or an action exposed to the player. It has no
session revision/authority or save encoder. `bunglist` currently remains the
owned imported snapshot; a future retirement mutator must own/update its current
history and cursor together instead of chaining plans against stale wire bytes.
Full reactor proposal/prune/production choices and complete discovery still
remain unimplemented.

Validation commands:

```
python3 test_unused_prototype_retirement.py --out /ABSOLUTE/NEW/HOST
python3 test_unused_prototype_retirement.py --out /ABSOLUTE/NEW/BE --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ sh build_unused_prototype_retirement_probe.sh /ABSOLUTE/OUTPUT
```

70 generated owning worlds pass host ASan/UBSan and actual independent Linux
68040/QEMU, identical output SHA256
fe922d32362eec4a1bf0c2be6bb3e694cc505e2f54e76cc05b4865c2944ada31.
They include40 accepted worlds,24 typed live-reference refusals, and6 owner/
edition/ID/cursor guard failures. Accepted worlds range from empty actors to128
unrelated vehicles/512 bases, multiple owners, duplicate/full/dormant queues and
arbitrary design flags. Nonempty-result rejection and nine owned-range alias
guards run in the probe. The entire World object, both owned byte buffers and
all decoded arrays are hashed before/after each query and alias checks.

Private original comparisons execute the COMPLETE original `retire_proto` and
its real history helper through their real returns,20 accepted worlds per
edition. Original helper bytes remain unchanged: no observation patches, fake
returns, callbacks, preference suppression or PE startup. Existing unrelated
vehicles are genuinely scanned. Complete720896 runtime-global bytes match an
independent model, including all raw base records, history, cursor, prototype
flags and unchanged vehicles/factions. Dynamic translated-block census finds
only35 block entries per edition within the two original helpers and own driver.

Base helper0x595ac0 (744bytes) calls real0x594210; expansion helper0x57f1d0
(736bytes) calls real0x57df30. The whole-function accepted-domain proof is stronger
than earlier separate suffix observations, but does not qualify live-reference
branches, a full discovery award or whole game. No proprietary instructions,
private receipts or owner assets are distributed. The normal SDK builds; actual
AmigaOS execution remains UNRUN while the user owns the watch session.
