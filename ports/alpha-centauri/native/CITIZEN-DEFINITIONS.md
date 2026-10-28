# Original citizen definitions

`read_citizen_definitions` reads caller-owned `CITIZENS` data using a qualified
89-row technology table from the same file and an explicit edition tag.
Both original editions consume ten ordinal rows: seven specialists, then Drone,
Worker and Talent names. The specialist fields are prerequisite, obsolete
technology, economy, psych and labs. Technology IDs retain None (-1) and Disable
(-2). Numeric fields retain signed values without clamping.

The original loader consumes two names for every row, and the five remaining
fields only for the first seven. It does not consume the stock eighth flags
column. This reader likewise ignores unconsumed tails, including tails on the
three name-only rows. Commas delimit names; literal semicolons are retained.
Empty names are accepted. The last three owned rows explicitly initialize their
unused numeric members to -2/-2/0/0/0; the original loader does not assign those
members, so these defaults are not reconstructed executable state.

The owned boundary requires exactly ten rows and bounded names, rejects embedded
NULs and malformed or overflowing numeric tokens, and supports the existing
reader's decimal/0b/0x/0d syntax. These are deliberate checked-input policies,
not claims about the original permissive numeric conversion. It allocates no
heap memory. Input/output overlap and overflowing address ranges reject before
writes. Other failures set count to zero; discard all partial output.

Source inspection covers OpenSMACX alpha.cpp and both original main loader
bodies. The full original loaders have not executed in this qualification.
Technology availability, specialist choice and bonus arithmetic remain separate
operations. No World/session binding, accounting mutation, persistence, upkeep,
turn, UI, native execution or playable-game completion is provided here.

Run `python3 ports/alpha-centauri/native/test_citizen_definitions.py --out DIR`
for synthetic controls, or add `--cc m68k-linux-gnu-g++ --m68k-linux` for the
independent big-endian 68040 controls. Owner inputs are optional private local
arguments; no owner data is committed. `build_citizens_probe.sh` builds using
the normal SDK without overlays; compiling does not qualify AmigaOS execution.
