# CHANGELOG

# General Info

Only binaries for Pico2 (w) arm-s and Pico2 (w) Risc-V are available. Risc-v binaries start with pico2_riscv_. or pico2_w_riscv_ The emulator is too slow for Pico (rp2340) based boards.


For more info see the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus#pcb-with-raspberry-pi-pico-or-pico-2).

> [!NOTE]
The emulator is still in development and may have performance issues, causing some games to not run at full speed and red screen flicker. It is not cycle accurate, feature-complete, fully tested, or entirely stable. Maybe some games will not run at all.

> [!WARNING]
>  Some games show red flashing between screens. This can be occasionally or severe depending on the game. If you are sensitive for this, or experience health issues while playing those games, please stop playing immediately.


3D-printed case design for PCB: [https://www.thingiverse.com/thing:6689537](https://www.thingiverse.com/thing:6689537). 
For the latest two player PCB 2.0, you need:

- Top_v2.0_with_Bootsel_Button.stl. This allows for software upgrades without removing the cover. (*)
- Base_v2.0.stl
- Power_Switch.stl.

(*) in case you don't want to access the bootsel button on the Pico, you can choose Base_v2.0.stl


# v0.6 Release notes (this is a re-release)

- Releases now built with SDK 2.1.1
- Support added for Adafruit Metro RP2350 board. See README for more info. No RISCV support yet.
- Switched to SD card driver pico_fatfs from https://github.com/elehobica/pico_fatfs. This is required for the Adafruit Metro RP2350. Thanks to [elehobica](https://github.com/elehobica/pico_fatfs) for helping making it work for the Pimoroni Pico DV Demo board.
- Besides FAT32, SD cards can now also be formatted as exFAT.
- Nes controller PIO code updated by [@ManCloud](https://github.com/ManCloud). This fixes the NES controller issues on the Waveshare RP2040 - PiZero board. [#8](https://github.com/fhoedemakers/pico_shared/issues/8)
- Board configs are moved to pico_shared.

## Fixes
- Fixed Pico 2 W: Led blinking causes screen flicker and ioctl timeouts [#2](https://github.com/fhoedemakers/pico_shared/issues/2). Solved with in SDK 2.1.1
- WII classic controller: i2c bus instance (i2c0 / i2c1) not hardcoded anymore but configurable via CMakeLists.txt. 


All changes are in the pico_shared submodule. When building from source, make sure you do a **git submodule update --init** from within the source folder to get the latest pico_shared module.