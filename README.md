
# PicoPeanutGB.

This software is a port of [Peanut-GB](https://github.com/deltabeard/Peanut-GB), a DMG Game Boy emulator for RP2350 based microcontroller boards like the RaspberryPi Pico 2. Sound and video are ouput over HDMI.
The code for HDMI output is based on [Shuichi Takano's Pico-InfoNes project](https://github.com/shuichitakano/pico-infones) which in turn is based on [PicoDVI](https://github.com/Wren6991/PicoDVI).

Put your Game Boy (.gb) rom files on a FAT32 formatted SD card. You can organize the roms in directories. A menu is displayed on which you can select the rom to play.

> [!NOTE]
> The emulator runs well on the Raspberry Pi Pico 2, but is too slow on the original Raspberry Pi Pico or other RP2040 based boards. 

## Work in progress

**This is a work in progress.**

> **WARNING** Some games show red flashing between screens. This can be occasionally or severe depending on the game. If you are sensitive for this, or experience health issues while playing those games, please stop playing immediately.

## System requirements - What do yo need?

A **Raspberry Pi Pico 2** on a Pimoroni Pico DV Deno Base, or a **Raspberry Pi Pico 2** on a breadboard. The emulator is too slow on the original Pico or other RP2040 based boards.

The binary specific for your config can be downloaded from the [releases](https://github.com/fhoedemakers/PicoPeanutGB/releases/latest) page.

You need a FAT32 formatted SD card to put your .gb roms on.

> For detailed instructions and specific configurations, see the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus). 

## Video

TODO


Also original NES and WII-classic controllers are supported in some configurations. See the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus) for more info.

## Menu Usage
Gamepad buttons:
- UP/DOWN: Next/previous item in the menu.
- LEFT/RIGHT: next/previous page.
- A (Circle): Open folder/flash and start game.
- B (X): Back to parent folder.
- START: Starts game currently loaded in flash.

## Emulator (in game)
Gamepad buttons:
- SELECT + START: Resets back to the SD Card menu. Game saves are saved to the SD card.
- SELECT + UP/SELECT + DOWN: switches screen modes.
- SELECT + A/B: toggle rapid-fire.
- START + A : Toggle framerate display.

## Building from source

When using Visual Studio code, make sure to build in Release or RelWithDbinfo mode, as the emulator is too slow in the other modes.

Build shell scripts are available:

- build.sh : Builds .uf2 for the Pimoroni DV Deno Base
- build_alternate.sh: For the PCB or breadboard variant
- build_feather_dvi.sh: For the Adafruit feather
- build_ws_rp2040_pizero.sh: For the Wavehare device

The _debug.sh scripts can be use to create a debug build for each system.
