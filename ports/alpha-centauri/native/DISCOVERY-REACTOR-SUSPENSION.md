# Owned reactor suspension

`world_discovery_reactor_suspension` is an additive checked query over the current
owned decoded World. It runs the same complete ordered bounded base reactor
sequence as the existing query. It can now retain that private staged frame at
an actual local reactor notice rather than rejecting it or inventing a dialog
return. The prior completed nonlocal API preserves its behavior and typed holds.

The domain still requires explicitly supplied runtime observations, original
base data, a human faction, multiplayer off, both auto preferences off, an
already-owned technology, CurrentTurn nonzero and SkipTechnologyScreen zero.
Unsupported live-reference retirement, naming/strength/callback/SMAX paths remain
held. Imported human or MapOwner fields and all runtime observations are data;
this query grants no authority to run an action or show a window.

## Boundaries and ownership

- `reactor_presentation` is the real pre-parse boundary at original base
  0x5d2b90. The privately owned World has completed all supported reactor-source
  proposal, retirement, history, record, name, prune and queue changes. Its
  history cursor is the actual staged cursor. Last-created and upgrade scratch
  remain intermediate. Two explicitly supplied original UI-cache observations
  are retained unchanged. The request owns the full name of the final eligible
  reactor, even when its proposal returned -1. Actual parse arguments are
  (0, name, -1, -1), followed by the real REACTORUPGRADE notice.
- `post_reactor` is the original 0x5d2c4e boundary after the supported nonlocal or
  local-no-pair route, including last-created and both cache resets to -1.
  It is not completed discovery. A local/omniscient generic discovery window
  is still mandatory immediately after this point.

`ready()` means this bounded plan exists. It does not mean a technology was
awarded. Pending reactor name and all decoded records remain private owned
copies accessible only through const getters. No external runtime pointer is
retained. Replacing a pending plan with a nonlocal result clears its name and
pending metadata, then rebases history storage; replacing it again safely
restores a pending result.

There is deliberately no acknowledge/resume/modal-return method, generic-window
result, nested-award callback, World commit or session commit. The earlier
DISCOVERY-CONTINUATION-CONTRACT documents the exact later order: generic
presentation, Secrets, local choices, reveal, technology effects, local
interludes, recursive Datalinks/TECHSHARE and first-Secrets notification. This
query executes none of those later stages and cannot skip them.

## Validation

The independent parent corpus remains 102 cases with its previous exact bytes.
The new corpus runs every supported parent state with local, nonlocal and missing
boundary observations: 264 cases. It checks every staged prototype, history,
queue and faction cursor against the independently interpreted parent effects,
plus intermediate runtime words and the owned reactor name. It includes changing
multi-reactor allocations, history/reuse/no-slot returns, collision naming,
current-decoded versus raw imported state and faction 7.

The probe checks complete input preservation, failed-call preservation, output
aliases into World/rules/definitions/facts/runtime/cache inputs, and input aliases
into old private plan storage. It rejects unterminated reactor names, preserves
the prior result on failure, and exercises successful pending/nonlocal/pending
replacement, history rebasing and input destruction. Host ASan/UBSan and actual
independent Linux 68040 execution are required. The normal SDK build is build-only;
no native/lab run is claimed.

The frozen private original-prefix proof has 12 local contexts: eight genuine
parse boundaries and four no-popup paths, complete unmodified real helpers and
all 720896 mapped globals compared. Sixty parent fixture parts used here are
byte-identical to that proof. Its original untouched cache words are also varied
in the portable corpus; these additional values are checked preservation cases,
not a claim of additional original helper replay. All original bytes, data and
receipts stay private. Workflow commands are unchanged and no publication occurs.
