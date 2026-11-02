#include "game.h"
#include <cassert>
#include <cstdint>

int main() {
    mine::Game game;
    assert(game.blockAt(0, 0, 0) != 0);
    assert(game.blockAt(0, 100, 0) == 0);
    game.setBlock(3, 20, 4, 2);
    assert(game.blockAt(3, 20, 4) == 2);
    game.setBlock(3, 20, 4, 0);
    assert(game.blockAt(3, 20, 4) == 0);
    uint16_t framebuffer[mine::ScreenWidth * mine::ScreenHeight]{};
    game.render(framebuffer);
    bool hasPixels = false;
    for (uint16_t pixel : framebuffer) hasPixels |= pixel != 0;
    assert(hasPixels);
    mine::Input input;
    input.forward = true;
    input.jumpPressed = true;
    for (int frame = 0; frame < 600; ++frame) {
        input.jumpPressed = frame == 0;
        game.update(input, 1.0f / 60.0f);
        game.render(framebuffer);
    }
    game.setBlock(0, 20, 0, 0);
    game.setBlock(0, 20, 0, 3);
    assert(game.blockAt(0, 20, 0) == 3);
    return 0;
}
