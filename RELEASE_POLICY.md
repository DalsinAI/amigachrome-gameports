# Release Policy

For this repository, **done means ready for release**.

A port may not be labelled DONE or RELEASE merely because it compiles or reaches first light. A release requires:

- pinned and reproducible source provenance;
- a clean build using the current OpenUp/Open* interfaces;
- no private replacement SDL/Mesa/audio/input stack when the Open platform supplies that service;
- runtime qualification on a clean target instance;
- graphics, audio, keyboard, mouse/controller input, persistence and clean shutdown exercised;
- licence/notices and redistributable-data policy verified;
- a complete package, checksums and run/install documentation;
- known release blockers at zero.

Intermediate states are **COMPILED**, **FIRST LIGHT**, or **TESTING**. They are never synonyms for done.
