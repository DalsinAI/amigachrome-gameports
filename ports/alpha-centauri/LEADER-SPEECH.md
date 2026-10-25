# Original faction leader speech recovery

This component recovers **14 original faction-selection speeches**: seven SMAC
factions and seven Alien Crossfire factions. It does not invent spoken diplomacy
for text-only conversations. Original assets remain private and are not included.

The event mapping is source-backed: Thinker at
`dabe62f2d1abba03de40f3dd3048494843f6a9af`, `src/game.cpp` lines 1680 and 1793
constructs `voices/<MFaction.filename>.mp3` during faction selection/new-game
configuration. Names and faction basenames are independently present in the
owner's original faction definition text files. The catalog maps these basenames,
not mutable player slot numbers. Base mode offers its seven original factions;
expansion mode offers all fourteen.

| Original basename | Leader | Available |
|---|---|---|
| gaians | Lady Deirdre Skye | SMAC / SMAX |
| hive | Chairman Sheng-Ji Yang | SMAC / SMAX |
| univ | Academician Prokhor Zakharov | SMAC / SMAX |
| morgan | CEO Nwabudike Morgan | SMAC / SMAX |
| spartans | Colonel Corazon Santiago | SMAC / SMAX |
| believe | Sister Miriam Godwinson | SMAC / SMAX |
| peace | Commissioner Pravin Lal | SMAC / SMAX |
| cyborg | Prime Function Aki Zeta-5 | SMAX |
| pirates | Captain Ulrik Svensgaard | SMAX |
| drone | Foreman Domai | SMAX |
| angels | Datajack Sinder Roze | SMAX |
| fungboy | Prophet Cha Dawn | SMAX |
| caretake | Caretaker Lular H'minee | SMAX |
| usurper | Usurper Judaa Marr | SMAX |

The stock ISO's fourteen MP3 streams are mono 22050 Hz, approximately 19.7–41.6
seconds. All fourteen passed strict host decoding without diagnostic errors.
This is integrity evidence, not native playback qualification or a transcript.

## Private preparation and audition

With an owner's original Planetary Pack ISO and local `7z`/`ffmpeg`:

```
python3 tools/recover_leader_speech.py OWNER.iso NEW_PRIVATE_DIRECTORY
CXX=/path/to/normal/m68k-amigaos-g++ native/build_leader_speech.sh leader-speech
```

On AmigaOS, `leader-speech` lists the catalog. For example:

```
leader-speech gaians PRIVATE_DIRECTORY
```

The audition reads `PRIVATE_DIRECTORY/voices/gaians.wav`, plays it **once**, then
exits. Host conversion explicitly produces PCM16LE mono 22050; the bounded
portable reader converts each sample to host order, including on big-endian
68040. This does **not** implement native MP3 decoding. Original MP3 files are
preserved. Missing/invalid media returns RC20; no synthesized replacement.

The selection lookup/PCM reader has no SDL dependency and can be reused by a
frontend. The audition owns its SDL device and must be launched only after the
parent closes/releases its audio device; pausing alone is not claimed to permit
multiple device ownership. No main frontend was modified in this scoped branch.
A shared mixer/device integration should own speech cancellation and music
restoration centrally. Current audition is a shell tool, not a diplomacy screen.

## Validation and remaining scope

`native/test_leader_speech.py` runs 105 catalog/variant/path-rejection, endian,
truncation, malformed-format, capacity and transactional-output controls under
host ASan/UBSan or independent big-endian 68040 Linux/QEMU. No original media is
needed in CI. The normal private Amiga SDK audition build compiles cleanly.
Actual isolated AmigaOS testing passed all 105 controls and every sample fingerprint
for all fourteen original WAV files against independent Python results. Deirdre's
and Judaa Marr's complete speeches played once, produced nonzero captured native
PCM, and exited RC0. Invalid faction selection returned RC20. The test used private
then-current target ROMs and SDL module, with 336 continuously polled display frames;
the lab stopped and configuration, ROMs, module and startup hashes were restored.
See `LEADER-SPEECH-EVIDENCE-20261025.json`. Initial-head CI actually ran and passed
all 105 cases on host and independent big-endian 68040. Final-head checks must also
pass before merge. Main frontend integration remains a separate step.

Technology quote recordings belong to the technology/discovery component.
Facility completion voices (`fac*`), secret-project narrations, computer warnings,
opening narration, procedural AMB music and cinematics are distinct categories.
The fourteen named files are not proof that arbitrary diplomatic conversations
have corresponding voice recordings.
