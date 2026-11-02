# ESP32 Mine

An ESP32-S3-targeted software voxel game with a macOS SDL3 host. The game core uses a fixed 240x135 RGB565 framebuffer, deterministic terrain, bounded block edits, software voxel DDA raycasting, and no platform APIs.

## Play

```sh
cmake -S . -B build && cmake --build build && open -na "$PWD/build/esp32mine_app.app"
```

The final `open -na` is required: it launches the `.app` through macOS LaunchServices, making the game the active application so keyboard input goes to the game instead of Terminal/tmux. Do not launch `Contents/MacOS/esp32mine_app` directly.

Controls: the game window activates, comes to the front, and stays above other windows on launch. It captures keyboard and mouse input immediately. Use `WASD` move, mouse look, `Space` jump, left mouse break, right mouse place, `1-4` select block, `Esc` quit.

## Play the actual ESP32-S3 firmware in QEMU

```sh
./tools/play-esp32-qemu.sh
```

This builds the ESP-IDF firmware, starts Espressif's ESP32-S3 QEMU with the
16 MB flash/8 MB octal-PSRAM configuration, opens its graphical RGB565
framebuffer, and starts a small macOS input bridge. Click the green bridge
panel for keyboard and mouse input; the QEMU window is the display. QEMU
shows the unchanged 240x135 core framebuffer at a 3x nearest-neighbour
presentation scale. The bridge is only a platform adapter: the firmware runs
the same portable world, physics, interaction, and software renderer used by
the SDL host.

The deterministic core smoke test remains available with:

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

## Support

If this project is useful to you, you can support my work:

[![Support me on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/zfaka)
