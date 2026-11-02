# Mission

Build a Minecraft-like game FOR ESP32-S3 N16R8.

The ESP32 is the product target.
macOS is only the development host because physical hardware is
currently unavailable.

The SAME game/core must run on macOS and later ESP32.

# Hardware

ESP32-S3
240 MHz dual-core
8 MB Octal PSRAM
16 MB flash
no GPU
240x135 RGB565

# Primary objective

GET A PLAYABLE GAME WORKING AS FAST AS POSSIBLE.

Playable means:

launch
-> voxel terrain
-> first-person look
-> WASD movement
-> gravity
-> collision
-> jumping
-> target blocks
-> break blocks
-> place blocks
-> select block type
-> continuously explore procedural terrain

Do not work on secondary systems while this loop is incomplete.

# Architecture

Portable C/C++ game core:

- world
- procedural generation
- renderer
- player
- physics/collision
- block interaction
- modifications/cache

Platform adapters:

- macOS: SDL3
- ESP32: ESP-IDF later

SDL/macOS APIs MUST NOT enter gameplay/core code.

Mac must not get a separate higher-resource implementation.

# Rendering

Software voxel renderer.
240x135 logical framebuffer.
No OpenGL/Metal/GPU dependency.

Use an ESP32-realistic algorithm such as voxel DDA/raycasting.

# World

Untouched terrain is deterministic and regenerable.

Expose approximately:

BlockId blockAt(x,y,z)
void setBlock(x,y,z,id)

Do not keep an infinite voxel world in memory.

Use bounded chunks/cache/modification storage.

# Performance rules

Design hot paths for ESP32 from the beginning.

Avoid:

- allocations per pixel/ray/voxel
- unbounded containers
- huge working sets
- desktop-only assumptions

Prefer:

- fixed-capacity storage
- packed blocks
- small working sets
- early ray termination
- preallocated buffers

# Scope

DO NOT initially implement:

mobs
crafting
multiplayer
fluids
redstone
complex lighting
sound
menus
SD persistence
benchmark infrastructure
Wokwi

# Development order

1. window/framebuffer
2. visible voxel terrain
3. mouse look
4. movement
5. collision
6. gravity/jump
7. targeting
8. break
9. place
10. block selection
11. continuous exploration

Build/run after each step.

Never spend time on later infrastructure while an earlier gameplay
step is broken.

# Definition of done

On macOS I can execute one documented command and PLAY the
ESP32-targeted game through the SDL host.

Do not stop at scaffolding, plans, documentation or partial gameplay.
