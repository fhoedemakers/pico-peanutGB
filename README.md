
# PicoPeanutGB, a Game Boy and Game Boy Color emulator for the Raspberry Pi Pico 2 (RP2350) and Pico (RP2040)

PicoPeanutGB is a Game Boy (DMG) and Game Boy Color emulator for RP2350 based microcontroller boards like the Raspberry Pi Pico 2, with picture and sound over HDMI and games loaded from an SD card. It also runs on the original RP2040 based Raspberry Pi Pico, in Game Boy (DMG) form only — see [Running on the Raspberry Pi Pico (RP2040)](#running-on-the-raspberry-pi-pico-rp2040).

The emulator core is a port of [Peanut-GB](https://github.com/deltabeard/Peanut-GB), a single header Game Boy (DMG) emulator written in C by Mahyar Koshkouei. Game Boy Color support comes from the [gbc-rtc-fix branch](https://github.com/tvecera/Peanut-GB/tree/gbc-rtc-fix) by [@tvecera](https://github.com/tvecera), the work behind Peanut-GB [PR #93](https://github.com/deltabeard/Peanut-GB/pull/93).

Video output uses the RP2350's HSTX hardware where the board configuration supports it, using the [pico_hdmi driver](https://github.com/fliperama86/pico_hdmi) by [@fliperama86](https://github.com/fliperama86). The remaining configurations use PicoDVI, based on [Shuichi Takano's Pico-InfoNes project](https://github.com/shuichitakano/pico-infones), which in turn is based on [PicoDVI](https://github.com/Wren6991/PicoDVI). Sound normally travels with the picture in the HDMI stream; boards with a DAC or line-out jack can output it separately instead.

Put your DMG Game Boy (.gb) or Game Boy Color (.gbc) rom files,  and optional [metadata](#using-metadata) on a FAT32 or exFAT formatted SD card. Preferred location for roms: /roms/GB You can organize the roms in directories. A menu is displayed on which you can select the rom to play. The last 20 games you started are kept in a [recently played list](#recently-played-games), one button press away in the menu.

> [!NOTE]
> The emulator runs best on the Raspberry Pi Pico 2. Some Game Boy Color games have image and sound glitches.
> Releases for the original Raspberry Pi Pico (RP2040) are available again, but with real limitations — see [Running on the Raspberry Pi Pico (RP2040)](#running-on-the-raspberry-pi-pico-rp2040).

## System requirements - What do you need?

> [!NOTE]
> For detailed instructions and specific configurations, see the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus). 

### Hardware

One of the boards below, a display with an HDMI input and an SD card. A USB game controller is the easiest way to play, but legacy controllers work too — see [Gamepad and keyboard usage](#gamepad-and-keyboard-usage).

The RP2350 boards play both Game Boy and Game Boy Color games. The RP2040 boards play Game Boy (DMG) games only, with the limitations described [below](#running-on-the-raspberry-pi-pico-rp2040).

#### RP2350 boards (Game Boy and Game Boy Color)

| Configuration | Setup | Notes |
| --- | --- | --- |
| **Raspberry Pi Pico 2 / Pico 2 W** on a breadboard with the [Adafruit DVI breakout](https://www.adafruit.com/product/4984) and [Adafruit microSD breakout](https://www.adafruit.com/product/254) | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) | No soldering required. |
| **Raspberry Pi Pico 2 / Pico 2 W** on the [PicoNES PCB](#picones-pcb) | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-raspberry-pi-pico-or-pico-2-and-pimoroni-pico-plus-2) | Same binary as the breadboard build. Console-style carrier with controller ports and an optional 3D printed case. |
| **Raspberry Pi Pico 2** on a [Pimoroni Pico DV Demo Base](https://shop.pimoroni.com/products/pimoroni-pico-dv-demo-base?variant=39494203998291) | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-for-pimoroni-pico-dv-demo-base) | Audio can be switched to the line-out jack. Use the Pico 2 binary on a Pico 2 W as well: the onboard LED driver conflicts with I2S audio here, so no W build is published. |
| [**Pimoroni Pico Plus 2**](https://shop.pimoroni.com/products/pimoroni-pico-plus-2?variant=42092668289107) | Use the breadboard or Pimoroni Pico DV Demo Base setup | Roms are loaded into the onboard PSRAM instead of flash. Fits the [PicoNES PCB](#picones-pcb) from design v2.6 onwards, provided male headers are soldered on. |
| [**Adafruit Metro RP2350**](https://www.adafruit.com/product/6003), also [with PSRAM](https://www.adafruit.com/product/6267) | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#adafruit-metro-rp2350) | With PSRAM fitted, roms are loaded into PSRAM instead of flash. Works fine without it. |
| [**Adafruit Fruit Jam**](https://www.adafruit.com/product/6200) | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#adafruit-fruit-jam) | Nothing else needed but a USB gamepad. Built-in speaker with volume control and a NeoPixel VU meter. Roms are loaded into PSRAM. |
| [**Waveshare RP2350-PiZero**](https://www.waveshare.com/rp2350-pizero.htm) | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#waveshare-rp2040rp2350-pizero-development-board) | Supports the optional PSRAM chip. [3D printed case](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#3d-printed-case-for-rp2040rp2350-pizero) available. |
| **Waveshare RP2350-Zero** on the [PicoNES Mini PCB](#picones-mini-pcb) | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-waveshare-rp2040rp2350-zero) | PCB required, advanced soldering. |
| **Waveshare RP2350-USB-A** on the [PicoNES Micro PCB](#picones-micro-pcb) | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-waveshare-rp2350-usb-a) | PCB required, USB controller only. The most demanding of the three builds. |
| [**SpotPear HDMI**](https://spotpear.com/index/product/detail/id/1207.html) | — | No setup section; wire the board according to its own documentation. |
| **Murmulator M1** with a Pico 2 | — | See [murmulator.ru](https://murmulator.ru). Untested by me, please report any issues. |
| **Murmulator M2** | — | RP2350 only. See [murmulator.ru](https://murmulator.ru). Untested by me, please report any issues. |

#### RP2040 boards (Game Boy only)

| Configuration | Setup |
| --- | --- |
| **Raspberry Pi Pico** on a breadboard with the [Adafruit DVI breakout](https://www.adafruit.com/product/4984) and [Adafruit microSD breakout](https://www.adafruit.com/product/254), or on the [PicoNES PCB](#picones-pcb) | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |
| **Raspberry Pi Pico** on a [Pimoroni Pico DV Demo Base](https://shop.pimoroni.com/products/pimoroni-pico-dv-demo-base?variant=39494203998291) | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-for-pimoroni-pico-dv-demo-base) |
| [**Adafruit Feather RP2040 DVI**](https://www.adafruit.com/product/5710) with an SD reader | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#adafruit-feather-rp2040-with-dvi-hdmi-output-port-setup) |
| [**Waveshare RP2040-PiZero**](https://www.waveshare.com/rp2040-pizero.htm) | [Setup](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#waveshare-rp2040rp2350-pizero-development-board) |

The binary specific for your config and optional PCB gerber files can be downloaded from the [releases](https://github.com/fhoedemakers/pico-peanutGB/releases/latest) page.

## Custom PCBs

Three community PCB designs turn a supported board and its breakouts into a finished little console, each with an optional 3D-printed case. They are simply a neater way to build hardware this emulator already supports, so nothing changes in the firmware: flash the binary for that configuration and you are done.

| Design | Board it carries | Build | Gerber archive | Designed by |
| --- | --- | --- | --- | --- |
| [PicoNES](#picones-pcb) | Pico 2, Pico 2 W, Pimoroni Pico Plus 2 or an original Pico | `-c2` | `pico_nesPCB_v2.6.zip` | John Edgar Park |
| [PicoNES Mini](#picones-mini-pcb) | Waveshare RP2350-Zero | `-c6` | `Gerber_PicoNES_Mini_PCB_v2.0.zip` | Gavin Knight |
| [PicoNES Micro](#picones-micro-pcb) | Waveshare RP2350-USB-A | `-c9` | `Gerber_PicoNES_Micro_v1.2.zip` | Gavin Knight |

All three archives are attached to every [release](https://github.com/fhoedemakers/pico-peanutGB/releases/latest) of this project and also live in [pico_shared/PCB](https://github.com/fhoedemakers/pico_shared/tree/main/PCB). Upload the zip as-is to a PCB manufacturer of your choice; [PCBWay](https://www.pcbway.com/) and JLCPCB are both good options.

The designs come from [pico-infonesPlus](https://github.com/fhoedemakers/pico-infonesPlus) and kept their NES-flavoured names, but there is nothing NES-specific about them — they are DVI, microSD and controller wiring, and this emulator runs on them just as well.

> [!NOTE]
> Sellers on AliExpress have copied the PicoNES design and sell ready-made boards. For questions about those, contact the seller.

### PicoNES PCB

The original design, by [@johnedgarpark](https://twitter.com/johnedgarpark). It carries the Pico, the DVI and microSD breakouts and up to two NES controller ports. It is also the only one of the three that takes an interchangeable Pico-format board, which is what makes a Pimoroni Pico Plus 2 — and with it PSRAM — an option, and the only one that can carry an original RP2040 Pico. The current design is **v2.6**.

<img width="480" alt="Populated PCB with a Pico plugged into the through-holes" src="https://github.com/user-attachments/assets/2bbc846d-56b1-4528-9899-01bc9b32ce11" />

#### Mounting the Pico

Design v2.6 added through-holes, so there are now two ways to fit the board:

| Mounting | Boards | Design version |
| --- | --- | --- |
| Soldered flat onto the PCB, no headers | Pico 2, Pico 2 W, Pico | any |
| Male headers plugged into the through-holes | Pico 2, Pico 2 W, Pico, Pimoroni Pico Plus 2 | v2.6 or later |

> [!IMPORTANT]
> A [Pimoroni Pico Plus 2](https://shop.pimoroni.com/products/pimoroni-pico-plus-2?variant=42092668289107) needs v2.6 **and** male headers. On v2.1 and older designs the board has to lie flat against the PCB, which the SP/CE connector on the back of the Pimoroni Pico Plus 2 prevents.

> [!NOTE]
> Soldering skills are required. Solder every connection from the Pico to the PCB, including the ones on the short right-hand side of the board — those are ground.

#### What you need

- One of the following, mounted as described above:
  * Raspberry Pi Pico 2 or Pico 2 W **without headers**, soldered flat.
  * Raspberry Pi Pico 2, Pico 2 W or [Pimoroni Pico Plus 2](https://shop.pimoroni.com/products/pimoroni-pico-plus-2?variant=42092668289107) **with male headers** soldered on ([these](https://a.co/d/dSNPuyo) fit), plugged into the through-holes.
  * An original Raspberry Pi Pico, either way — Game Boy (DMG) games only, see [Running on the Raspberry Pi Pico (RP2040)](#running-on-the-raspberry-pi-pico-rp2040).
- [Adafruit DVI Breakout Board — For HDMI Source Devices](https://www.adafruit.com/product/4984)
- [Adafruit Micro SD SPI or SDIO Card Breakout Board — 3V ONLY!](https://www.adafruit.com/product/4682)
- For a controller on the GPIO port:
  * [one NES controller port](https://www.zedlabz.com/products/controller-connector-port-for-nintendo-nes-console-7-pin-90-degree-replacement-2-pack-black-zedlabz) (the PCB has room for two, see below)
  * a NES or SNES controller
- [Micro USB to OTG Y-cable](https://a.co/d/b9t11rl) if you want to use a USB game controller — it powers the board and connects the controller at the same time.
- Micro USB power supply.
- Optional: an on/off switch, such as [this one](https://www.kiwi-electronics.com/en/spdt-slide-switch-410?search=KW-2467).

#### About the second controller port

A Game Boy is a single player machine, so only port 1 drives the emulator. A controller in port 2 still works in the menu, but not in a game — populating one port is enough unless you also run [pico-infonesPlus](https://github.com/fhoedemakers/pico-infonesPlus) or another two-player emulator on the same board.

> [!NOTE]
> A plain NES controller is short of buttons here, as noted under [Gamepad and keyboard usage](#gamepad-and-keyboard-usage). The sockets speak the SNES protocol as well, so an SNES pad with a [SNES-to-NES adapter cable](https://nl.aliexpress.com/item/1005007923169070.html) — [or one you make yourself](http://www.neshq.com/hardmods/snes_to_nes_controller.txt) — is the better choice.

#### Which binary to flash

- Pico 2 **and** Pimoroni Pico Plus 2 — `PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2`
- Pico 2 W — `PicoPeanutGB_AdafruitDVISD_pico2_w_arm.uf2`
- Original Pico — `PicoPeanutGB_AdafruitDVISD_pico_arm.uf2` (Game Boy only)

The Pimoroni Pico Plus 2 needs no separate build. The emulator reads the real flash size from the chip at boot and detects PSRAM at runtime, so the same `pico2` image adapts to whichever board is plugged in.

#### What the Pimoroni Pico Plus 2 adds

The Pimoroni Pico Plus 2 brings 8 MB of PSRAM and 16 MB of flash. The PSRAM is what you notice: roms are loaded into it and a game starts the moment you select it, instead of after the wait a plain Pico 2 needs to write the rom to its flash.

#### 3D printed case

Gavin Knight ([DynaMight1124](https://github.com/DynaMight1124)) designed an NES-like enclosure for this PCB: [thingiverse.com/thing:6689537](https://www.thingiverse.com/thing:6689537). The v2.0 design has a base, a power-switch part and a choice of two top covers — one with a button that reaches the BOOTSEL button so firmware can be updated without opening the case, one without. Print the files that match the PCB version you own; Gavin's Thingiverse page has the details.

> [!IMPORTANT]
> If the Pico is mounted with male headers, download the **latest** top cover. Headers raise the Pico, and only the newest cover leaves room for the USB cable — the older ones assume a Pico soldered flat onto the PCB.

<img width="480" alt="Top cover with a button for BOOTSEL" src="https://github.com/user-attachments/assets/3c8f8990-51b9-4873-9054-64bb2cd6c300" />

For the full photo gallery and assembly detail, see the [PCB section of the pico-infonesPlus documentation](https://github.com/fhoedemakers/pico-infonesPlus#pcb-with-raspberry-pi-pico-or-pico-2-and-pimoroni-pico-plus-2).

### PicoNES Mini PCB

A smaller take on the same idea by Gavin Knight ([DynaMight1124](https://github.com/DynaMight1124)), built around a Waveshare RP2350-Zero and two NES controller ports. It uses cheaper but considerably harder to solder parts, so it is a more advanced project than the PicoNES — if you are unsure of your soldering, start with that one instead. The current design is **v2.0** (`Gerber_PicoNES_Mini_PCB_v2.0.zip`), which improved the SD slot and the components around the HDMI port.

Flash `PicoPeanutGB_WaveShareRP2350ZeroWithPCB_arm.uf2`. The design also exists in an RP2040-Zero flavour, for which this project publishes no binary — use an RP2350-Zero.

> [!NOTE]
> Good soldering skills are required, especially around the HDMI portion: plenty of flux, a fine tip and solder wick. The recommended order is the resistor arrays first, then the HDMI port, then the Pico or the microSD adaptor, and the NES ports last — they can be hard to push into the PCB.

The build guide and the full component list are on Instructables: <https://www.instructables.com/PicoNES-RaspberryPi-Pico-Based-NES-Emulator/>

<img width="480" alt="Soldered PicoNES Mini PCB" src="https://github.com/user-attachments/assets/13933b1d-af00-402e-a0a0-8456de4a82da" />

#### 3D printed case for the Mini

Also by Gavin Knight: [thingiverse.com/thing:7041536](https://www.thingiverse.com/thing:7041536). The same page still carries the older v1.0 PCB design files, gerber and BOM. Without a printer of your own, a local printing service or a professional one such as PCBWay or JLCPCB will produce it — the professional finishes are excellent.

<img width="480" alt="PicoNES Mini in its 3D-printed case" src="https://github.com/user-attachments/assets/732384bd-062d-43ca-97cb-a16a39607c41" />

### PicoNES Micro PCB

The smallest of the three, again by Gavin Knight: a Waveshare RP2350-USB-A board on a PCB barely larger than the USB port itself, with a single player controlling the console over USB — which is all a Game Boy emulator needs. The current design is **v1.2** (`Gerber_PicoNES_Micro_v1.2.zip`).

Flash `PicoPeanutGB_WaveShare2350USBA_arm_piousb.uf2`. The game controller plugs into the USB-A port; the USB-C port is for power and for flashing the firmware.

> [!NOTE]
> Because of the size, micro-soldering skills are required — the design uses 0603 SMD components. This is the most demanding of the three builds.

The build guide is on Instructables: <https://www.instructables.com/PicoNES-RaspberryPi-Pico-Based-NES-Emulator/>

<img width="480" alt="PicoNES Micro populated PCB, NES controller shown for scale" src="https://github.com/user-attachments/assets/59c8a31b-dc3e-47b0-8ffb-89e1eab2a75b" />

<img width="480" alt="PicoNES Micro in its 3D-printed case" src="https://github.com/user-attachments/assets/1d6051f2-1393-40e1-aad0-e39ffb7717a0" />

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



# Gamepad and keyboard usage

Below the button mapping for different controllers. You can also use a USB-keyboard.

|     | (S)NES | Genesis | XInput | Dual Shock/Sense | Wii Classic |
| --- | ------ | ------- | ------ | ---------------- | ----------- |
| Button1 | B (*)  |    A    |   A    |    X             |   B         |
| Button2 | A  |    B    |   B    |   Circle         |   A         |
| Button3 | X (SNES only) | C | Y | Triangle    |   X         |
| Select  | select | Mode or C | Select | Select     |   Select    |

It is not advised to use a NES controller because it lacks enough buttons.

(*) On SNES USB-controller press Y once to activate the B-button.

> [!NOTE]
> An original NES controller has no Button3. Everything reachable with it can also be reached from the settings menu.

## in menu

- UP/DOWN: Next/previous item in the menu.
- LEFT/RIGHT: next/previous page.
- Button2 : Open folder/flash and start game.
- Button1 : Back to parent folder.
- Button3 : Open the [recently played list](#recently-played-games).
- START: Show metadata and box art (when available). 
- SELECT: Opens a setting menu. Here you can change settings like screen mode, scanlines, framerate display, menu colors and other board specific settings. Settings can also be changed in-game by pressing some button combinations as explained below. The settings menu can also be opened in-game.

## Recently played games

The menu keeps a list of the **last 20 games you started**, most recent first. Open it with **Button3** in the menu, or with the **Recently played** entry at the top of the settings menu. That entry is only there when the settings menu is opened from the menu — a game cannot be started from inside a running game.

> [!NOTE]
> On an original 3-button Genesis Mini controller, C acts as SELECT and opens the settings menu instead. Take the **Recently played** entry there.

In the list:

| Button | Action |
| ------ | ------ |
| UP/DOWN | Select a game. |
| Button2 | Start the highlighted game. |
| Button1 | Close the list and return to the menu. |
| SELECT | Remove the highlighted game from the list. Asks for confirmation first. This only removes the entry, the rom on the SD card is left alone. |
| START | Show [metadata](#using-metadata) and box art (when available). |

Games are added to the list automatically when you start them, so nothing has to be enabled. Starting a game that is already in the list moves it back to the top. The list closes by itself after a minute without input.

The list is kept in **`/recent_GB.txt`** in the root of the SD card, as plain text with one game per line. It survives a reboot and can be read, edited or deleted on a PC. Deleting the file simply empties the list, and a damaged file is treated as an empty list — unlike the settings file, nothing else is reset. Each emulator running under pico-bootLoader keeps its own list.

If a game was moved, renamed or deleted on the SD card in the meantime, the list says so instead of starting it. Use SELECT to remove such an entry.

On boards **without** PSRAM, one entry can be tagged **[READY]**. That is the game whose rom is currently written to flash, which is the one that starts without waiting for the flashing step described [above](#what-the-pimoroni-pico-plus-2-adds).

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
  - C: Button3. In the menu this opens the [recently played list](#recently-played-games).

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

## Credits

This emulator is other people's work brought together on a Pico.

**Emulation**

- [Peanut-GB](https://github.com/deltabeard/Peanut-GB) by **Mahyar Koshkouei** ([@deltabeard](https://github.com/deltabeard)) — the Game Boy emulator core this project is built on. Parts of it come from [SameBoy](https://github.com/LIJI32/SameBoy) by **Lior Halphon**, marked as such in the source.
- **Game Boy Color support** — [@tvecera](https://github.com/tvecera) brought CGB emulation to Peanut-GB in [PR #93](https://github.com/deltabeard/Peanut-GB/pull/93), combining the CGB work of [@froggestspirit](https://github.com/froggestspirit) with real-time-clock fixes, correct CGB sprite priority and double-speed serial timing. This project builds on the [gbc-rtc-fix branch](https://github.com/tvecera/Peanut-GB/tree/gbc-rtc-fix) of that fork.
- **minigb_apu**, the sound chip emulation shipped with Peanut-GB, is based on [MiniGBS](https://github.com/baines/MiniGBS) by **Alex Baines**.

**Video and sound output**

- [pico_hdmi](https://github.com/fliperama86/pico_hdmi) by [@fliperama86](https://github.com/fliperama86) — the HSTX driver that carries picture and sound over HDMI on the RP2350 boards, and a great deal of help along the way.
- [pico-infones](https://github.com/shuichitakano/pico-infones) by **Shuichi Takano** — the PicoDVI video path and the USB HID gamepad handling this project inherited.
- [PicoDVI](https://github.com/Wren6991/PicoDVI) by **Luke Wren** ([@Wren6991](https://github.com/Wren6991)) — the DVI-over-HDMI implementation underneath it.

**Libraries and drivers**

- [pico_fatfs](https://github.com/elehobica/pico_fatfs) by [@elehobica](https://github.com/elehobica), wrapping [FatFs](http://elm-chan.org/fsw/ff/00index_e.html) by **ChaN** — SD card access.
- [tusb_xinput](https://github.com/Ryzee119/tusb_xinput) by **Ryan Wendland** ([@Ryzee119](https://github.com/Ryzee119)) — Xbox controller support.
- [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB) by [@sekigon-gonnoc](https://github.com/sekigon-gonnoc) — the second USB port on the boards that have one.
- [lwmem](https://github.com/MaJerle/lwmem) by **Tilen Majerle** ([@MaJerle](https://github.com/MaJerle)) — allocator used for the PSRAM heap.

**Hardware**

- The **PicoNES PCB** was designed by **John Edgar Park** ([@johnedgarpark](https://twitter.com/johnedgarpark)).
- The **PicoNES Mini** and **PicoNES Micro** PCBs, and the 3D-printed cases for all of them, were designed by **Gavin Knight** ([DynaMight1124](https://github.com/DynaMight1124)).
- The [metadata pack](#using-metadata) — the box art, game info and themed borders/bezels on the SD card — was put together by **Gavin Knight** ([DynaMight1124](https://github.com/DynaMight1124)).
- **Murmulator M1 and M2** support was contributed by [@javavi](https://github.com/javavi).

**This project**

- **PicoPeanutGB** — the port to the Pico, the menu, the settings screen and the board support — is by **Frank Hoedemakers** ([@fhoedemakers](https://github.com/fhoedemakers)).
- The menu, settings, controller handling and board configurations are shared with [pico-infonesPlus](https://github.com/fhoedemakers/pico-infonesPlus) and the other emulators in the family through [pico_shared](https://github.com/fhoedemakers/pico_shared).
- Part of the code and documentation was written with the assistance of **[Claude Code](https://claude.com/claude-code)**, Anthropic's agentic coding tool.
