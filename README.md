
# PicoPeanutGB, a DMG Game Boy and Game Boy Color emulator for RP2350/Raspberry Pi Pico 2

This software is a port of [Peanut-GB](https://github.com/deltabeard/Peanut-GB), a DMG and Game Boy Color emulator for RP2350 based microcontroller boards like the RaspberryPi Pico 2. It also runs on the original RP2040 based Raspberry Pi Pico, in Game Boy (DMG) form only — see [Running on the Raspberry Pi Pico (RP2040)](#running-on-the-raspberry-pi-pico-rp2040). Sound and video are ouput over HDMI.
The code for HDMI output is based on [Shuichi Takano's Pico-InfoNes project](https://github.com/shuichitakano/pico-infones) which in turn is based on [PicoDVI](https://github.com/Wren6991/PicoDVI).

Put your DMG Game Boy (.gb) or Game Boy Color (.gbc) rom files,  and optional [metadata](#using-metadata) on a FAT32 or exFAT formatted SD card. Preferred location for roms: /roms/GB You can organize the roms in directories. A menu is displayed on which you can select the rom to play.

> [!NOTE]
> The emulator runs best on the Raspberry Pi Pico 2. Some Game Boy Color games have image and sound glitches.
> Releases for the original Raspberry Pi Pico (RP2040) are available again, but with real limitations — see [Running on the Raspberry Pi Pico (RP2040)](#running-on-the-raspberry-pi-pico-rp2040).

## System requirements - What do yo need?

> [!NOTE]
> For detailed instructions and specific configurations, see the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus). 

### Hardware

A **Raspberry Pi Pico 2** on a Pimoroni Pico DV Deno Base, or a **Raspberry Pi Pico 2** on a breadboard or PCB. An original **Raspberry Pi Pico** (RP2040) also works, with the limitations described [below](#running-on-the-raspberry-pi-pico-rp2040).

Other boards that can be used:
- [Adafruit Fruit Jam](https://www.adafruit.com/product/6200)
- [Adafruit Metro RP2350](https://www.adafruit.com/product/6003) or [Adafruit Metro RP2350 with PSRAM](https://www.adafruit.com/product/6267)
- [Pimoroni Pico Plus 2](https://shop.pimoroni.com/products/pimoroni-pico-plus-2?variant=42092668289107)
  Use the breadboard config or Pimoroni Pico DV Demo base. This board does not fit the PCB because of the SP/CE connector on back of the board.
  The PSRAM on the board is used in stead of flash to load the roms from SD.


The binary specific for your config and optional PCB gerber files can be downloaded from the [releases](https://github.com/fhoedemakers/pico-peanutGB/releases/latest) page.

## Running on the Raspberry Pi Pico (RP2040)

The emulator runs on the original Pico, but the RP2040 is close to its limit and
two things are given up to get there. Use a Pico 2 if you have one.

**Game Boy Color games are not supported.** RP2040 builds are DMG only. Colour
support needs about 35 KB more RAM than the 256 KB part can spare, and costs
roughly two thirds more work per scanline. Cartridges marked Game Boy Color only
are refused when you start them, with a message in the menu. Cartridges that are
Game Boy Color *enhanced* but still run on an original Game Boy work fine, in
their Game Boy (DMG) form.

**Some games still flicker.** The video pipeline has no framebuffer on RP2040:
the emulator hands finished scanlines to the display one at a time and has to
keep pace with it. Quiet scenes have room to spare, but busy ones — lots of
sprites, or scrolling with a window layer — can miss the deadline, which shows
up as brief red bands. It is not harmful and the picture recovers on its own,
but it is visible. How often depends entirely on the game and the scene.

Everything else behaves as it does on the Pico 2: sound, save games, borders,
palettes, the menu and the settings screen are all unchanged.

RP2040 images are the ones without `pico2` in the file name:

| Board | File |
| --- | --- |
| Pimoroni Pico DV Demo Base | `PicoPeanutGB_PimoroniDVI_pico_arm.uf2` |
| Adafruit DVI + microSD breakout, or the PCB | `PicoPeanutGB_AdafruitDVISD_pico_arm.uf2` |
| Adafruit Feather RP2040 DVI + SD Wing | `PicoPeanutGB_AdafruitFeatherDVI_arm.uf2` |
| Waveshare RP2040-PiZero | `PicoPeanutGB_WaveShareRP2040PiZero_arm.uf2` |



## Video

TODO



# Gamepad and keyboard usage

Below the button mapping for different controllers. You can also use a USB-keyboard.

|     | (S)NES | Genesis | XInput | Dual Shock/Sense |
| --- | ------ | ------- | ------ | ---------------- |
| Button1 | B (*)  |    A    |   A    |    X             |
| Button2 | A  |    B    |   B    |   Circle         |
| Select  | select | Mode or C | Select | Select     |

It is not advised to use a NES controller because it lacks enough buttons.

(*) On SNES USB-controller press Y once to activate the B-button.

## in menu

- UP/DOWN: Next/previous item in the menu.
- LEFT/RIGHT: next/previous page.
- Button2 : Open folder/flash and start game.
- Button1 : Back to parent folder.
- START: Show metadata and box art (when available). 
- SELECT: Opens a setting menu. Here you can change settings like screen mode, scanlines, framerate display, menu colors and other board specific settings. Settings can also be changed in-game by pressing some button combinations as explained below. The settings menu can also be opened in-game.

## Emulator (in game)

- SELECT + START, Xbox button: opens the settings menu. From there, you can:
  - Quit the game and return to the SD card menu
  - Adjust settings and resume your game.
- SELECT + UP/SELECT + DOWN: switches screen modes.
- SELECT + Button1: When metadata pack is installed on SDcard: Toggle between default bezel, random bezel or themed bezel. (according to the selected rom)
- START + Button1: When playing a DMG Game, toggle between green, color and greyscale palette.
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

## Using metadata.

<img width="640" height="360" alt="image" src="https://github.com/user-attachments/assets/f6aeb7cd-702b-4064-a69c-e8de36dcb6be" />

Download the metadata pack from the [releases page](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/GBMetadata.zip) It contains box art, game info and themed borders/bezels for many games. The metadata is used in the menu to show box art and game info when a rom is selected.  When the screensaver is started, random box art is shown. When in-game, themed borders/bezels are shown around the game screen.
- Download pack [here](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/GBMetadata.zip).  
  - Extract the zip contents to the **root of the SD card**.  
  - In the menu:  
    - Highlight a game and press **START** → show cover art and metadata.  
    - Press **SELECT** → show full game description.  
    - Press **Button1** → return to menu.  
    - Press **START** or **Button2** → start the game.
  - In-game toggle between different border modes: **SELECT** + **Button1**

| Super Gameboy Default | Super Gameboy Random | Game-Specific |
| ------- | ------ | ------ |
| <img width="320" height="180" alt="image" src="https://github.com/user-attachments/assets/8d15a58f-9343-47ea-b940-58784d7a6071" /> | <img width="320" height="180" alt="image" src="https://github.com/user-attachments/assets/a6ed915f-315e-4813-8803-5b7e21bb041e" />  | <img width="320" height="180" alt="image" src="https://github.com/user-attachments/assets/ed6fbd45-ef3d-4339-8f95-3092da6e8f95" />  |

  - In-game playing a DMG Game Boy game only. (Not Gameboy color). Toggle between Green, Color and greyscale palette: **START** + **Button1**

| green | color | grayscale |
| ------- | ------ | ------ |
| <img width="320" height="180" alt="image" src="https://github.com/user-attachments/assets/537a38b9-350b-470d-8a90-22ad86101fac" /> | <img width="320" height="180" alt="image" src="https://github.com/user-attachments/assets/9cadbdad-235e-45b9-bcfe-08f8b7d5caa0" /> | <img width="320" height="180" alt="image" src="https://github.com/user-attachments/assets/62d820dc-6cc6-4ebf-889a-05f279109c85" /> |

## Building from source

Use the bld.sh script to build the project. Build using Ubuntu Linux or WSL on Windows. See the Pico SDK installation instructions on how to set up the build environment.

Use ./bld.sh --h for options.

The resulting .uf2 file will be in the releases/ folder. Copy it to the Pico when in bootloader mode.
