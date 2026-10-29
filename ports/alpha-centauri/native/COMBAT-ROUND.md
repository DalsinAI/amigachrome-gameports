# Combat round numeric stage

Explicit caller-owned CRT state is separate from movement RNG. The original CRT
transition uses 214013 and 2531011, returning bits16..30. It is neither session
initialization nor a per-unit/combat reset. A future owning controller must obtain,
retain and persist genuine CRT state without substituting the movement/map seed.

The checked stage computes artifact/reactor health, conditionally draws attacker
then defender rolls, gives ties to the defender, and applies normal1 or psi raw
recipient-reactor byte damage with original uint8 wrap. Zero raw reactor still
has minimum power10 but contributes zero psi damage. Hit branch does not imply
damage changed. Dead initial participants consume no random draws.

18024 synthetic controls compare independent Python bytes on host ASan/UBSan and
Linux68040. Private originals:32848 unchanged CRT integer-tail calls per edition
(after unexecuted Windows TLS getter);4096 supplied-roll post-draw decision/write
fragments per edition, each with two explicit observation exits before UI/death;
8192 complete power helper returns per edition and8192 complete SMAX health
returns. Neither full CRT/TLS nor full fight nor original draw-callsite gating
was executed. No original media/code bytes belong in this repository.

Negative strengths and nonbyte profiles are rejected transactionally. No legal
attack, retreat, death/removal, actor/RNG/session commit, player control or complete
battle is provided. Normal Amiga SDK builds only; native and remote CI UNRUN.
