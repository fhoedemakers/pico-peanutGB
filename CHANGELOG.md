# CHANGELOG

# General Info

Only binaries for Pico2 arm-s and Pico2 Risc-V are available. Risc-v binaries start with pico2_riscv_. The emulator is too slow for Pico (rp2340) based boards.

- pico2_PicoPeanutGBPimoroniDV.uf2 : For the Pimoroni DV Deno Base with Pico2.
- pico2_PicoPeanutGBAdaFruitDVISD.uf2 : For the breadboard or PCB variant with Pico 2
- pico_nesPCB_v2.1.zip: PCB Design.

For more info see the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus#pcb-with-raspberry-pi-pico-or-pico-2).

>[!NOTE]
>There is no specific build for the Pico2 w because of issues with the display when blinking the led. Use the pico_2_ binaries instead. There is no blinking led on the Pico 2 w.

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


# v0.4 Release notes

## Features

- The menu now uses the entire screen resolution of 320x240 pixels. This makes a 40x30 char screen with 8x8 font possible instead of 32x29. This also fixes the menu not displaying correctly on Risc-v builds because of a not implemented assembly rendering routine in Risc-v.
- Updated NESPAD to have CLK idle HIGH instead of idle LOW. Thanks to [ManCloud](https://github.com/ManCloud). 
- Other minor changes.

## Fixes

- Menu now displaying correctly on Pico2 Risc-V builds.

All changes are in the pico_shared submodule. When building from source, make sure you do a **git submodule update --init** from within the source folder to get the latest pico_shared module.
