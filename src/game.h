#pragma once

#include <cstdint>

namespace mine {

constexpr int ScreenWidth = 240;
constexpr int ScreenHeight = 135;
using BlockId = uint8_t;

struct Input {
    float mouseDx = 0.0f;
    float mouseDy = 0.0f;
    bool forward = false, backward = false, left = false, right = false;
    bool jumpPressed = false;
    bool breakPressed = false;
    bool placePressed = false;
    bool selectPressed[4]{};
};

class Game {
public:
    Game();
    void update(const Input& input, float dt);
    void render(uint16_t* framebuffer) const;
    BlockId blockAt(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockId id);

private:
    struct Edit { int16_t x, y, z; BlockId id; };
    struct Hit { bool hit; int x, y, z; int nx, ny, nz; float distance; };

    static constexpr int MaxEdits = 256;
    Edit edits_[MaxEdits]{};
    int editCount_ = 0;
    float px_, py_, pz_;
    float velocityY_ = 0.0f;
    float yaw_ = 0.0f, pitch_ = 0.0f;
    bool grounded_ = false;
    BlockId selected_ = 1;
    Hit target_{};

    static int terrainHeight(int x, int z);
    static bool solid(BlockId id) { return id != 0; }
    Hit raycast() const;
    void move(float dx, float dy, float dz);
    bool collides(float x, float y, float z) const;
};

void drawFps(uint16_t* framebuffer, int fps);

}
