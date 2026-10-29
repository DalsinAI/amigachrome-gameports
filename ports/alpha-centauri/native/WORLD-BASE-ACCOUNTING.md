# Connected base resource forecast

`world_base_accounting` joins fresh mineral support with one convoy/worked-tile
calculation, then reuses that exact intake for nutrient, mineral and pre-trade
energy accounting. It projects support flags and energy-shortfall orders,
without changing any actor, base, faction, original bytes or session authority.
The summary and full actor-state output succeed or fail together. Late energy
overflow, invalid worker assignments, short buffers and aliases preserve both.

This remains the bounded directed-human inspection path: base game, valid
first-21 worked tiles, no specialists or automatic allocator. Fresh fungus
effects are supported. It does not bypass the explicit upkeep-stage gate.
Commerce and energy-grid inputs are explicit fresh caller facts. The in-game
forecast sets them to zero to show energy **before trade**, with that limitation
visible beside the number. It does not use saved energy surplus as income.

The candidate frontend's **F** key recomputes this forecast from its current
owned world and original parsed rule tables. Pending facility confirmation
must be resolved first. Escape and other actions close the overlay, preventing
stale forecasts after world changes. The ordinary frontend remains read-only.

On the untouched original Boreholes scenario, the native view displays The
Hive's food intake 12 / net +2, gross minerals 6, unit upkeep 2 / net +4, and
pre-trade energy 6. The second human base derives upkeep 1 / mineral net +6
and food net +1. The F/Escape/Q run returned 0, kept dirty state 0 and left
the scenario unchanged.

Validation covers the full composed output against independent wire/integer
models: 77 worlds and 107 base queries, including normal, negative-energy convoy
cancellation, late overflow and unsupported upkeep. All source regions, both
outputs and buffer tail guards are checked. Genuine 68040/FPU HUNK runs in
FS-UAE/AROS passed normal Boreholes, late overflow and upkeep-stage rejection,
with complete outputs byte-identical to host and independent oracles. Exact
hashes and UI evidence are recorded separately.

This is not a completed turn. Facility construction has a separately tested,
engine-internal stage and persistence encoder, with an explicitly nonpersistent
per-object ledger; it is not exposed through this UI. Actual staged worker
allocation, trade generation, economy/labs/psych distribution, riots, growth,
research, event processing, persistent turn sequencing and AI remain required.
