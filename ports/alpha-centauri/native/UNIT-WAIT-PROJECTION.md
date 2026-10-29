# Owned deferred selection state

This candidate adds an atomic backend projection of deferring the local current
unit (state bit2) and running the original offline ready search. The returned
selection is a spatial unit ID, not a phase capability or installed UI cursor.
Movement, orders, accounting and RNG are unchanged. The search can clear deferred
bits belonging to other factions, as the original does. Its clear mask can include
the newly deferred bit even when that bit was initially clear.

The private session command checks lifetime, revision, sole local human ownership,
suspension and storage aliases. It recomputes before applying changes. Revision
and persistence obligations reflect net world changes; a repaint obligation is
reported even when the final world is unchanged. Rejected preparation never sets
the deferred bit. Export/reload preserves all original container bytes except the
supported actor-state changes. A missing selection never authorizes End Turn.

This is **not a complete Console_wait command**. The original command clears its
force-search field, defers the actor, then calls real Console_ready. That call
renders the cursor before searching, assigns the selected ID, and can perform map
focus, iteration reset, rally orders/waypoints, audio and input flush. Only after
it returns does Console_wait close the unit command loop. These UI and command
lifecycle effects remain unimplemented here; no fake renderer return or callback
acknowledgement substitutes for them. No player control is exposed.

The original UI Skip command also remains distinct from the bounded veh_skip
helper: it enters the real order executor with direction−1 and flags3 through the
offline network dispatcher. The executor normalizes direction8 and includes
Popup construction, world/stack/base checks, aircraft routing and shared effects.
The helper's one-byte assignment does not qualify a complete player Skip command.

Validation:

- 144 synthetic worlds, save versions7..12, on ASan/UBSan host and independent
  Linux68040:88 selections,56 empty results,2,424 guards per architecture and
  identical complete container bytes, selection metadata and clear masks.
- All144 inherited ready-search worlds remain unchanged on each architecture,
  including the prior output hash and2,448 guards per architecture.
- Private original observations,24 per edition: the actual Console_wait prefix
  stops **before** its Console_ready call; every mapped global and Console byte
  matches the defer effect. A separate harness invocation then runs the complete
  original offline search with real dependency helpers and normal return.
  Both owned architectures match all48 resulting complete actor arrays and
  selected IDs. This is not execution of the complete original Console_wait.
- Two fresh normal Amiga SDK builds are byte-identical. AmigaOS and remote CI
  remain **UNRUN**; publication is held.

Source: OpenSMACX GPL3-or-later, Brendan Casey2013–2021,
commit2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c; original command/search code
inspected privately. Owner executables, fragments and differential corpora are
private control files and are not repository content.
