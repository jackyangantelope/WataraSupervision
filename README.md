# Watara Supervision for RP2350

Public source release based on the tested 0.1.17 fixed-ON emulator core.
The public hardware target is Adafruit Fruit Jam, using an RP2350 at 252 MHz
with 8 MiB PSRAM, HDMI audio/video and SD storage. No system BIOS is needed.
Game cartridges are supplied by the user and are not included.

## Validation

The prior RP2350B bench baseline passed basic testing with Assembloids:
23,199 frames in 380.091122 seconds, 61.035364 emulated FPS, zero reported
audio underflow/overflow, resync and scheduling resets. The user confirmed
continuous sound and smooth picture. A separate rebuilt-firmware retest
covered 6,593 frames in 108.024586 seconds, with sound and controls confirmed
normal. These are separate captures of the same baseline, not one continuous
test. Other cartridges and a 30-minute run remain unverified.

The public Fruit Jam build uses different board wiring and platform/input
integration. **It has not been tested on physical Fruit Jam hardware.**
See [VALIDATION.md](VALIDATION.md) for build and host results.

## Source and licenses

Devices derive from [libretro/Potator](https://github.com/libretro/potator)
at `227c5f6f3ce74d32e9002ce24c1420288559a860` (Unlicense).
The replacement CPU uses [vrEmu6502](https://github.com/visrealm/vrEmu6502)
at `aae98cb14386d832cb7357c99626520b6590bc24` (MIT), with local cycle batching.
The original restricted Potator CPU is excluded. The device integration,
five guarded loop batches, PC/opcode filtering, input mapping and original
audio scheduling come from the accepted baseline. The public build fixes all
batch/filter selectors ON. No replacement sound or music is added.

The shared platform derives from
[pico_shared](https://github.com/fhoedemakers/pico_shared) at
`df683bf00ff95e1ceda88f132bbf72f8cc7cadb0` with local changes.
Licenses, file-level changes and reproducible comparisons are documented in
[THIRD_PARTY.md](THIRD_PARTY.md) and `provenance/`.
Run `python scripts/verify-upstream.py` to fetch and compare exact public core
revisions; add `--write-diffs` to regenerate the comparison records.

## Build

Install Git, CMake 3.20+, Ninja, Python 3 and Arm GNU Toolchain 14.2.Rel1.
`dependencies.lock.json` pins Pico SDK 2.3.0, its TinyUSB revision and
Pico-PIO-USB. Obtain clean checkouts using:

```powershell
./scripts/fetch-dependencies.ps1 -Destination <outside-source-directory>
./scripts/build.ps1 -PicoSdkPath <pico-sdk> -PicoPioUsbPath <Pico-PIO-USB> `
  -ToolchainPath <arm-toolchain> -NinjaPath <ninja.exe>
```

The SDK can build or fetch pioasm and picotool. An offline build can pass
`-PioasmDir`, `-PicotoolDir`, `-CmakeExe` and `-PythonExe` explicitly.
The target is `adafruit_fruit_jam` (`HW_CONFIG=8`), and the output is
`build/fruit-jam/picoSupervision.uf2`. It is linked at `0x10080000` for a
compatible application loader with a 16 MiB flash layout; it is not a
standalone image at flash offset zero.

Fruit Jam uses HDMI data GPIO15/17/19 and clock GPIO13, SD SPI GPIO34/35/36
with CS GPIO39, USB D+/D- GPIO1/2 with power GPIO11, and SDK-default UART1
GPIO8/9. These connections and the emulator's public-board integration still
need physical validation. The public input implementation supports USB HID
keyboard/controller handling. The legacy direct NES/SNES adapter is disabled.

Place legally obtained `.sv` cartridges under `/roms/SUPERVISION` on the SD
card. Keyboard controls are arrows, Z or Space for A, X for B, Enter for
Start and Tab for Select. See `runtime/input_map.h` for the exact mapping.

## Host regression

On Linux/WSL install GCC/G++, Make and Python. Run:

```sh
bash scripts/test-host.sh /path/to/user-game.sv
python scripts/audit-source.py
```

The host check runs 3,600 frames in each of idle, keyboard and controller
modes, checks guard bytes and reload, and uses ASan/UBSan. Results, PCM and
preview files stay under ignored `build/`. Host execution does not establish
physical FPS, audio continuity or USB latency.
