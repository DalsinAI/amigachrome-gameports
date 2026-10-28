# Ordered original battle bookkeeping

`preview_battle_bookkeeping` computes two caller-owned numeric snapshots from
explicit fields: statistics before cleanup, then statistics and the enemy-destroyed
monument record after cleanup. It does not remove/capture a unit, update a World,
commit a session, or make an attack legal. This is a dependency for composing the
existing owned body-state result with the original caller effects.

Attacker loss increments runtime wins[defender]/losses[attacker] before cleanup;
defender faction combat_total/count[attacker] follow cleanup. Defender loss
increments attacker faction combat_total/count[defender] and runtime
wins[attacker]/losses[defender] before cleanup. Every increment preserves original
32-bit wrapping. Runtime wins/losses are not invented saved faction fields.
Source raw casualty argument6 remains a separate branch-specific parameter.

An explicit supplied runtime observation, multiplayer off and PBEM off are
required. Original caller prefixes in both editions execute their real branches
past combat notifications under those facts. This does not implement network or
PBEM notification UI, or infer runtime authority from a save. Other profiles are
held without changing the output.

After cleanup, both faction IDs must be nonzero before the source monument helper
is called. If the winner already has that award, or is an SMAX alien, its record
stays unchanged. Otherwise the original first-award/rank/leader selection and
wrapping year addition produce year/opponent/leader/awarded/rank/focus fields.
The last qualifying prior rank holder wins the scan; no sorting or invented tie
rule is used. SMAC ignores the alien rule bit. All other records stay exact.
The structures are selected numeric facts, not native or wire layouts.

The original helper writes its record before active-player `monument(1)` UI.
This API conservatively holds that path before publishing either snapshot.
It does not silently acknowledge a monument, return from a fake callback, or
claim that preparing fields has completed the original caller event.

Private original receipts cover 3,072 counter-range executions, 2,048 complete
counter-and-offline-notification prefixes, and 12,288 natural returns from the
whole enemy-destroyed helper with its real game_year helper. All compare 1,114,112
global bytes per call. Prefix harnesses append an observation jump at their exact
end boundary; the observed original instructions are unchanged. Full helper
calls have no observer/patch. Their explicit current-player -1 profile avoids UI
through its original condition. No full battle caller was executed.

Host ASan/UBSan and independent Linux 68040 controls each execute 17,380 cases:
15,836 accepted, 1,524 monument UI holds, 17 invalid, one missing runtime and two
network/PBEM holds. Each checks complete before/after snapshots, unchanged input,
transactional rejected output, and an aligned overlap guard. Cases include every
prior-award bitmask, both editions/outcomes, wrapping counters/year, alien flags,
arbitrary raw numeric fields and active-player holds. The independent Python
model and original source observations are separate evidence; the owned
composition has not been executed as a whole original battle caller.

Actual AmigaOS execution, remote CI, World binding, session/revision authority,
active-player event UI, artifact friction/treaty, base/cargo effects, persistence,
AI/turn/diplomacy and full playable games remain unqualified or incomplete.
