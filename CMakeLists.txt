cmake_minimum_required(VERSION 3.13)
set(projectname PicoPeanutGB)
#set(PICO_DEOPTIMIZED_DEBUG on)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

IF(NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE Debug
       CACHE STRING "Choose the type of build : None Debug Release RelWithDebInfo MinSizeRel."
       FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)
message("* Current build type is : ${CMAKE_BUILD_TYPE}")
if (NOT HW_CONFIG)
    set(HW_CONFIG 2 CACHE STRING "Select the hardware configuration for your board")      
endif()
if (NOT USE_HSTX)
    set(USE_HSTX 1 CACHE STRING "Use HSTX for display output when possible")
endif()
include("pico_shared/BoardConfigs.cmake")

if (NOT PICO_BOARD )
    set(PICO_BOARD pico CACHE STRING "Board type")
    message("PICO_BOARD not set, using default: ${PICO_BOARD}")
endif()

add_definitions( -DNDEBUG )

# ---------------------------------------------------------------------------
# TinyUSB family support + Pico SDK init
# Include family_support.cmake BEFORE pico_sdk_init() so that TinyUSB picks up
# the board-specific configuration (PIO USB VBUSEN pin, DP pin, etc.) from the
# BOARD variable set by BoardConfigs.cmake.  The Pico SDK's own TinyUSB
# integration forcibly sets BOARD="pico_sdk" which loses these board-specific
# defines; including family_support.cmake first (with its include_guard) prevents
# the SDK from overriding the board configuration.
# ---------------------------------------------------------------------------
set(FAMILY rp2040)
include($ENV{PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)

# Some boards (e.g. adafruit_metro_rp2350) force CFG_TUH_RPI_PIO_USB=1 in their
# board.cmake, which TinyUSB's rp2040/family.cmake then bakes into
# tinyusb_common_base. When PIO USB is disabled for this build, strip that
# interface define so it doesn't fight our -DCFG_TUH_RPI_PIO_USB=0 and drag
# hcd_pio_usb.c / pio_usb.h into the compile.
if(ENABLE_PIO_USB EQUAL 0 AND TARGET tinyusb_common_base)
    get_target_property(_tu_defs tinyusb_common_base INTERFACE_COMPILE_DEFINITIONS)
    if(_tu_defs)
        list(REMOVE_ITEM _tu_defs "CFG_TUH_RPI_PIO_USB=1")
        set_target_properties(tinyusb_common_base PROPERTIES
            INTERFACE_COMPILE_DEFINITIONS "${_tu_defs}")
    endif()
endif()

project(${projectname} C CXX ASM)

pico_sdk_init()

# check_and_add_pico_pio_usb_support() needs pico_generate_pio_header which is
# only available after pico_sdk_init().  The first call inside family.cmake
# (before SDK init) was a no-op; this second call actually creates the
# tinyusb_pico_pio_usb target AND appends hcd_pio_usb.c / dcd_pio_usb.c to
# tinyusb_host_base / tinyusb_device_base. Skip it when PIO USB is disabled,
# otherwise those files get compiled into the project and fail looking for
# pio_usb.h (its include dir lives on tinyusb_pico_pio_usb, which we don't
# link in that case).
if(ENABLE_PIO_USB EQUAL 1)
    message("Checking for PIO USB support...")
    check_and_add_pico_pio_usb_support()
endif()
message("PIO USB support: ${ENABLE_PIO_USB}, using PIO: ${PIO_USB_USE_PIO}")

# ---------------------------------------------------------------------------
# Game Boy Color support.
#
# Costs ~32KB of SRAM permanently (WRAM 0x2000->0x8000, VRAM 0x2000->0x4000)
# and, more importantly for a slow core, a per-pixel cgbMode test in every
# background, window and sprite inner loop plus a second 160-byte pixelsPrio
# clear per scanline -- all paid on plain DMG cartridges too.
#
# RP2350 has the headroom, so it stays on there. The RP2040's Cortex-M0+ needs
# roughly 65% more instructions for __gb_draw_line than the M33 does, and the
# PicoDVI path pins clk_sys at 252MHz (the DVI bit clock is clk_sys, and 640x480p60
# needs a 25.2MHz pixel clock), so there is no clock headroom to spend either.
# DMG-only is therefore the default on RP2040.
#
# Override either way with -DPEANUT_FULL_GBC_SUPPORT=ON/OFF, e.g. a DMG-only
# RP2350 build:  EXTRA_CMAKE_ARGS=-DPEANUT_FULL_GBC_SUPPORT=OFF ./bld.sh -c 8 -2
# ---------------------------------------------------------------------------
if (NOT DEFINED PEANUT_FULL_GBC_SUPPORT)
    if (PICO_PLATFORM STREQUAL "rp2040")
        set(PEANUT_FULL_GBC_SUPPORT OFF)
    else()
        set(PEANUT_FULL_GBC_SUPPORT ON)
    endif()
endif()
if (PEANUT_FULL_GBC_SUPPORT)
    message(STATUS "Game Boy Color support: ON (.gb and .gbc)")
else()
    message(STATUS "Game Boy Color support: OFF - DMG only. CGB-only cartridges are rejected at load.")
endif()

# ---------------------------------------------------------------------------
# How many slices a video frame's audio is rendered in.
#
# Slicing cuts APU timing quantisation from a whole frame (16.7ms) to ~2ms, but
# it only works where the emulator is decoupled from scanout by a framebuffer:
# HSTX always, and RP2350 PicoDVI which allocates one in SRAM.
#
# RP2040 PicoDVI has no framebuffer, and there the choice is about *where in the
# frame* the APU work lands rather than how much of it there is. Rendered at end
# of frame it falls in the bottom-border/VBlank window, where scanout wants no
# new lines and the ~200-300us costs nothing. Sliced, that same work moves into
# the active window, where dvi_getlinebuffer() blocks and every microsecond
# spends the lead DVI_N_LINE_BUFFERS exists to provide -- about 20% of what is
# left over in Super Mario Land's demo (see the measurements below). Paying video
# margin for audio granularity is the wrong trade on the one platform with none
# to spare, so: one slice, at end of frame.
#
# (This was first set for a hypothesis that turned out to be wrong -- that the
# slice burst was itself the red flicker. It was not; the buffer depth below was.
# The setting survives on the argument above, which does not depend on depth.)
#
# Set to 1 on any board to rule audio slicing out while diagnosing video, or
# raise it (4/8/16) to trade CPU for finer audio timing.
# ---------------------------------------------------------------------------
if (NOT DEFINED AUDIO_CHUNKS_PER_FRAME)
    if (PICO_PLATFORM STREQUAL "rp2040")
        set(AUDIO_CHUNKS_PER_FRAME 1)
    else()
        set(AUDIO_CHUNKS_PER_FRAME 8)
    endif()
endif()
message(STATUS "Audio slices per frame: ${AUDIO_CHUNKS_PER_FRAME}")

# ---------------------------------------------------------------------------
# Depth of the PicoDVI line-buffer pool (RP2040 only; RP2350 renders into a
# framebuffer and never touches this path).
#
# Measured on Super Mario Land at 252MHz, DMG-only: raw emulation is 9.1ms on
# the title screen and 11.5-12.5ms in the demo, all inside the 16.67ms frame.
# Per *line* though: 9116/154 = 59.2us beats the 63.5us the display consumes a
# source line in, while 11500/154 = 74.7us loses 11.2us on every one. With the
# library default of 5 buffers (~318us of lead) that runs out after ~28 lines,
# and because dvi.cpp only accepts a line on an exact `line * 2 == lineCounter_`
# match, one miss forfeits the frame: red lines and a hard drop to 30fps. Hence
# 60fps on the title screen and 30fps in the demo with nothing in between.
#
# Covering 144 lines at 11.2us needs ~26 buffers, ~40 at the 12.5ms peak. 48 is
# 61KB of heap at 1280 bytes each, which the DMG-only RP2040 build (75KB static
# of 256KB) has room for.
# ---------------------------------------------------------------------------
if (NOT DEFINED DVI_N_LINE_BUFFERS)
    if (PICO_PLATFORM STREQUAL "rp2040")
        set(DVI_N_LINE_BUFFERS 48)
    else()
        set(DVI_N_LINE_BUFFERS 5)
    endif()
endif()
message(STATUS "PicoDVI line buffers: ${DVI_N_LINE_BUFFERS}")

# ---------------------------------------------------------------------------
# Diagnostics, both off by default.
#
# EMU_FRAME_STATS  prints average and peak emu_run_frame() time over UART once a
#                  second. Needs a board with UART_ENABLED=1 (HW_CONFIG 2/3/4;
#                  HW_CONFIG 1 and 12/13 have it disabled).
# NORENDER         makes the emulator draw into a throwaway buffer instead of
#                  the display, so nothing blocks on scanout. Combined with
#                  EMU_FRAME_STATS this reports raw emulation cost with the
#                  display pipeline taken out of the picture.
#
#   EXTRA_CMAKE_ARGS="-DEMU_FRAME_STATS=1"              ./bld.sh -c 2   # loop cost
#   EXTRA_CMAKE_ARGS="-DEMU_FRAME_STATS=1 -DNORENDER=1" ./bld.sh -c 2   # emu cost
#
# Cached, so they show up in the IDE's CMake settings and can be flipped there.
# Being cached also means they persist in an existing build directory: an IDE
# build reuses build/ with whatever cache a previous configure left behind, so a
# run configured with -DEMU_FRAME_STATS=1 keeps printing until the value is
# cleared. bld.sh always deletes build/ first, so it is unaffected. To clear it
# in place: cmake -DEMU_FRAME_STATS=0 build, or just delete build/.
# ---------------------------------------------------------------------------
option(EMU_FRAME_STATS "Print average/peak emu_run_frame() time over UART once a second" OFF)
option(NORENDER "Draw into a throwaway buffer instead of the display (timing diagnostics)" OFF)
if (EMU_FRAME_STATS OR NORENDER)
    message(WARNING "Diagnostic build: EMU_FRAME_STATS=${EMU_FRAME_STATS} NORENDER=${NORENDER}."
                    " NORENDER shows no picture. Not for release.")
endif()

add_executable(${projectname}
    main.cpp
    splash.cpp
    gb.c
    minigb_apu.c
    DefaultSS444.c
    DefaultSS555.c
    EmuOverlay444.c  
    EmuOverlay555.c
)

pico_set_program_name(${projectname} "${projectname}")
pico_set_program_version(${projectname} "0.1")

# disable uart output for Pimoronis Pico DV Demo Base, as gpio 1 is used for NES controller
# so debug messages will not be visible on the serial console.
#if ( HW_CONFIG EQUAL 1 )
if (UART_ENABLED EQUAL 0 )
    message("Disabling UART output for this board.")
    pico_enable_stdio_uart(${projectname} 0)
else()
    pico_enable_stdio_uart(${projectname} 1)
endif()

pico_enable_stdio_usb(${projectname} 0)

# ---------------------------------------------------------------------------
# What family_configure_target() used to do for us, inlined:
#   * RP2040-E5 USB device enumeration fix
#   * Silence harmless warnings from TinyUSB / pico-pio-usb sources
# ---------------------------------------------------------------------------
target_compile_definitions(${projectname} PRIVATE
    PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1
)

target_compile_options(${projectname} PRIVATE
    -Wno-unused-parameter
    -Wno-unused-function
    -Wno-cast-function-type
    -Wno-maybe-uninitialized
)

# NOTE: -flto does not work on this project. The SDK links stdio through
# --wrap=printf/puts, and GCC's LTO does not resolve those wrappers across
# partitions, so the link fails with "undefined reference to __wrap_printf"
# from every translation unit that prints. It also dropped the SCRATCH_X core1
# stack reservation. Do not re-enable without solving both.

# add_compile_definitions(CFG_TUSB_DEBUG=0)
# set(GB_FLAGS -DGB_FILE_ADDR=0x10080000 -DNDEBUG)
target_compile_definitions(${projectname} PUBLIC 
    # ${GB_FLAGS}
    DVICONFIG=${DVICONFIG}
    SDCARD_PIN_CS=${SD_CS}
    SDCARD_PIN_SCK=${SD_SCK}
    SDCARD_PIN_MOSI=${SD_MOSI}
    SDCARD_PIN_MISO=${SD_MISO}
    SDCARD_PIO=pio1        # Fallback when SPI initialization fails
    SDCARD_SPI=${SD_SPI}  
    NES_PIN_CLK=${NES_CLK}
    NES_PIN_DATA=${NES_DATA}
    NES_PIN_LAT=${NES_LAT}
    NES_PIO=${NES_PIO}
    NES_PIN_CLK_1=${NES_CLK_1}
    NES_PIN_DATA_1=${NES_DATA_1}
    NES_PIN_LAT_1=${NES_LAT_1}
    NES_PIO_1=${NES_PIO_1}
    WII_PIN_SDA=${WII_SDA}
    WII_PIN_SCL=${WII_SCL}
    LED_GPIO_PIN=${LED_GPIO_PIN}
    HW_CONFIG=${HW_CONFIG}
    WIIPAD_I2C=${WIIPAD_I2C}
    USE_I2S_AUDIO=${USE_I2S_AUDIO}
    PICO_AUDIO_I2S_DATA_PIN=${PICO_AUDIO_I2S_DATA_PIN}
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=${PICO_AUDIO_I2S_CLOCK_PIN_BASE}
    PICO_AUDIO_I2S_PIO=${PICO_AUDIO_I2S_PIO}
    PICO_AUDIO_I2S_DMA_IRQ=1
    PICO_AUDIO_I2S_CLOCK_PINS_SWAPPED=${PICO_AUDIO_I2S_CLOCK_PINS_SWAPPED}
    PICO_AUDIO_I2S_RESET_PIN=${PICO_AUDIO_I2S_RESET_PIN} # Reset pin for TLV320 codec
    PICO_AUDIO_I2S_INTERRUPT_PIN=${PICO_AUDIO_I2S_INTERRUPT_PIN}
    PICO_AUDIO_I2S_INTERRUPT_IS_BUTTON=${PICO_AUDIO_I2S_INTERRUPT_IS_BUTTON}
    CFG_TUH_RPI_PIO_USB=${ENABLE_PIO_USB} # Enable PIO USB support
    PIO_USB_USE_PIO=${PIO_USB_USE_PIO} # Use PIO for USB
    PSRAM_CS_PIN=${PSRAM_CS_PIN} # Select the GPIO pin for PSRAM chip select
    GPIOHSTXD0=${GPIOHSTXD0} # HSTX D0 pin positive
    GPIOHSTXD1=${GPIOHSTXD1} # HSTX D1 pin positive
    GPIOHSTXD2=${GPIOHSTXD2} # HSTX D2 pin positive
    GPIOHSTXCK=${GPIOHSTXCK} # HSTX clock pin positive
    GPIOHSTXINVERTED=${GPIOHSTXINVERTED} # Set to 1 if HSTX pins are inverted: D- = D+ -1
    ENABLE_VU_METER=${ENABLE_VU_METER} # Enable VU meter (fruitjam NeoPixel leds)
    ENABLE_SOUND=1
    PEANUT_FULL_GBC_SUPPORT=$<BOOL:${PEANUT_FULL_GBC_SUPPORT}>
    AUDIO_CHUNKS_PER_FRAME=${AUDIO_CHUNKS_PER_FRAME}
    # $<BOOL:> because option() yields ON/OFF, and these reach C as values, not
    # just #if tests: -DNORENDER=OFF compiles only because the preprocessor
    # treats the unknown identifier as 0, and breaks as soon as one is used in
    # an expression.
    EMU_FRAME_STATS=$<BOOL:${EMU_FRAME_STATS}>
    NORENDER=$<BOOL:${NORENDER}>
    DVI_N_LINE_BUFFERS=${DVI_N_LINE_BUFFERS}
    WIIPAD_DELAYED_START=${WIIPAD_DELAYED_START} # Delay Wii Pad initialization to avoid I2C errors
    ENABLEDVI=1
    )

# Take core1's boot stack from the linker-reserved SCRATCH_X bank
# (__StackOneBottom..__StackOneTop) instead of the private 4KB .bss array in the
# HSTX driver, returning 4KB of SRAM and putting the stack in a bank with no
# DMA/core0 bus contention. PICO_CORE1_STACK_SIZE claims the full bank; the
# default (= PICO_STACK_SIZE = 3072) would leave core1's resync-path printf
# short. HSTX builds only: this mirrors the HSTX macro in FrensHelpers.h, and on
# the PicoDVI path SCRATCH_X already holds the TMDS tables (a full 4KB stack
# overflows the bank) while core1 is launched from FrensHelpers with its own
# stack, so there would be nothing to reclaim.
if (NOT PICO_PLATFORM STREQUAL "rp2040"
    AND GPIOHSTXD0 AND GPIOHSTXD1 AND GPIOHSTXD2 AND GPIOHSTXCK)
    message(STATUS "HSTX build: core1 stack in SCRATCH_X, frees 4KB of SRAM.")
    target_compile_definitions(${projectname} PUBLIC
        HSTX_CORE1_STACK_IN_SCRATCH_X=1
        PICO_CORE1_STACK_SIZE=4096
    )
endif()
if(NOT PICO_PLATFORM STREQUAL "rp2040")
    message(STATUS "Using PICO_PIO_USE_GPIO_BASE=1 for non-rp2040 platforms.")
    target_compile_definitions(${projectname} PRIVATE
        PICO_PIO_USE_GPIO_BASE=1 # Needed for PIO USB support and DVI with gpio pins > 32
        PICO_STACK_SIZE=3072
    )
endif()
if (DEFINED PICO_DEFAULT_UART_TX_PIN)
    message(STATUS "Using PICO_DEFAULT_UART_TX_PIN: ${PICO_DEFAULT_UART_TX_PIN}")
    target_compile_definitions(${projectname} PRIVATE
        PICO_DEFAULT_UART_TX_PIN=${PICO_DEFAULT_UART_TX_PIN}
        PICO_DEFAULT_UART=0
    )
endif()
if (DEFINED PICO_DEFAULT_UART_RX_PIN)
    message(STATUS "Using PICO_DEFAULT_UART_RX_PIN: ${PICO_DEFAULT_UART_RX_PIN}")
    target_compile_definitions(${projectname} PRIVATE
        PICO_DEFAULT_UART_RX_PIN=${PICO_DEFAULT_UART_RX_PIN}
    )
endif()
if (DEFINED PICO_DEFAULT_UART)
    message(STATUS "Using PICO_DEFAULT_UART: ${PICO_DEFAULT_UART}")
    target_compile_definitions(${projectname} PRIVATE
        PICO_DEFAULT_UART=${PICO_DEFAULT_UART}
    )
endif()
# ---------------------------------------------------------------------------
# PIO USB (optional)
# tinyusb_pico_pio_usb is a target the pico-sdk exposes when TinyUSB is present.
# ---------------------------------------------------------------------------
if(ENABLE_PIO_USB EQUAL 1)
    if(NOT PIO_DP_PLUS_PIN EQUAL -1)
        message(STATUS "Using PIO USB DP pin: ${PIO_DP_PLUS_PIN}")
        target_compile_definitions(${projectname} PRIVATE
            PICO_DEFAULT_PIO_USB_DP_PIN=${PIO_DP_PLUS_PIN}
        )
    endif()
    message("Adding target link libraries for PIO USB support.")
    target_link_libraries(${projectname} PRIVATE tinyusb_pico_pio_usb)
endif() 
target_link_libraries(${projectname} PRIVATE
    pico_stdlib
    pico_multicore
    hardware_dma
    hardware_pio
    hardware_i2c
    hardware_interp
    hardware_timer
    hardware_clocks
    hardware_pwm
    hardware_flash
    dvi
    util
    tinyusb_host 
    tinyusb_board
    pico_shared  
    xinput_host 
)
if(PICO_BOARD STREQUAL "pico_w" OR PICO_BOARD STREQUAL "pico2_w")
    # Attention pico2_w in arm-s or risv-s mode causes the led blinking to generate red screen flicker and
    # multiple "[CYW43] do_ioctl(2, 263, 16): timeout" messages to the console.
    # pico_w does not have this issue.
    target_link_libraries(${projectname} PRIVATE pico_cyw43_arch_none)
endif()

target_include_directories(${projectname}
    PRIVATE
    pico_lib
    pico_shared
    tusb_xinput
    .
)

add_subdirectory(pico_lib/dvi)
add_subdirectory(pico_lib/util)
add_subdirectory(pico_shared)
add_subdirectory(tusb_xinput xinput_host)
target_link_options(${projectname} PRIVATE -Xlinker --print-memory-usage --data-sections)

# Optional: relink this emulator to run from the bootloader's application
# partition (for use with pico_emuLoader). Off by default so standalone builds
# are unchanged. Enable with: cmake -DBUILD_FOR_BOOTLOADER=ON ..
if(BUILD_FOR_BOOTLOADER)
    include("pico_shared/BootPartition.cmake")
    if(DEFINED BUILD_FOR_BOOTLOADER_SLOT)
        frens_link_to_pinned_slot(${projectname} ${BUILD_FOR_BOOTLOADER_SLOT})
    else()
        frens_offset_for_bootloader(${projectname})
    endif()
endif()

pico_add_extra_outputs(${projectname})



