# Original supply-pod detection

Original goody_at returns0none/1ordinary/2special from current map seed, tile
climate/items, game rules and explicit signed map coefficients. Both original
EXE initialized coefficients are61for difference and11for average. These are
runtime data inputs; map seed is used for map placement only and never replaced
with the movement/combat random streams.

Supply-remove/monolith masks take precedence, followed by no-unity-scattering,
explicit pod bit, zero seed, generated land and generated secondary placement.
Generated land uses floor division for negative diagonal coordinates; secondary
placement uses signed division toward zero. Unsigned arithmetic retains original
low bits without native signed overflow. Coordinates/map geometry reject before
writes. Result0 does not authorize movement or retreat.

20028public synthetic controls (20015accepted/13invalid), including4096map edges, direct map/tile/coefficient
alias checks and independent Python outputs pass host ASan/UBSan and actual
Linux68040. Actual unchanged complete original goody_at returns2858times per
edition, every mapped851968global byte unchanged, comparing both owned binaries.
The final stride7 sample exercises all branches; initial stride10 sampled only
no-unity-scattering and is retained as limited history, not full qualification.

No World binding, pod opening/reward/discovery/effects, RNG mutation or action
controller exists. Native/frontend/remote CI UNRUN; normal Amiga SDK builds only.
