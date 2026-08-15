# CHANGELOG

Brings back support for the original **Raspberry Pi Pico (RP2040)**, with limitations, and reworks **audio** so it no longer drifts against the output clock. Also a round of speed work and a handful of display fixes.

# General Info

[Binaries for each configuration and PCB design are at the end of this page](#downloads___).

[See setup section in in Pico-infoNesPlus readme how to install and wire up](https://github.com/fhoedemakers/pico-infonesPlus#pico-setup)

# v0.12 Release notes

## Raspberry Pi Pico (RP2040) support

Releases for the original Pico are published again, after being dropped for
being too slow. They come with two limitations, both described in the README:

- **Game Boy Color games are not supported.** RP2040 builds are DMG only.
  Colour support needs about 35 KB more RAM than the 256 KB part can spare, and
  costs roughly two thirds more work per scanline. Cartridges marked Game Boy
  Color only are refused when started, with a message in the menu. Cartridges
  that are Game Boy Color *enhanced* but still run on an original Game Boy work
  normally, in their Game Boy form.
- **Busy scenes can still show brief red bands.** There is no framebuffer on
  RP2040, so the emulator has to hand each scanline to the display in time.
  Quiet scenes have room to spare; scenes with many sprites or a scrolling
  window layer can miss the deadline. The picture recovers by itself.

Everything else — sound, save games, borders, palettes, the menu and the
settings screen — behaves as it does on the Pico 2. A Pico 2 is still the
better choice where you have one.

## Audio

- **Fixed audio drifting against the output clock.** The emulator produced a
  fixed 738 samples per frame while every output consumes exactly 44100 per
  second. Over HDMI that was a slow overproduction, so the audio buffer filled
  until packets were dropped; over I2S it was a slow underproduction, so the
  buffer ran dry and the output stalled and restarted. Either way it built up to
  a regular tick or dropout after a minute or two of play. The emulator now asks
  for exactly as many samples as the output is ready to take.
- **Improved note timing.** A whole frame of audio used to be generated in one
  go once the frame had finished, so everything the game did to the sound chip
  during those 16.7 ms landed at the same instant. Audio is now generated in
  eight slices spread through the frame, which sharpens short notes, drums and
  envelopes. (RP2040 keeps the single-block behaviour; slicing there would cost
  video timing it cannot spare.)
- Removed a resampling step that is no longer needed, along with the small loss
  of quality that came with it.

## Speed

Mostly relevant to Game Boy Color titles, which were closest to the limit:

- The processor emulation loop, the sound chip emulation and the sound registers
  now run from RAM instead of flash. The main loop previously reached the
  instruction step through a slow detour on every single instruction.
- Screen colour lookup reduced to a single table read per pixel.
- Replaced the per-scanline sprite sort with a cheaper one.

## Fixes

- **Fixed the display driver locking up.** Opening the settings menu in-game and
  choosing Reset Game, Save/Restore State or the frame rate toggle could freeze
  the picture on a red screen with the emulator stopped. Some display buffers
  were being left behind when the menu handed the screen back. This also
  explains occasional random freezes reported earlier.
- **Fixed the frame rate counter appearing several times down the screen** on
  boards without a framebuffer.
- **Fixed the scanline setting being ignored when a game starts.** Games came up
  without the scanline effect you had selected, and only picked it up once you
  had opened the settings menu and left it again. Starting a game now applies
  your chosen screen mode, scanline style and aspect ratio straight away. Most
  visible on boards that boot directly into a game, which is every board without
  PSRAM.
- **Removed the Save/Restore State entry from the in-game settings menu.** This
  emulator has no save state support, so the entry did nothing.
- Resetting the settings to their defaults no longer leaves an unused internal
  scanline switch at whatever value the settings file happened to hold.

# v0.11 Release notes

This release adds support for pico-bootLoader, extends controller
support, introduces a controller test screen and an additional display
option, and contains a number of stability fixes.

## Support for pico-bootLoader

An RP2350 board normally holds a single program. Running a different
emulator requires connecting the board to a computer, holding BOOTSEL
and copying a different `.uf2` file onto it.

[pico-bootLoader](https://github.com/fhoedemakers/pico-bootLoader)
removes that requirement. The loader is flashed onto the board once, the
applications are placed on the SD card, and from then on a menu is shown
at every power-on. The menu is operated with a USB game controller or a
USB keyboard and can be displayed either with cover artwork per
application or as plain text. Selecting an entry starts the
corresponding application; a reset or power cycle returns to the menu.

Besides this Game Boy / Game Boy Color emulator, the loader supports the
NES, Sega Genesis / Mega Drive, Sega Master System / Game Gear,
PC Engine and Philips Videopac / Odyssey² emulators, as well as a native
port of *Doom*.

Please note:

- The binaries listed on this page are the standalone builds and are
  unchanged. Use one of these to run the Game Boy emulator on its own.
- The builds intended for the loader are published in the pico-bootLoader
  repository, not here. See the [pico-bootLoader releases
  page](https://github.com/fhoedemakers/pico-bootLoader/releases) for
  those files and for the installation instructions.
- When the emulator is started from the loader, the settings menu
  contains an additional entry, **Return to emulator selection**, which
  returns to the application menu. This entry is not present in
  standalone builds.

## What's new

### Controller Test screen

The settings menu contains a new entry, **Controller Test**. It displays
a controller layout on screen and highlights each button while it is
pressed, following the most recently used input source: the controller
ports on the board, USB controllers 1 and 2, and a Wii Classic
controller. A list below the layout shows the connection status of each
source and whether input has been received from it.

Hold **SELECT + START** for two seconds to leave the screen.

### Display options

- New **Scanline Type** option with the values *Simple* and *LCD*. Simple
  darkens alternate lines; LCD darkens alternate columns as well,
  resulting in a grid pattern that resembles an LCD screen.
- The separate **Scanlines** on/off option has been removed. Scanlines
  are now selected through the **Screen Mode** option.

### Settings menu

- The option list is now scrollable, so that all options remain
  accessible as their number increases. The SAVE, CANCEL and DEFAULT
  actions are shown at a fixed position.
- The software version is displayed in the menu.

### Menu and SD card

- The default directory for roms is now **`/roms/GB`**. Roms may still be
  placed anywhere on the card and organised in subdirectories.
- When leaving a subdirectory, the selection returns to the directory
  that was left instead of the first entry in the list.
- If the last used directory no longer exists, the menu falls back to the
  root of the card.

### Controller support

- Shoulder buttons are now mapped on the **DualShock 4**, **DualSense**,
  **Xbox / XInput** controllers, the **MantaPad** and the **Retro-bit
  Mega Drive Arcade Pad**. The Square button is mapped on the
  **PlayStation Classic** controller, and L/R and ZL/ZR are mapped on the
  **Wii Classic Controller**. Additional keys were added to the USB
  keyboard mapping.
- On XInput controllers, the left thumbstick also functions as a d-pad.
- **SNES controllers** connected to the controller port on the board are
  now fully supported. NES controllers are detected automatically and are
  unaffected.

## Fixes

### Display

- Corrected the alignment of the frame buffer, which could cause crashes
  and corrupted scanlines.
- The HDMI output pins are configured with a higher drive strength and a
  faster slew rate, which improves signal quality on longer or
  lower-quality cables.

### Stability

- Fixed a crash when loading larger roms, caused by the rom data
  overwriting the area of flash memory in which the board parameters are
  stored.
- Fixed a possible stack overflow when saving or loading settings, which
  could cause a crash on boards with limited memory.
- The SD card driver has been updated with a number of reliability
  improvements, including card detection after a reset.
- **Adafruit Fruit Jam and Adafruit Feather RP2350 only:** fixed
  initialisation of the audio DAC failing when a NES- or
  SNES-Classic-Mini controller is connected at power-on. These
  controllers interfere with the I2C bus that is shared with the DAC. A
  controller connected at power-on is now also usable from the first menu
  screen.

## Please note

- Settings are reset to their default values the first time this version
  is run. The settings file has changed to accommodate the new options,
  so earlier settings files cannot be reused. Preferences have to be set
  again in the settings menu and saved.


# previous changes

See [HISTORY.md](https://github.com/fhoedemakers/pico-peanutGB/blob/main/HISTORY.md)


<a name="downloads___"></a>
## Downloads by configuration

Binaries for each configuration are listed below. Binaries for Pico(2) also work for Pico(2)-w. No blinking led however on the -w boards.
For some configurations risc-v binaries are available. It is recommended however to use the arm binaries. 

### Standalone boards

| Board | Binary | Readme | |
|:--|:--|:--|:--|
| Adafruit Metro RP2350 | [PicoPeanutGB_AdafruitMetroRP2350_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitMetroRP2350_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#adafruit-metro-rp2350) | |
| Adafruit Fruit Jam | [PicoPeanutGB_AdafruitFruitJam_arm_piousb.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitFruitJam_arm_piousb.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#adafruit-fruit-jam)| |
| Waveshare RP2350-PiZero | [PicoPeanutGB_WaveShareRP2350PiZero_arm_piousb.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_WaveShareRP2350PiZero_arm_piousb.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#waveshare-rp2040rp2350-pizero-development-board)| [3-D Printed case](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#3d-printed-case-for-rp2040rp2350-pizero) |
| Adafruit Feather RP2040 DVI **(RP2040, DMG only)** | [PicoPeanutGB_AdafruitFeatherDVI_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitFeatherDVI_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#adafruit-feather-rp2040-dvi) | |
| Waveshare RP2040-PiZero **(RP2040, DMG only)** | [PicoPeanutGB_WaveShareRP2040PiZero_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_WaveShareRP2040PiZero_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#waveshare-rp2040rp2350-pizero-development-board)| [3-D Printed case](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#3d-printed-case-for-rp2040rp2350-pizero) |

### Breadboard

| Board | Binary | Readme |
|:--|:--|:--|
| Pico 2 | [PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |
| Pico 2 W | [PicoPeanutGB_AdafruitDVISD_pico2_w_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico2_w_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |
| Pimoroni Pico Plus 2 | [PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |
| Pico **(RP2040, DMG only)** | [PicoPeanutGB_AdafruitDVISD_pico_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |


### PCB Pico2

| Board | Binary | Readme |
|:--|:--|:--|
| Pico 2 | [PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-raspberry-pi-pico-or-pico-2) |
| Pico 2 W | [PicoPeanutGB_AdafruitDVISD_pico2_w_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico2_w_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-raspberry-pi-pico-or-pico-2) |
| Pico **(RP2040, DMG only)** | [PicoPeanutGB_AdafruitDVISD_pico_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-raspberry-pi-pico-or-pico-2) |

PCB [pico_nesPCB_v2.6.zip](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/pico_nesPCB_v2.6.zip)

3D-printed case designs for PCB:

[https://www.thingiverse.com/thing:6689537](https://www.thingiverse.com/thing:6689537). 
For the latest two player PCB 2.0, you need:

- Top_v2.0_with_Bootsel_Button.stl. This allows for software upgrades without removing the cover. (*)
- Base_v2.0.stl
- Power_Switch.stl.
(*) in case you don't want to access the bootsel button on the Pico, you can choose Top_v2.0.stl

### PCB WS RP2350-Zero (PCB required)

| Board | Binary | Readme |
|:--|:--|:--|
| Waveshare RP2350-Zero | [PicoPeanutGB_WaveShareRP2350ZeroWithPCB_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_WaveShareRP2350ZeroWithPCB_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-waveshare-rp2040rp2350-zero) |

PCB: [Gerber_PicoNES_Mini_PCB_v2.0.zip](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/Gerber_PicoNES_Mini_PCB_v2.0.zip)

3D-printed case designs for PCB WS2XX0-Zero:
[https://www.thingiverse.com/thing:7041536](https://www.thingiverse.com/thing:7041536)

### PCB Waveshare RP2350-USBA with PCB
[Binary](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_WaveShare2350USBA_arm_piousb.uf2)

PCB: [Gerber_PicoNES_Micro_v1.2.zip](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/Gerber_PicoNES_Micro_v1.2.zip)

[Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-waveshare-rp2350-usb-a)

[Build guide](https://www.instructables.com/PicoNES-RaspberryPi-Pico-Based-NES-Emulator/)


### Pimoroni Pico DV

| Board | Binary | Readme |
|:--|:--| :--|
| Pico 2/Pico 2 w | [PicoPeanutGB_PimoroniDVI_pico2_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_PimoroniDVI_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-for-pimoroni-pico-dv-demo-base) |
| Pimoroni Pico Plus 2 | [PicoPeanutGB_PimoroniDVI_pico2_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_PimoroniDVI_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-for-pimoroni-pico-dv-demo-base) |
| Pico **(RP2040, DMG only)** | [PicoPeanutGB_PimoroniDVI_pico_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_PimoroniDVI_pico_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-for-pimoroni-pico-dv-demo-base) |

> [!NOTE]
> On Pico W and Pico2 W, the CYW43 driver (used only for blinking the onboard LED) causes a DMA conflict with I2S audio on the Pimoroni Pico DV Demo Base, leading to emulator lock-ups. For now, no Pico W or Pico2 W binaries are provided; please use the Pico or Pico2 binaries instead.

### Murmulator M1

For more info about the Murmulator see this website: https://murmulator.ru/ and [#150](https://github.com/fhoedemakers/pico-infonesPlus/issues/150)

| Board | Binary |
|:--|:--|
| Pico 2/Pico 2 w | [PicoPeanutGB_MurmulatorM1_pico2_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_MurmulatorM1_pico2_arm.uf2) |

### Murmulator M2

For more info about the Murmulator see this website: https://murmulator.ru/ and [#150](https://github.com/fhoedemakers/pico-infonesPlus/issues/150)

| Board | Binary |
|:--|:--|
| Pico/Pico w | [PicoPeanutGB_MurmulatorM2_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_MurmulatorM2_arm.uf2) |

### Other downloads

- Metadata: [GBMetadata.zip](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/GBMetadata.zip)


Extract the zip file to the root folder of the SD card. Select a game in the menu and press START to show more information and box art. Works for most official released games. Screensaver shows floating random cover art.




