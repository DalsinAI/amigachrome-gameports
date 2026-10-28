# Complete nonmodal human prototype pruning

The original prune_protos helper has a genuine callback-free full-return domain:
the faction is human, actual MultiplayerActive is zero, and original auto-prune
preference0x100000 is OFF. It performs ordered component-coverage marking and
returns without entering AI/production-choice/upgrade/synchronisation branches.
This API proposes the complete512-design flag result for that domain, not a
committed World change or a completed prototype allocation/discovery award.

Runtime multiplayer is an explicit observation parameter. UNKNOWN and ON reject;
it is never inferred from a save, a player enum is not a trusted capability, and
this helper provides no active session/AI/network authority. Root's eventual
owning controller must bind genuine runtime facts separately. Snapshot human
membership/preferences are used only as read-only data. No original scenario
flag or owner file is altered to evade the required conditions. Source call
profile uses is_synch=0 as in the discovery caller; no synchronisation is faked.

The component phase runs only when selected ID<64 or selected flags include4.
It scans the faction's64 designs in original index order. An active design
without flag4 becomes flags|0x104 if its weapon, armor and chassis are covered
by available models. Each component can be covered by a different model:

- Exact weapon/armor IDs cover themselves, including special/nonpositive types.
  Otherwise a positive target strength requires at least that candidate strength.
- Exact chassis IDs cover themselves; otherwise equal triad and at least target
  unsigned-byte chassis speed is required.
- Active predefined models require original has_tech semantics (None/Disable
  asymmetry, known-owner bit, future88 excluded). Own models require flag4.
- Newly marked own designs participate in later-index comparisons. A frozen
  availability list would incorrectly erase this original ordering.

No reactor, ability or cost filter is invented. No obsolete-owner byte is
changed by this domain. Other human/AI/auto-prune/network paths reject before
writes and retain their actual production-choice/lifecycle obligations.

The query binds the owning World and supplied qualified technology/component
tables, checks counts/indices/byte-sized strength/chassis domains, and rejects
output overlap with the entire World/raw/monument/decoded storage, Rules and
UnitDefinitions. Every error leaves previous output unchanged. All512 flags are
owned result values; no source record is mutated or serialized.

```
python3 test_human_component_pruning.py --out /ABSOLUTE/NEW/HOST
python3 test_human_component_pruning.py --out /ABSOLUTE/NEW/BE --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ sh build_human_component_pruning_probe.sh /ABSOLUTE/OUTPUT
```

71 owning-world controls pass host ASan/UBSan and actual independent Linux68040:
38 accepted contexts with706 total flag changes across cases,26 runtime/domain
refusals and7 malformed structural guards. Output SHA256:
736d71ebcdcb7dbc8bacf47cb99837781084cccc6401a8fddcb41ce10ef25152.
Whole World and both rule objects are hashed before/after. Eleven input/output
alias ranges reject transactionally; populated output survives unknown-runtime
refusal. No native AmigaOS execution has occurred; normal SDK build is build-only.

Private comparisons execute COMPLETE unchanged original prune_protos and real
has_tech through their normal returns in19 accepted contexts PER edition.
All720896 runtime-global bytes independently match; only intended prototype flags
change. Base0x595db0/5597bytes+0x5d17b0/101bytes and expansion0x57f4b0/5033bytes+
0x5b9f20/105bytes are run with zero byte patches, fake callbacks, observation jumps
or imports. Explicit synthetic observed runtime is offline; this is not evidence
for the user's running watch. Dynamic translated-block census includes only the
two original helpers and own driver (62base/60expansion blocks), not the full game.
No proprietary bytes, decompile, owner assets or private receipts are included.

Real proposal slot selection, naming/retirement, auto-design consideration,
other prune branches, production upgrades, recursive sharing and full discovery
still require complete effects, trusted controller ownership and persistence.
