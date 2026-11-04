cmake_minimum_required(VERSION 3.20)
set(PICO_BOARD adafruit_fruit_jam CACHE STRING "Public target" FORCE)
set(PICO_PSRAM_CS_PIN 47 CACHE STRING "Fruit Jam PSRAM CS" FORCE)
set(PICO_PSRAM_SIZE_BYTES 0x800000 CACHE STRING "PSRAM size" FORCE)
if(NOT DEFINED PICO_SDK_PATH OR NOT DEFINED PICO_PIO_USB_PATH)
  message(FATAL_ERROR "Set PICO_SDK_PATH and PICO_PIO_USB_PATH to pinned public checkouts")
endif()
set(ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(DEP "${ROOT}/deps")
set(SHARED "${DEP}/pico_shared")
set(HDMI "${SHARED}/drivers/pico_hdmi")
set(FATFS "${SHARED}/drivers/pico_fatfs")
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
project(WataraSupervision C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
set(SAVED_TINYUSB_TOP "${TOP}")
set(TOP "${PICO_SDK_PATH}/lib/tinyusb")
check_and_add_pico_pio_usb_support()
set(TOP "${SAVED_TINYUSB_TOP}")
add_subdirectory("${FATFS}" "${CMAKE_BINARY_DIR}/pico_fatfs")

add_library(supervision_core STATIC runtime/sv_runtime.c runtime/sv_pcfilter_core.c
  runtime/sv_fixed_boot.c deps/potator/controls.c deps/potator/memorymap.c
  deps/potator/gpu.c deps/potator/sound.c deps/potator/timer.c)
target_include_directories(supervision_core PUBLIC runtime PRIVATE "${DEP}/potator" "${DEP}/vrEmu6502/src")
target_compile_options(supervision_core PRIVATE -O3 -funsigned-char -ffunction-sections -fdata-sections
  -fstack-usage -include "${ROOT}/runtime/sv_config.h")
target_link_libraries(supervision_core PRIVATE pico_stdlib)

add_library(supervision_hdmi STATIC "${HDMI}/hstx.c" "${HDMI}/video_output.c"
  "${HDMI}/hstx_data_island_queue.c" "${HDMI}/hstx_packet.c")
target_include_directories(supervision_hdmi PUBLIC "${HDMI}")
target_compile_definitions(supervision_hdmi PRIVATE GPIOHSTXD0=15 GPIOHSTXD1=17 GPIOHSTXD2=19
  GPIOHSTXCK=13 GPIOHSTXINVERTED=1 JACK_HSTX_126MHZ=1 JACK_HSTX_DOUBLE_BUFFER=1 DI_RING_BUFFER_SIZE=128)
target_compile_options(supervision_hdmi PRIVATE -O3)
target_link_libraries(supervision_hdmi PUBLIC pico_stdlib pico_multicore hardware_dma hardware_sync
  hardware_clocks hardware_vreg hardware_pio hardware_spi hardware_i2c hardware_pwm hardware_adc)

add_executable(picoSupervision firmware/supervision.c platform/input/jack_input.c
  platform/input/jack_input_devices.c platform/input/jack_tinyusb_input.c platform/input/jack_nespad_input.c)
target_include_directories(picoSupervision PRIVATE runtime platform/input platform/text "${SHARED}" "${FATFS}/fatfs")
target_compile_definitions(picoSupervision PRIVATE HW_CONFIG=8 PICO_STACK_SIZE=4096 N3_SAVE=0
  R8_TITLE="WATARA SUPERVISION" R8_DIRECTORY="/roms/SUPERVISION" R8_EXTENSION=".sv" R8_BIOS=""
  R8_PROFILE=1 R8_ASYNC_PAD=1 R8_DEFERRED_PRESENT=1 R8_PROFILE_TX_CAPACITY=4352
  R8_VARIANT="SUPERVISION_PUBLIC_ON" R8_BUILD_VERSION="0.1.17-public"
  JACK_TINYUSB_KEYBOARD_RECOVERY=1 PICO_PIO_USE_GPIO_BASE=1 CFG_TUH_RPI_PIO_USB=1
  PIO_USB_USE_PIO=2 PICO_DEFAULT_PIO_USB_DP_PIN=1
  SDCARD_PIN_CS=39 SDCARD_PIN_SCK=34 SDCARD_PIN_MOSI=35 SDCARD_PIN_MISO=36 SDCARD_SPI=spi0 SDCARD_PIO=pio0)
target_compile_options(picoSupervision PRIVATE -O3 -Wall -Wextra -Werror
  -Wno-unused-parameter -Wno-cast-function-type -Wno-missing-field-initializers
  -Wno-attributes -Wno-sign-compare -fstack-usage -include "${ROOT}/runtime/sv_config.h")
target_link_libraries(picoSupervision PRIVATE supervision_core supervision_hdmi pico_fatfs pico_stdlib pico_multicore
  hardware_dma hardware_pio hardware_spi hardware_clocks hardware_vreg hardware_psram hardware_xip_cache
  tinyusb_host tinyusb_board tinyusb_pico_pio_usb m)
target_link_options(picoSupervision PRIVATE -Wl,--gc-sections -Wl,--print-memory-usage)
pico_set_program_name(picoSupervision "picoSupervision")
pico_set_program_version(picoSupervision "0.1.17-public")
pico_enable_stdio_uart(picoSupervision 1)
pico_enable_stdio_usb(picoSupervision 0)
include("${ROOT}/cmake/LoaderPartition.cmake")
supervision_link_for_loader(picoSupervision)
pico_add_extra_outputs(picoSupervision)
