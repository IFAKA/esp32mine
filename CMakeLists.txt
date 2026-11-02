cmake_minimum_required(VERSION 3.20)
project(esp32mine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SDL3 CONFIG REQUIRED)
if(APPLE)
    enable_language(OBJCXX)
    set(PLATFORM_ACTIVATION src/platform_activation.mm)
else()
    set(PLATFORM_ACTIVATION src/platform_activation.cpp)
endif()

add_executable(esp32mine_app src/main.cpp src/game.cpp ${PLATFORM_ACTIVATION})
target_include_directories(esp32mine_app PRIVATE src)
target_link_libraries(esp32mine_app PRIVATE SDL3::SDL3)
if(APPLE)
    set_source_files_properties(macos/README.txt PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(esp32mine_app PRIVATE macos/README.txt)
    set_target_properties(esp32mine_app PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "ESP32 Mine"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.esp32mine.game"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )
    target_link_libraries(esp32mine_app PRIVATE "-framework Cocoa")
endif()

if(APPLE)
    add_executable(qemu_input tools/qemu_input.cpp)
    target_link_libraries(qemu_input PRIVATE SDL3::SDL3)

    add_executable(qemu_activate tools/qemu_activate.mm)
    target_link_libraries(qemu_activate PRIVATE "-framework ApplicationServices")
endif()

enable_testing()
add_executable(core_smoke tests/core_smoke.cpp src/game.cpp)
target_include_directories(core_smoke PRIVATE src)
add_test(NAME core_smoke COMMAND core_smoke)
