
# PicoPeanutGB, a DMG Game Boy and Game Boy Color emulator for RP2350/Raspberry Pi Pico 2

This software is a port of [Peanut-GB](https://github.com/deltabeard/Peanut-GB), a DMG and Game Boy Color emulator for RP2350 based microcontroller boards like the RaspberryPi Pico 2. Sound and video are ouput over HDMI.
The code for HDMI output is based on [Shuichi Takano's Pico-InfoNes project](https://github.com/shuichitakano/pico-infones) which in turn is based on [PicoDVI](https://github.com/Wren6991/PicoDVI).

Put your DMG Game Boy (.gb) or Game Boy Color (.gbc) rom files and optional [metadata](#using-metadata) on a FAT32 or exFAT formatted SD card. You can organize the roms in directories. A menu is displayed on which you can select the rom to play.

> [!NOTE]
> The emulator runs well on the Raspberry Pi Pico 2, but is too slow on the original Raspberry Pi Pico or other RP2040 based boards. Only releases for Pico 2 (RP2350) are available. Some Game Boy Color games have image and sound glitches.

## System requirements - What do yo need?

> [!NOTE]
> For detailed instructions and specific configurations, see the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus). 

### Hardware

A **Raspberry Pi Pico 2** on a Pimoroni Pico DV Deno Base, or a **Raspberry Pi Pico 2** on a breadboard or PCB. The emulator is too slow on the original Pico or other RP2040 based boards.

Other boards that can be used:
- [Adafruit Fruit Jam](https://www.adafruit.com/product/6200)
- [Adafruit Metro RP2350](https://www.adafruit.com/product/6003) or [Adafruit Metro RP2350 with PSRAM](https://www.adafruit.com/product/6267)
- [Pimoroni Pico Plus 2](https://shop.pimoroni.com/products/pimoroni-pico-plus-2?variant=42092668289107)
  Use the breadboard config or Pimoroni Pico DV Demo base. This board does not fit the PCB because of the SP/CE connector on back of the board.
  The PSRAM on the board is used in stead of flash to load the roms from SD.


The binary specific for your config and PCB gerber files can be downloaded from the [releases](https://github.com/fhoedemakers/pico-peanutGB/releases/latest) page.



## Video

TODO



# Gamepad and keyboard usage

Below the button mapping for different controllers. You can also use a USB-keyboard.

|     | (S)NES | Genesis | XInput | Dual Shock/Sense |
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
  - In-game toggle between default bezel, random bezel and themed bezel: **SELECT** + **Button1**

| Default | Random | themed |
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
