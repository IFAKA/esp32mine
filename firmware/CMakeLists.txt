cmake_minimum_required(VERSION 3.16)

# ESP-IDF 6's GCC toolchain leaves CMAKE_OBJDUMP empty on this host; ldgen
# needs the target objdump to produce the final linker script.
set(CMAKE_OBJDUMP "xtensa-esp32s3-elf-objdump" CACHE FILEPATH "ESP32-S3 objdump")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32mine_firmware)
