# Owned LAND artifact capture

`stage_artifact_capture` stages the original `battle_kill` early capture0 route
in an owned current World frame. It supports an artifact prototype (plan12), a
distinct nonnative recipient, LAND chassis for both actors, a nonmissile recipient
and a land destination with altitude bits>=96. Matching original unit and
technology tables and road rate are explicit inputs. No missing runtime facts
are inferred from a save.

Loss is counted first for the original faction/prototype. The current actor then
receives the recipient faction, damage0, order0 and state mask0xfcffbdff. Old/new
active-unit byte counters decrement/increment with source wrap; same-faction
capture naturally cancels those changes. Checked lift/drop moves the artifact to
the recipient's current stack head, preserving dense actor count and IDs. The
shared combined relocation primitive has no faction-dependent lift effect: its
lift phase reads links, coordinates and occupancy only; its drop phase uses the
new faction, as the original separate calls do.

`world_vehicle_speed` runs against the transferred current actor, with damage
already cleared and original design ID retained. This preserves the distinction
between the new vehicle owner and original prototype owner for speed abilities.
The resulting supported0..255 speed assigns the spent byte; it may lower an
already larger expenditure. Unsupported wrapped speeds are held, not clipped or
replaced with3. Only the optional captured caller pool increments. Killed and
native-reward pools, the raw argument6 counter, total combat count, faction
energy and all ordinary cursors remain unchanged. Lift runtime ends at id−1 with
the original source coordinates, as the source capture return does.

This route preempts the interlude gate even if the actor has state0x800000 and
actual GamePreferences0x40000 is clear. There is no invented interlude outcome or
acknowledgment. Water/stack_check8, missile/air/sea recipient profiles, missing or
self recipient, malformed facts, active multiplayer/logging/visible BaseWin,
unsupported speed, aliases, invalid storage and memory failure leave the caller
and any existing output intact. Failure never publishes a partial loss/transfer.
All output records, baseline bytes, monuments and history have independent
ownership; unit/technology input objects are not retained. Runtime world RNG is
preserved without drawing or inferring a seed.

The private reference contract already executed complete original battle_kill
capture routes432times per edition, including capture before an interlude, with
all bounded global bytes compared. Its controlled artifact speed was3. This
candidate separately composes the qualified original speed binding with capture
storage; its1062 synthetic cases compare against an independent Python wire model
on host and independent68040. They cover versions7..12,2/9/2048actors, linked and
solitary source stacks, same/different faction transfer (including old faction0),
byte/loss/caller-pool wrap, original flags/state, actual new-owner morale0..6,
explicit ground speed1..4 and road1..3, expenditure reduction, interlude preemption,
source destruction and output replacement. Full decoded faction comparisons mask
only the two permitted counter arrays. All actors, tiles, counters, energy,
runtime, actor-before record and caller pools are checked byte-for-byte.

The private original proof and this owned test corpus are distinct observations;
this is not a claim that the full original fight/GUI or this candidate ran in
AmigaOS. Original pool wrap semantics use inspected x86 ADD behavior; the prior
complete-original capture vectors used captured count42.

Host controls: `python3 test_artifact_capture.py --out /tmp/alpha-artifact-host`.
Independent controls add `--cc m68k-linux-gnu-g++ --m68k-linux`.
`build_artifact_capture_probe.sh` builds a normal SDK executable only.

Legal attack/engagement caller, session commit, artifact delivery/research award,
full cargo/kill caller, event/modal lifecycle, variable-length save encoding,
frontend/native execution, turn/AI/diplomacy and source publication remain held.
Original wire views remain the immutable imported baseline, not current saves.
