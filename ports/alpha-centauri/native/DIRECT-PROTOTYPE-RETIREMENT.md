# Direct-conversion original retirement proposal

This extends the frozen unused-design proposal with a complete source-backed
retirement domain: every actual vehicle referencing the retired custom design
must take the original direct fallback-conversion branch. It stages owned copies
of the vehicles, selected faction state, history, all base queues and final
prototype flags. It does not mutate a World, commit a save, grant a technology,
control an AI faction or provide session authority.

Replacement selection scans the faction's64 custom designs in original order,
excluding the retired ID, requiring active status and equal group ID. Score2 is
nonobsolete for this faction, score1 obsolete; first strictly highest score wins.
No invented component-strength, technology or prototype filter is added.

For each referencing vehicle in reverse original order, direct conversion
requires the retirement faction to be nonhuman in the saved membership snapshot,
a replacement, and matching vehicle ownership. A unit visible to any human
additionally requires its actual map tile to have the original base item bit and
a positive low-nibble owner below8. Otherwise the whole proposal rejects with
`live_callbacks_required` BEFORE changes. Missing replacements, human retirement
factions, foreign units and visible nonqualifying tiles retain the unimplemented
kill/base_at/stack/salvage/modal/network obligations; no callback return is faked.
No-reference worlds remain supported irrespective of human membership.

Conversions change only the vehicle design ID and the selected faction's two
`units_active` bytes, preserving original modulo256 decrement/increment behavior.
These are actual saved byte-counter updates, not recomputed fleet counts. Other
faction/unit fields remain unchanged. History/cursor, queue semantics and custom
active-bit clear then complete the retirement proposal. There is no mineral
reset or invented bookkeeping. Saved human membership is data for this query,
not authority for a player/AI action or proof of current engine runtime flags.

Inputs are restricted to the correct legacy/current profile, owner1..7 and its
custom design64..511, actual cursor0..63, at most2048 vehicles/512 bases (explicit
proposal capacities). All affected vehicles are checked before allocation or
writes. Output aliases with all World allocations reject. Allocation failures
preserve existing output. No live World mutation, endian snapshot encoder,
revision/session authority, prototype proposal/pruning, production choice or
complete discovery award is supplied. Current history still comes from the
owned imported bunglist; future commit integration must update current history,
cursor, counts, vehicles, queues and flags together instead of reusing baseline.

```
python3 test_direct_prototype_retirement.py --out /ABSOLUTE/NEW/HOST
python3 test_direct_prototype_retirement.py --out /ABSOLUTE/NEW/BE --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ sh build_direct_prototype_retirement_probe.sh /ABSOLUTE/OUTPUT
```

128 synthetic owning-world contexts pass host ASan/UBSan and actual independent
Linux68040/QEMU, output SHA256
5963727234ff252ffa061aafdd34b8731145f989fa345f09b0135b42a48b24f1.
They cover no-reference and actual conversion, visibility/base-owner gates,
missing replacement, foreign/human refusal, first-choice score ties and improved
nonobsolete selection, byte-counter wrap, full queues and original history.
Complete World object/raw+monument buffers/all decoded arrays are hashed before
and after; full unaffected fields and nonempty-result failure are checked. Nine
output-alias ranges reject transactionally.

Private comparisons run the COMPLETE unchanged original retire_proto plus its
real history helper through normal returns in16 accepted contexts PER edition.
All720896 runtime-global bytes independently match, including raw vehicles,
faction counters/cursor, history, queues and design flags. No original byte
patches, simulated calls, imports, kill/salvage callback or PE startup is used.
Dynamic translated-block census finds46(base)/47(SMAX) blocks, only inside the
two real helpers and own driver. Runtime human mask, tile bytes and half-width
are explicit actual snapshot inputs, not forced bypass flags. Whole-function
proof is limited to the accepted domain, not all retirement branches.

Original bytes, decompile, owner assets and private receipts are not included.
Normal SDK builds; native AmigaOS remains UNRUN. An initial independent compiler
warning in the probe exposed a missing explicit null check before comparing base
records; fixed before the final full host/68040 controls. No failed run is
silently treated as qualified.
