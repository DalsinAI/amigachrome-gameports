# Alpha Centauri native rules-reader prototype

A bounded first implementation for separate Alpha Centauri and Alien Crossfire
ports. It reads **only the TECHNOLOGY section**, with 89 records, case-insensitive
keys, None/Disable sentinels, priorities, binary flags and resolved forward
prerequisites. It is not a game engine or launcher.

Derived from Brendan Casey's [OpenSMACX](https://github.com/b-casey/OpenSMACX),
revision `2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`:
`text.cpp` token interface and `alpha.cpp::read_tech` two-pass key resolution.
GPL-3.0-or-later; original licence retained in LICENSE. This is an adaptation,
not an unmodified upstream build. The caller owns the input and Rules state.
There are no Windows headers, fixed game addresses, original DLL calls, packed
native-memory records, dynamic allocation in the parser or rendering dependencies.

The reader accepts CRLF/LF/CR, an unterminated final line, blank/comment lines,
space/tab padding, original positive 0b/0x/0d number prefixes and signed decimal
values. It rejects missing/duplicate/oversized keys, unknown prerequisites,
overflow, truncated/extra rows and long or NUL-containing lines. It intentionally
rejects malformed numeric suffixes rather than copying atoi's permissive behavior.
No filename precedence/scenario resolver, string table or section cache is present.
An unsuccessful read leaves partial output; use state only after Error::ok.

## Controls

```sh
python3 ports/alpha-centauri/native/test_rules.py --out /tmp/alpha-host
python3 ports/alpha-centauri/native/test_rules.py --out /tmp/alpha-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
```

The host runs ASan/UBSan (leak checking disabled for sandbox compatibility).
The Linux cross control runs generated big-endian 68040 code under qemu-m68k.
Both use generated synthetic data; no original assets are fetched or included.
An independent Python reader fingerprints every name/key, priority, prerequisite
and flag with explicit byte order, so matching counts alone cannot pass.
Use --data /path/to/alpha.txt --data /path/to/alphax.txt for owner-provided data.

Private 25 October controls using the owner's CD files find 89 records in each,
77/86 active records and 136/152 prerequisite edges. The Amiga GCC 16.2.0-open3
build additionally passes a raw 68040/QEMU control against fingerprints of both
actual files. That raw control checks the parser's generated m68k instructions,
not AmigaOS startup. The actual AmigaOS command-line probe also passes both files in the existing
isolated lab (RC 0), and rejects a duplicate-key file with RC 20. The lab was
stopped; its ROM/module/startup were unchanged. No live-instance execution or
full-game qualification is claimed. The CI Linux ABI is not an AmigaOS ABI qualification.

The full rules, faction reader, gameplay algorithms, original-game differential
checks, UI/audio, saves and launchable game separation remain future work.
Neither GPL source licensing nor this prototype conveys rights to game assets.
