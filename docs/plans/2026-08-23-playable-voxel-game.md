# Playable Voxel Game Implementation Plan

> **For Codex:** Implement this plan task-by-task in the current workspace.

**Goal:** Build a playable ESP32-S3-targeted voxel game with a macOS SDL3 host.

**Architecture:** A fixed-capacity, allocation-free C++ game core owns world generation, edits, player physics, interaction, and software RGB565 rendering. The SDL3 adapter supplies keyboard/mouse input and presents the 240x135 framebuffer at a scaled window size.

**Tech Stack:** C++17, SDL3, CMake, software voxel DDA.

---

### Task 1: Create the portable game core

**Files:** Create `src/game.h`, `src/game.cpp`.

Implement deterministic terrain, bounded edit storage, player movement/collision/gravity/jump, voxel DDA targeting, block break/place/selection, and a 240x135 RGB565 renderer. Keep SDL out of these files and avoid per-frame allocations.

### Task 2: Create the SDL3 host

**Files:** Create `src/main.cpp`, `CMakeLists.txt`.

Wire SDL3 events to the core, render the logical framebuffer scaled 4x, capture relative mouse input, and provide a minimal HUD/crosshair.

### Task 3: Build, run, and verify

Run `cmake -S . -B build && cmake --build build` and launch `./build/esp32mine`. Exercise movement, mouse look, jump, break/place, and block selection. Fix compile/runtime failures before handoff.
