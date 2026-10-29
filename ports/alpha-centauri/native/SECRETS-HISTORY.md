# Secrets monument history stage

`monument_effect` additionally supports event2, the original Secrets history
mutation. It accepts the complete eight-record little-endian monument table,
explicit edition, faction, actual faction rule flags, local faction and current
game year. It proposes one complete owned record without mutating the source.

For a previously unachieved event, it records the current year, initializes both
extra-year slots to-1, and records first-faction status using all eight original
records. When several other records claim to be first, the last qualifying index
wins. Repeated events fill param2 then param1 only if the slot is-1. Latest event
becomes2 even when both slots are full. All unknown bytes remain unchanged.

Base Alpha does not suppress this history for alien faction flags; Alien
Crossfire does. A local presentation request remains possible even when no bytes
change. `changed` therefore reports actual record differences for event2, while
`presentation_required` reports the original monument(1) call boundary. It is not
an acknowledgement that the original modal lifecycle has run.

This stage independently adapts pinned Thinker monument.cpp (MIT attribution in
monument_effects.h) with instruction-observed edition differences. No proprietary
instructions, executable tables, assets or private receipts are included.

Validation commands:

```
python3 test_secrets_monument.py --out /ABSOLUTE/NEW/OUTPUT
python3 test_secrets_monument.py --out /ABSOLUTE/NEW/BE/OUTPUT --cc m68k-linux-gnu-g++ --m68k-linux
python3 test_monument_effects.py --out /ABSOLUTE/NEW/REGRESSION/OUTPUT
```

The Secrets control contains2048 synthetic cases covering initial and repeated
history, arbitrary nonzero markers, absent/multiple first markers, negative/zero
years, both editions, alien flags, local/foreign owners, unchanged records with
presentation, and full byte preservation. The probe also checks null and alias
rejection transactionally for event2 and the inherited event4. Existing2064
facility/Preserve cases remain unchanged. Host sanitizers and independent Linux
68040 execution pass identical output bytes. A normal SDK binary was built;
AmigaOS execution remains UNRUN and no player controls are exposed.

Private original comparisons execute each original Secrets helper and its real
game_year helper over the same1024 contexts per edition. Foreign/alien return
paths run naturally; local requests stop immediately before the genuine monument
presentation call. Complete720896-byte global state is compared per case.
This is history-stage proof, not a discovery award or complete original helper
presentation proof. The proposal does not set ownership, first-Secrets faction
flags, grant bonus advances, execute recursive technology sharing or encode a
committed world. Those remain part of the wider discovery transaction.
