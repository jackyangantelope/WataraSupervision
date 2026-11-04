# Validation, 2026-11-04

| Evidence | Result | Scope |
|---|---|---|
| Prior RP2350B 0.1.17 ON bench run | Basic physical pass, 2026-11-03 | Assembloids, 65,536 bytes, CRC32 `c71131e4`. 23,199 frames / 380.091122 s, 61.035364 FPS. Audio under/over, resync, scheduling resets and input/UART loss zero. User confirmed continuous sound and smooth picture. Whole-frame maximum 24.225 ms, so this is not a per-frame hard realtime guarantee. |
| Rebuilt baseline retest | Physical pass, separate capture | 6,593 frames / 108.024586 s, 61.032402 FPS. Audio under/over, resync and scheduling resets zero. User confirmed sound and controls normal. Mid-run capture does not independently prove cold boot or cartridge identity. |
| Public Fruit Jam build | Passed from independent clean clone | Flash 206,020 B, static RAM 451,932 B (86.20%). Physical behavior and runtime memory high-water remain unmeasured. |
| Public core host regression | Passed | External Assembloids cartridge only. Idle, keyboard and pad modes, 3,600 frames and 2,601,123 stereo sample pairs each, ASan/UBSan and reload passed. Keyboard and pad modes produced identical final RAM, pixel hash and PCM. |
| Public Fruit Jam hardware | Not tested | Board wiring, input and shared platform differ from the bench baseline. |

The prior tested firmware SHA-256 is
`fd4ad92cd65cf604c3047dc61f7847962c7ac69e1aae914b631c88d89a9dd3a6`.
This source release does not include that binary, game ROM, captured private
logs or build caches. The source preparation does not change the tested
baseline package. Other cartridges, 30-minute stability and physical public
board acceptance remain open.

The independent clone repeated all three host checks with matching results.
Both builds produced UF2 SHA-256
`2f76de228835073438e39265a39bba46a4d683583163330b4f6a6174b8505c37`.
Source selection and release blocking audits passed. Only documentation was
updated after the clean-clone build. No ROM, BIOS or generated binary is
tracked in the public repository.
