# Pending notification layout and presentation boundary

`notification_layout` is a portable text-layout stage for an already owned,
ready `DiscoveryNotification`. It returns owned glyph positions, not a technology
award, original popup return value or session permission. All previous pending
objects, current decoded state and resources remain unchanged. No acknowledge,
dismiss, keyboard-confirm or resume method exists at this stage.

The caller supplies actual rasterizer advance/ascent/line-height data and body,
caption and viewport geometry. Explicit observations are data, never authority.
The private host qualification uses verified existing Arial Regular12 and Arial
Bold14 with measured advances and kerning disabled; it never substitutes Arial
Narrow. These are the current port font sizes, **not** recovered Win32 logical
height/hinting equivalence. The probe body top24 and caption top0 are explicit
port test geometry; complete original margins, frame chrome, palette, sprites,
shadow, clipping and resolution-dependent scaling remain unqualified. Private
software raster images are host proofs, not screenshots of a running game.

## Recovered original behavior and current supported domain

Instruction/decompile inspection of SMAX `BasePop_calculate_dimensions`0x603800,
`Buffer_write_strings`0x5db040 and the real flying-wrap helpers0x5ddda0,
0x5ddde0,0x5ddfb0,0x5de1d0 establishes:

- Physical SCRIPT body lines of style0 continue the same flying paragraph,
  inserting one separating space; they are not separate screen paragraphs.
- Caret style1 flushes that paragraph and starts another.
- Style2 uses independently wrapped centered text.
- Style3 inserts one font line-height gap.

`Popup_on_draw_background`0x405c80 uppercases the caption through CharUpperA
before centered drawing. The portable stage supports ASCII uppercase and holds
non-ASCII captions, whose original casing needs actual runtime codepage facts.
Prepared parser/content bytes are preserved. Body bytes index the supplied font
metrics without invented Unicode/codepage conversions. Unsupported glyphs,
invalid metrics, aliases, over-capacity text and unbreakable oversized words
reject before changing the prior output. The independent port word wrapping
uses the supplied advances; original GDI line-break/inline-style/kerning behavior
has not been executed and is not claimed identical. No fake GDI callback exists.

All listed original routines were **inspected only in this stage**. The prior
complete original template helper evidence remains inherited and unchanged;
there is no new original popup/window/import execution proof here.

## Input/outcome contract — held at owning-session boundary

Actual original `BasePop_on_key_down`0x604410 tests live flags0x4000 and active
control/list selection before updating popup result0xa40. Actual
`BasePop_on_button_clicked`0x604300 reads live dialog selection, uses button IDs
-1/-2 and flags0x40/0x100/0x4000, writes result/command fields and invokes real
Win/object vtable lifecycle methods. `BasePop_exec_3`0x602600 creates live child
buttons and window objects; the original pump/paint path includes real OS input
and dispatch. These dependencies have not run. Supplying an Enter boolean or
an inspected result constant would not satisfy that lifecycle.

The root owning session must supply the next real integration:

1. Bind the pending continuation/content/view to its actual session and revision,
   without importing authority from saved human/visibility/runtime flags.
2. Create and display an actual owned UI view, preserving staged intermediate
   state and original resource identity. Font/raster/window integration needs
   native qualification, including focus, normal close and restoration.
3. Accept real window-system input only while that exact view/session is active.
   Match outcome to its pending event and revision; reject stale, reordered or
   unrelated events. Cancellation/window loss is not unconditional acceptance.
4. Consume the genuine owned UI outcome through a root-owned continuation API.
   It must not fabricate an original X_pop/import return, skip mandatory effects
   or claim original modal/network execution.
5. Complete subsequent source-backed log/object lifecycle and recursive awards
   before committing a complete technology event. First-Secrets still requires
   the real post-presentation log path and original object cleanup; reactor
   continuation still must preserve its ordered reset semantics.

No callback acknowledgement or outcome token is invented in this component.
The resulting layout stays pending even after host pixels have been produced.
The supported SCRIPT resources have no audio directive, so rendering never
selects or plays a guessed technology quote. Generic discovery FX/ambient music
and source log/destructor effects remain separate unfinished dependencies.

## Controls

The connected private probe retains all173 inherited World/content cases,112
prepared notifications, with full decoded/opaque input preservation, actual
SCRIPT bytes and old boundaries. It compares portable glyph positions with an
independent measured-font host reference, renders a private host example, and
checks caret paragraphs/centering/gaps, missing glyphs, invalid metrics, geometry,
aliases and transactional failures. Host ASan/UBSan and actual independent
Linux68040 execution must agree. Normal SDK builds are build-only; native and
real UI input/outcome integration remain UNRUN. All inherited workflows stay
byte-identical. No original fonts, text resources or private proof bytes enter
Git, and no lab/watch/frontend/export/publication action is performed.
