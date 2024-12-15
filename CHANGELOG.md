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


# Release notes

## v0.3

### Technical changes

- Lots of code is now moved to git module pico_shared. This is code that can be shared between other RP2040/RP2350 emulators. This includes the menu system, the SD-card handling, the display handling. Also the code for controller input (NES, Wii-Classic, USB, keyboard) is moved to this module. When building from source, make sure you do a **git submodule update --init** from within the source folder to get the pico_shared module and all the other modules.

### Features

Because of the shared code, the following features are now available in Pico-SMSPlus:

- Some settings are now saved to SD card. This includes the selected screen mode, chosen with Select+Up or Select+Down  and the last chosen menu selection. Settings are written to /settings.dat on the SD-card. When screen mode is changed, this will be automatically saved. The causes some red flicker due to the delay it causes.
- The colors in the menu can be changed and saved:
  - Select + Up/Down changes the foreground color.
  - Select + Left/Right changes the background color.
  - Select + A saves the colors. Screen will flicker when saved.
  - Select + B resets the colors to default. (Black on white)

## v0.2

### Features

- Add support for these USB gamepads:
  - Sega Mega Drive/Genesis Mini 1 and Mini 2 controllers.
  - PlayStation Classic controller.
  - Mantapad, cheap [NES](https://nl.aliexpress.com/w/wholesale-nes-controller-usb.html?spm=a2g0o.home.search.0) and [SNES](https://nl.aliexpress.com/w/wholesale-snes-controller-usb.html?spm=a2g0o.productlist.search.0) USB controllers from AliExpress. When starting a game, it is possible you have to unplug and replug the controller to get it working.
  - XInput controllers like Xbox 360 and Xbox One controllers. 8bitdo controllers are also XInput controllers and should work. Hold X + Start to switch to XInput mode. (LED 1 and 2 will blink). For Xbox controllers, remove the batteries before connecting the USB cable. Playing with batteries in the controller will work, but can cause the controller to stop working. Sometimes the controller will not work after flashing a game. In that case, unplug the controller and plug it back in. In case of 8bitdo controllers, unplug the controller, hold start to turn it off, then plug it back in. This will make the controller work again.
- Add USB keyboard support:
  - A: Select
  - S: Start
  - Z: B
  - X: A
  - Cursor keys: D-pad
- When an USB device is connected, the device type is shown at the bottom of the menu. Unsupported devices show as xxxx:xxxx.
- Minor cosmetic changes to the menu system.
- Minor changes in PCB design (pico_nesPCB_v2.1.zip)
  - D3 and D4 of NES controller port 2 are connected to GPIO28 (D3) and GPIO27 (D4), for possible future zapper use.
  - More ground points are added.

XInput driver: https://github.com/Ryzee119/tusb_XInput by [Ryzee119](https://github.com/Ryzee119) When building from source, make sure you do a **git submodule update --init** from within the source folder to get the XInput driver.

For more details, see the Pico-InfonesPlus [README](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#gamecontroller-support) and [troubleshooting](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#troubleshooting-usb-controllers) section

## v0.1

### Features
- Initial release, based on infonesPlus.

### Fixes

