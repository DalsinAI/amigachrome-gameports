# Original combat sensors

`world_combat_sensor_coverage` reads the actual defender tile/faction and the
original25 offsets. Sea defenders and Planet faction0 receive no sensor bonus.
SMAC checks physical sensor-array bits. SMAX first checks the array bit, then
uses the actual nearest base and physical Geosynchronous Survey Pod to return
status2. Nearest-base ties select the last record. The original distance metric,
flat bounds and single horizontal wrap are retained.

Land sensor ownership uses the stored tile territory byte and the original
GameRules0x80/diplomacy0x08000008 predicates. A foreign owner without both treaty
bits can resolve as unowned and benefit the defender; with both treaty bits or
GameRules0x80 it resolves as foreign and is excluded. This original behavior is
preserved. Sea sensor ownership instead uses the nearest base owner. A sea array
with no base and an active sensor owner>7 reject before the original unsafe
array access; they are not silently assigned neutral ownership.

Coverage flags are0,1,3: none, ordinary sensor, or ordinary+GSP. The GSP flag is
SMAX-only. `sensor_combat_adjustment` applies the same rule percentage once for
bit1 and again for bit2, preserving signed multiply/divide order and truncation.
Every original int32 intermediate is checked. `world_sensor_combat_adjustment`
combines that stage with the actual World coverage. No nearest-distance/UI
cache, World state, original bytes or RNG is written. These are read-only stages,
not final battle strengths, legal attacks or command authority.

Raw host ASan/UBSan and independent Linux68040 execute565 controls each:
276 accepted,215 invalid and74 checked-overflow rejections. World controls cover
both editions, sea/land, arrays/GSP, flat/wrapped edges, mixed base/territory owners,
diplomacy and bounds/aliases. Duplicate-base diagnostics distinguish first versus
last nearest-base ties; these do not claim a valid newly generated game. Explicit
no-base sea-array and invalid-owner fixtures exercise typed refusals.

Private tests execute COMPLETE unchanged original battle_compute at SMAC516390
and SMAX501da0 in1,158 contexts per edition (1,152 sensor contexts plus6 tie
contexts). Real sensor/base_find/whose_territory/basic/morale/prototype/ability,
Strings_get and reached CRT helpers return normally; original radius tables are
mapped directly. There are zero instruction patches, callback stubs, imports or
PE startup. Independent coverage facts plus the actual scalar stage on both
owned architectures reproduce all56 recorded numeric bytes: final strengths,
modifier counts/values and morale caches. This does not claim full owned
battle_compute or direct complete-original World execution. Label buffer contents
are not compared; a supplied bounded label table feeds genuine string helpers.
All other851,276 SMAC/851,268 SMAX global bytes are checked unchanged, excluding
declared battle/string/cache and nearest-distance write spans.

The first World builds failed strict misleading-indentation warnings; layout was
fixed without disabling warnings. A private direct comparison initially omitted
the documented LeakSanitizer ptrace setting, then reran with detect_leaks=0;
address/undefined sanitizers remain active. Draft offsets were replaced by the
actual matching tables from both originals before execution. Earlier fixture
sets are retained; the final set adds tie and unsafe-path controls.

SDK builds are compilation only. AmigaOS/native execution, remote final-head CI,
player UI, full battle/damage/casualties/AI/turn and full-game playability remain
unqualified. No watch input or lab writes occur.
