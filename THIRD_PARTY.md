# Attribution and modifications

The combined source distribution carries GPL-3.0-only in root `LICENSE`
because the selected shared platform is GPL-3.0-only. Individual components
retain their own license and notices; this does not relicense them.
New integration files carry BSD-3-Clause notices (`LICENSE.BSD-3-Clause`),
and CPU batching/filter files carry MIT notices (`deps/vrEmu6502/LICENSE`).

| Component | Exact revision and license | Included code and changes |
|---|---|---|
| [Potator](https://github.com/libretro/potator) | `227c5f6f3ce74d32e9002ce24c1420288559a860`, Unlicense | Selected controls, GPU, memory map, sound, timer and headers in `deps/potator/`. CPU integration now calls the MIT replacement through `sv_cpu.h`; cartridge DMA wraps at cartridge size; scanline rendering supports SRAM placement and bounded batching. Original restricted CPU and `watara.c` orchestration are excluded. `runtime/sv_runtime.c` replaces machine orchestration. See `provenance/potator-changes.diff`. |
| [vrEmu6502](https://github.com/visrealm/vrEmu6502) | `aae98cb14386d832cb7357c99626520b6590bc24`, MIT | Selected CPU C/header and LICENSE. Local base changes provide cycle execution and SRAM placement. `runtime/sv_*reference.inc` and `sv_pcfilter_core.c` build guarded CMP/BEQ, threshold, DEX, input-poll and key-release batches, plus PC/opcode filters. Guards preserve interrupt exits and caller cycle budgets. See `provenance/vrEmu6502-changes.diff`. |
| [pico_shared](https://github.com/fhoedemakers/pico_shared) | `df683bf00ff95e1ceda88f132bbf72f8cc7cadb0`, GPL-3.0-only root | Selected HDMI, FatFs and USB support. HDMI double buffering and bounded audio queue changes are recorded in `provenance/pico_shared-changes.diff`. Embedded FatFs and SD wrapper license notices remain intact. |
| [font8x8](https://github.com/dhepper/font8x8) | Public domain; source SHA-256 `49d8df366296b203ca3211bc0672cf2a762135bf12710735b6292756b19dffd5` | ASCII extraction, retained attribution in `platform/text/font_8x8.h`. |
| Pico SDK / TinyUSB / Pico-PIO-USB | Public revisions in `dependencies.lock.json` | External build dependencies; not vendored. Their own licenses accompany the fetched checkouts. |

`dependencies.upstream.json` records the component pins. The core comparison
script fetches those exact public archives and records selected upstream paths,
source hashes (normalized to LF) and unified diffs. It never imports game data or the restricted
CPU. Baseline-specific package paths and development hardware are excluded.

The public frontend, linker layout and CMake configuration are in `firmware/`,
`platform/`, `cmake/` and the root CMake file. Core comment cleanup preserves
code tokens, original dependency licenses and third-party attribution.
