
# PICOGB.

This software is a port of [SmsPlus](https://segaretro.org/SMS_Plus), a Sega Master System and Game Gear emulator for RP2040 based microcontroller boards like the RaspberryPi Pico. Sound and video are ouput over HDMI.
The code for HDMI output is based on [Shuichi Takano's Pico-InfoNes project](https://github.com/shuichitakano/pico-infones) which in turn is based on [PicoDVI](https://github.com/Wren6991/PicoDVI).

Put your Master System (.sms) and Game Gear (.gg) rom files on a FAT32 formatted SD card. You can organize the roms in directories. A menu is displayed on which you can select the rom to play.

## Work in progress

**This is a work in progress, and there are still some issues to be fixed. The emulator runs decently well. Some games may not run at full speed or at all. (afterburner).**

> **WARNING** Some games show red flashing between screens. This can be occasionally or severe depending on the game. If you are sensitive for this, or experience health issues while playing those games, please stop playing immediately.

## System requirements - What do yo need?

The binary specific for your config can be downloaded from the [releases](https://github.com/fhoedemakers/PICOGB/releases/latest) page.

You need a FAT32 formatted SD card to put your .sms and .gg roms on.

## For Raspberry Pi Pico / Pico W

- Raspberry Pi Pico and [Pimoroni Pico DV Demo Base](https://shop.pimoroni.com/products/pimoroni-pico-dv-demo-base?variant=39494203998291)
- [Custom Printed Circuit Board](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-raspberry-pi-pico)
- Breadboard with components.

## Other RP2040 based boards

These boards already contain an RP2040 cpu, a separate Raspberry Pi Pico is not needed.

- [Adafruit Feather RP2040 with DVI Output](https://www.adafruit.com/product/5710) and [FeatherWing - RTC + SD](https://www.adafruit.com/product/2922).
- [Waveshare RP2040-PiZero Development Board](https://www.waveshare.com/rp2040-pizero.htm)

> For more detailed instructions and specific configurations, see the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus). (For using a breadboard with components or how to use an Original NES or WII-classic controller.)

## Video
Click on image below to see a demo video.

[![Video](https://img.youtube.com/vi/__E8h2Ay3g8/0.jpg)](https://www.youtube.com/watch?v=__E8h2Ay3g8)


## Supported USB controller
The following controllers are supported.

- BUFFALO BGC-FC801
- SONY DUALSHOCK 4
- SONY DualSense

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

## Things to do (if possible):

- [ ] Improve performance
- [ ] Save states

