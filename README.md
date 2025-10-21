
# PicoPeanutGB.

This software is a port of [Peanut-GB](https://github.com/deltabeard/Peanut-GB), a DMG and Game Boy Color emulator for RP2350 based microcontroller boards like the RaspberryPi Pico 2. Sound and video are ouput over HDMI.
The code for HDMI output is based on [Shuichi Takano's Pico-InfoNes project](https://github.com/shuichitakano/pico-infones) which in turn is based on [PicoDVI](https://github.com/Wren6991/PicoDVI).

Put your DMG Game Boy (.gb) or Game Boy Color (.gbc) rom files and optional metadata on a FAT32 or exFAT formatted SD card. You can organize the roms in directories. A menu is displayed on which you can select the rom to play.

> [!NOTE]
> The emulator runs well on the Raspberry Pi Pico 2, but is too slow on the original Raspberry Pi Pico or other RP2040 based boards. Only releases for Pico 2 (RP2350) are available.

## System requirements - What do yo need?

A **Raspberry Pi Pico 2** on a Pimoroni Pico DV Deno Base, or a **Raspberry Pi Pico 2** on a breadboard or PCB. The emulator is too slow on the original Pico or other RP2040 based boards.

Other boards:
- [Adafruit Fruit Jam](https://www.adafruit.com/product/6200)
- [Adafruit Metro RP2350](https://www.adafruit.com/product/6003) or [Adafruit Metro RP2350 with PSRAM](https://www.adafruit.com/product/6267)
- [Pimoroni Pico Plus 2](https://shop.pimoroni.com/products/pimoroni-pico-plus-2?variant=42092668289107)
  Use the breadboard config or Pimoroni Pico DV Demo base. This board does not fit the PCB because of the SP/CE connector on back of the board.
  The PSRAM on the board is used in stead of flash to load the roms from SD.


The binary specific for your config can be downloaded from the [releases](https://github.com/fhoedemakers/pico-peanutGB/releases/latest) page.


> [!NOTE]
> For detailed instructions and specific configurations, see the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus). 

## Video

TODO

## Using metadata.

Download the metadata pack from the [releases page](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/SMSPlusMetadata.zip) It contains box art, game info and themed borders/bezels for many games. The metadata is used in the menu to show box art and game info when a rom is selected.  When the screensaver is started, random box art is shown. When in-game, themed borders/bezels are shown around the game screen.
- Download pack [here](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/SMSPlusMetadata.zip).  
  - Extract the zip contents to the **root of the SD card**.  
  - In the menu:  
    - Highlight a game and press **START** → show cover art and metadata.  
    - Press **SELECT** → show full game description.  
    - Press **B** → return to menu.  
    - Press **START** or **A** → start the game.

# Gamepad and keyboard usage

|     | (S)NES | Genesis | XInput | Dual Shock/Sence |
| --- | ------ | ------- | ------ | ---------------- |
| Button1 | B  |    A    |   A    |    X             |
| Button2 | A  |    B    |   B    |   Circle         |
| Select  | select | Mode or C | Select | Select     |

## in menu

- UP/DOWN: Next/previous item in the menu.
- LEFT/RIGHT: next/previous page.
- Button2 : Open folder/flash and start game.
- Button1 : Back to parent folder.
- START: Show metadata and box art (when available). 

The colors in the menu can be changed and saved:
  - Select + Up/Down changes the foreground color.
  - Select + Left/Right changes the background color.
  - Select + Button2 saves the colors. Screen will flicker when saved.
  - Select + Button1 resets the colors to default. (Black on white)


## Emulator (in game)

- SELECT + START, Xbox button: Resets back to the SD Card menu. Game saves are saved to the SD card.
- SELECT + UP/SELECT + DOWN: switches screen modes.
- SELECT + Button1: When metadata pack is installed on SDcard: Toggle between default bezel, random bezel or themed bezel. (according to the selected rom)
- START + Button2 : Toggle framerate display
- **Pimoroni Pico DV Demo Base only**: SELECT + LEFT: Switch audio output to the connected speakers on the line-out jack of the Pimoroni Pico DV Demo Base. The speaker setting will be remembered when the emulator is restarted.
- **Fruit Jam Only** 
  - pushbutton 1 (on board): Mute audio of built-in speaker. Audio is still outputted to the audio jack.
  - SELECT + UP: Toggle scanlines. 
  - pushbutton 2 (on board) or SELECT + RIGHT: Toggles the VU meter on or off. (NeoPixel LEDs light up in sync with the music rhythm)
- **Genesis Mini Controller**: When using a Genesis Mini 3 button controller, press C for SELECT. On the 8-button Genesis controllers, MODE acts as SELECT.
- **USB-keyboard**: When using an USB-Keyboard
  - Cursor keys: up, down, left, right
  - A: Select
  - S: Start
  - Z: Button1
  - X: Button2
 
## Building from source

When using Visual Studio code, make sure to build in Release or RelWithDbinfo mode, as the emulator is too slow in the other modes.

Build shell scripts are available:

- build.sh : Builds .uf2 for the Pimoroni DV Deno Base
- build_alternate.sh: For the PCB or breadboard variant
- build_feather_dvi.sh: For the Adafruit feather
- build_ws_rp2040_pizero.sh: For the Wavehare device

The _debug.sh scripts can be use to create a debug build for each system.
