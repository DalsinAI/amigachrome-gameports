# Owned ordinary and native casualty staging

`stage_casualty` owns a current world frame and caller accumulators for the
ordinary/native deletion route of original `battle_kill`. It stages a complete
result without changing the caller world, runtime observations or pools. This
is a gameplay dependency; it does not authorize an attack or commit a session.

The result increments the victim faction's uint16 `units_lost[prototype]`, the
raw flag0x20 argument6 counter when supplied, the optional native reward pool,
and the optional killed count. Counter arithmetic follows original unsigned
bit wrap. Reward uses the actual victim's original native morale with opponent
zero and `(rank+1)*10`. It never credits faction energy. The raw faction argument6
is supplied explicitly: the inspected SMAX caller passes the attacking faction,
which must not be inferred to be the winning faction. Captured and absent pools
remain unchanged.

The frozen checked removal then supplies dense compaction, stack/tile effects,
active/combat counts, current World count and `VehCount`, and runtime reference
updates. Current decoded records may differ from imported bytes; the result
owns those records, monument bytes and history, and survives source destruction.
Original wire views remain the imported baseline, not an updated save container.

All artifact prototypes are conservatively held. The original capture route
precedes its interlude gate and increments the loss count even when returning
capture0; no artifact is silently deleted here. Required interlude routes are
also held, even where further source conditions could make them unnecessary.
No partial loss/counter result is published on these holds. Missing required
native counterpart, invalid IDs, aliases, missing runtime observation, active
multiplayer/logging/visible BaseWin, invalid storage and memory failure likewise
leave an existing output intact. Offline observations are supplied facts, not
facts inferred from a saved file.

Validation covers versions7..12, solitary and linked stacks, 1/2/9/2048 actors,
ordinary/native actors including native IDs0/10/19/511, all caller-pool presence
combinations, turns44/45/89/90/169/170/329/330, counter wrap, output replacement,
current state/flags/fungus changes and source destruction. An independent Python
wire model checks every surviving actor/tile, active/lost counts, source counters,
energy, runtime and removed actor. Complete decoded faction comparisons mask only
permitted deltas; other records and imported bytes remain exact. Alias tests use
aligned addresses so the test itself does not form an invalid C++ reference.

The private reference contract separately executed2592 complete original
`battle_kill` calls across both editions, including native and artifact routes,
with every byte of the bounded global range compared after each call. These
are source-route observations, not an execution of this owned candidate inside
the original full game. Original native reward pools in that proof started at−5;
owned pool wrap controls additionally implement the inspected 32-bit ADD behavior.

Host controls: `python3 test_world_casualty.py --out /tmp/alpha-casualty-host`.
Independent big-endian controls add `--cc m68k-linux-gnu-g++ --m68k-linux`.
The normal SDK build script produces an Amiga executable only. Actual AmigaOS
execution, native UI, remote CI, source publication, artifact capture, interlude
outcomes, legal battle command/commit, full kill/cargo caller, variable-length
persistence, turn, AI and diplomacy remain unqualified or unimplemented.
