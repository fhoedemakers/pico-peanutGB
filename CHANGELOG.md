# CHANGELOG

# General Info

Only binaries for Pico 2 are available. The emulator is too slow on the original Pico or other RP2040 based boards.

- pico2_PicoPeanutGBPimoroniDV.uf2 : For the Pimoroni DV Deno Base with Pico2.
- pico2_PicoPeanutGBAdaFruitDVISD.uf2 : For the breadboard or PCN variant with Pico 2
- pico_nesPCB_v20.zip: PCB Design. For more info see the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus#pcb-with-raspberry-pi-pico-or-pico-2).

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

For more details, see the Pico-InfoNesPlus [README](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#gamecontroller-support) and [troubleshooting](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#troubleshooting-usb-controllers) section

## v0.1

### Features
- Initial release, based on infonesPlus.

### Fixes

