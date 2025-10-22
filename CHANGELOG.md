# CHANGELOG

# General Info

[Binaries for each configuration and PCB design are at the end of this page](#downloads___).

[See setup section in in Pico-infoNesPlus readme how to install and wire up](https://github.com/fhoedemakers/pico-infonesPlus#pico-setup)

# v0.7 Release Notes

- Game Boy Color games can be played now.
- In-game hotkeys are now consistent with the other emulators. 
- Themed borders/bezels are shown in-game. For this you need the latest [metadata pack](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/GBMetadata.zip) from the releases section. Download and unzip it's contents to the root of the SD card.
- Support for [Retro-bit 8 button Genesis-USB](https://www.retro-bit.com/controllers/genesis/#usb). 
- Added support for [Adafruit Fruit Jam](https://www.adafruit.com/product/6200):  
  - Uses HSTX for video output.  
  - Audio is not supported over HSTX — connect speakers via the **audio jack** or the **4–8 Ω speaker connector**.  
  - Audio is simultaneousy played through speaker and jack. Speaker audio can be muted with **Button 1**.  
  - Controller options:  
    - **USB gamepad** on USB 1.  
    - **Wii Classic controller** via [Adafruit Wii Nunchuck Adapter](https://www.adafruit.com/product/4836) on the STEMMA QT port.  
  - Scanlines can be toggled with **SELECT + UP**.  
  - NeoPixel leds act as a VU meter. Can be toggled on or of via Button2 on the Fruit Jam, or SELECT + RIGHT on the controller.

- Added support for [Waveshare RP2350-PiZero](https://www.waveshare.com/rp2350-pizero.htm):  
  - Gamepad must be connected via the **PIO USB port**.  
  - The built-in USB port is now dedicated to **power and firmware flashing**, removing the need for a USB-Y cable.  
  - Optional: when you solder the optional PSRAM chip on the board, the emulator will make use of it. Roms will be loaded much faster using PSRAM.

- Added support for Waveshare RP2350-USBA with PCB. More info and build guide at: https://www.instructables.com/PicoNES-RaspberryPi-Pico-Based-NES-Emulator/
- Added support for [Spotpear HDMI](https://spotpear.com/index/product/detail/id/1207.html) board.

- Framebuffer implemented in SRAM. This eliminates the red flicker during slow operations, such as SD card I/O.

- **Cover art and metadata support**:  
  - Download pack [here](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/GBMetadata.zip).  
  - Extract the zip contents to the **root of the SD card**.  
  - In the menu:  
    - Highlight a game and press **START** → show cover art and metadata.  
    - Press **SELECT** → show full game description.  
    - Press **B** → return to menu.  
    - Press **START** or **A** → start the game.

- Screensaver
  - Block screensaver, which is shown when no metadata is available, is replaced by static floating image.

Huge thanks to [Gavin Knight](https://github.com/DynaMight1124) for providing the metadata and images as well as testing the different builds!

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

### Breadboard

| Board | Binary | Readme |
|:--|:--|:--|
| Pico 2 | [PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |
| Pico 2 W | [PicoPeanutGB_AdafruitDVISD_pico2_w_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico2_w_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |
| Pimoroni Pico Plus 2 | [PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |


### PCB Pico2

| Board | Binary | Readme |
|:--|:--|:--|
| Pico 2 | [PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-raspberry-pi-pico-or-pico-2) |
| Pico 2 W | [PicoPeanutGB_AdafruitDVISD_pico2_w_arm.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_AdafruitDVISD_pico2_w_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-raspberry-pi-pico-or-pico-2) |

PCB [pico_nesPCB_v2.1.zip](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/pico_nesPCB_v2.1.zip)

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
| Waveshare RP2350-Zero | [PicoPeanutGB_WaveShareRP2350PiZero_arm_piousb.uf2](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/PicoPeanutGB_WaveShareRP2350ZeroWithPCB_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-waveshare-rp2040rp2350-zero) |

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

> [!NOTE]
> On Pico W and Pico2 W, the CYW43 driver (used only for blinking the onboard LED) causes a DMA conflict with I2S audio on the Pimoroni Pico DV Demo Base, leading to emulator lock-ups. For now, no Pico W or Pico2 W binaries are provided; please use the Pico or Pico2 binaries instead.


### Other downloads

- Metadata: [GBMetadata.zip](https://github.com/fhoedemakers/pico-peanutGB/releases/latest/download/GBMetadata.zip)


Extract the zip file to the root folder of the SD card. Select a game in the menu and press START to show more information and box art. Works for most official released games. Screensaver shows floating random cover art.



