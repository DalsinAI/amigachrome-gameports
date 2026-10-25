# Original mineral and ecological accounting

`calculate_minerals` calculates one numeric base stage from explicit inputs. It returns mineral intake, consumption, surplus, pre-elevator final surplus, ecological damage, facility bonus and zero mineral inefficiency. Rejected states and original signed32 overflow leave the result unchanged. No world, turn or file is mutated.

Inputs are the **pre-stage** intake and terraforming ecological damage plus resolved support, convoy, faction, facility and project facts. A loaded save contains already calculated accounting values; feeding those values back through this stage would double-count bonuses. This API is not bound to saved world data until the preceding yield/support stages are recovered.

The implementation adapts GPL-3.0-or-later OpenSMACX `base.cpp::base_minerals` at `2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c` (Brendan Casey, 2013–2021). The test body keeps explicit fixture adapters. Three recovered unsigned temporaries are corrected to signed arithmetic, and atrocity/tectonic penalties are divided separately. These changes are backed by privately executed unchanged Alien Crossfire original instructions, rather than a claim that the recovered source is exact.

The explicit runtime edition selects satellite production IDs34..37 for Alpha Centauri or65..68 for Alien Crossfire. Base mode rejects nonzero tectonic counts because that term is absent from its original routine. Private original helper comparisons cover both editions independently.

The original amplifies incoming convoys, uses Nanoreplicator OR Singularity Inductor for ecological reduction, and leaves `final_surplus` at its pre-Space-Elevator value. These behaviours are retained instead of importing Thinker fixes.

The supported domain uses nonnegative resource/count inputs, population 0..127, native life 0..2, difficulty 0..5 for the relevant human/AI branch, and representable signed32 intermediate products. Satellites, planet and sea factors retain original clamping. Unsupported or overflowing inputs are rejected; original overflow behaviour is not emulated.

## Validation and remaining gate

Host ASan/UBSan checks all 32,768 facility/project/era/edition flag combinations, integer boundaries, explicit convoy/elevator and separate-penalty regressions: 33,043 cases (32,945 accepted, 83 invalid, 15 overflow). An independent Python integer implementation checks every output and failure sentinel. A GPL reference body, with the documented instruction corrections, checks every accepted result.

Private unchanged owner Alpha Centauri and Alien Crossfire `base_minerals`, `has_fac` and bitmask helpers execute separately under qemu-i386 with explicit fixture globals. All six original accounting outputs match for all 32,945 accepted contexts: 16,559 base contexts (397,416 bytes) and 16,386 expansion contexts (393,264 bytes). This does not execute original PE startup, Windows imports or a full game. No original executable bytes or proprietary decompile are shipped.

The normal private Amiga SDK probe compiles. Actual AmigaOS execution remains **unrun** while the user owns the interactive watch session. Independent big-endian execution is required in CI. The change remains a draft until the native gate passes.

Run `test_base_minerals.py --out NEW_DIRECTORY` for host controls, or add `--cc m68k-linux-gnu-g++ --m68k-linux` for the independent 68040 controls. `build_base_minerals_probe.sh OUTPUT` uses the normal Amiga SDK. The probe takes a bounded `MNR1` control file and a fresh output path. Its existing-output check is for private evidence; it is not an atomic gameplay save operation or an `O_EXCL` qualification.

Production completion, yield and support generation, ecology events, upkeep, research, AI, turn advancement and persistent player actions remain separate work.
