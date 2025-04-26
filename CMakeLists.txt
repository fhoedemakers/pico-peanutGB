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

include(pico_sdk_import.cmake)

project(${projectname} C CXX ASM)
pico_sdk_init()

if (NOT HW_CONFIG)
    set(HW_CONFIG 2 CACHE STRING "Select the hardware configuration for your board")       
endif()
include("pico_shared/BoardConfigs.cmake")

add_executable(${projectname}
    main.cpp
    splash.cpp
    gb.c
    minigb_apu.c
)

pico_set_program_name(${projectname} "${projectname}")
pico_set_program_version(${projectname} "0.1")

pico_enable_stdio_uart(${projectname} 1)
pico_enable_stdio_usb(${projectname} 0)

# tinyusb
set(FAMILY rp2040)
set(BOARD pico_sdk)
set(TINYUSB_DEBUG_LEVE 0)
include(${PICO_TINYUSB_PATH}/hw/bsp/${FAMILY}/family.cmake)
if (PICO_SDK_VERSION_MAJOR LESS 2)
    family_configure_target(${projectname})                                                                             
else()
    family_configure_target(${projectname} "" )
endif()
# add_compile_definitions(CFG_TUSB_DEBUG=0)
# set(GB_FLAGS -DGB_FILE_ADDR=0x10080000 -DNDEBUG)
target_compile_definitions(${projectname} PUBLIC 
    # ${GB_FLAGS}
    DVICONFIG=${DVICONFIG}
    USE_OLD_SDDRIVER=${USE_OLD_SDDRIVER}
    SDCARD_PIN_CS=${SD_CS}
    SDCARD_PIN_SCK=${SD_SCK}
    SDCARD_PIN_MOSI=${SD_MOSI}
    SDCARD_PIN_MISO=${SD_MISO}
    SDCARD_PIO=pio1        # when USE_OLD_SDDRIVER = 1
    SDCARD_SPI=${SD_SPI}   # when USE_OLD_SDDRIVER = 0
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
    NES_MAPPER_5_ENABLED=${INFONES_MAPPER_5_ENABLED}
    HW_CONFIG=${HW_CONFIG}
    WIIPAD_I2C=${WIIPAD_I2C}
    ENABLE_SOUND=1)
    
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
pico_add_extra_outputs(${projectname})



