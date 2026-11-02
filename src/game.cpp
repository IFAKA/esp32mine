#include "game.h"

#include <algorithm>
#include <cmath>

namespace mine {
namespace {
constexpr float HalfPi = 1.57079632679f;
constexpr float EyeHeight = 1.62f;
constexpr float PlayerHeight = 1.8f;
constexpr float PlayerRadius = 0.28f;
constexpr float Reach = 6.0f;

uint32_t hash2(int x, int z) {
    uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(z) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
float noise(int x, int z) { return static_cast<float>(hash2(x, z) & 255u) / 255.0f; }

uint16_t rgb(int r, int g, int b) {
    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}
uint16_t shade(uint16_t c, int amount) {
    int r = ((c >> 11) & 31) * 255 / 31;
    int g = ((c >> 5) & 63) * 255 / 63;
    int b = (c & 31) * 255 / 31;
    r = std::clamp(r * amount / 100, 0, 255);
    g = std::clamp(g * amount / 100, 0, 255);
    b = std::clamp(b * amount / 100, 0, 255);
    return rgb(r, g, b);
}

void drawGlyph(uint16_t* fb, int x, int y, char glyph, uint16_t color) {
    static constexpr uint8_t digits[10][5] = {
        {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
        {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}
    };
    static constexpr uint8_t letters[3][5] = {{6,5,6,5,5},{6,5,6,5,6},{3,4,4,4,3}};
    const uint8_t* rows = nullptr;
    if (glyph >= '0' && glyph <= '9') rows = digits[glyph - '0'];
    else if (glyph == 'F') rows = letters[0];
    else if (glyph == 'P') rows = letters[1];
    else if (glyph == 'S') rows = letters[2];
    if (!rows) return;
    for (int row = 0; row < 5; ++row)
        for (int col = 0; col < 3; ++col)
            if (rows[row] & (1u << (2 - col))) fb[(y + row) * ScreenWidth + x + col] = color;
}
}

void drawFps(uint16_t* framebuffer, int fps) {
    if (!framebuffer) return;
    fps = std::clamp(fps, 0, 999);
    for (int y = 2; y < 10; ++y)
        for (int x = 2; x < 31; ++x) framebuffer[y * ScreenWidth + x] = 0x18E3;
    constexpr uint16_t White = 0xFFFF;
    drawGlyph(framebuffer, 4, 4, 'F', White); drawGlyph(framebuffer, 8, 4, 'P', White); drawGlyph(framebuffer, 12, 4, 'S', White);
    drawGlyph(framebuffer, 18, 4, static_cast<char>('0' + fps / 100), White);
    drawGlyph(framebuffer, 22, 4, static_cast<char>('0' + (fps / 10) % 10), White);
    drawGlyph(framebuffer, 26, 4, static_cast<char>('0' + fps % 10), White);
}

Game::Game() : px_(0.5f), py_(12.0f), pz_(0.5f) {}

int Game::terrainHeight(int x, int z) {
    float broad = std::sin(x * 0.055f) * 2.2f + std::cos(z * 0.047f) * 1.8f;
    float detail = (noise(x, z) - 0.5f) * 1.4f;
    return 5 + static_cast<int>(std::floor(broad + detail));
}

BlockId Game::blockAt(int x, int y, int z) const {
    for (int i = editCount_ - 1; i >= 0; --i)
        if (edits_[i].x == x && edits_[i].y == y && edits_[i].z == z) return edits_[i].id;
    int h = terrainHeight(x, z);
    if (y < 0) return 3;
    if (y > h) return 0;
    if (y == h) return 1;
    if (y >= h - 2) return 2;
    return 3;
}

void Game::setBlock(int x, int y, int z, BlockId id) {
    for (int i = 0; i < editCount_; ++i) {
        if (edits_[i].x == x && edits_[i].y == y && edits_[i].z == z) { edits_[i].id = id; return; }
    }
    if (editCount_ < MaxEdits) edits_[editCount_++] = {static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(z), id};
}

bool Game::collides(float x, float y, float z) const {
    int minX = static_cast<int>(std::floor(x - PlayerRadius));
    int maxX = static_cast<int>(std::floor(x + PlayerRadius));
    int minY = static_cast<int>(std::floor(y));
    int maxY = static_cast<int>(std::floor(y + PlayerHeight - 0.001f));
    int minZ = static_cast<int>(std::floor(z - PlayerRadius));
    int maxZ = static_cast<int>(std::floor(z + PlayerRadius));
    for (int bz = minZ; bz <= maxZ; ++bz)
        for (int by = minY; by <= maxY; ++by)
            for (int bx = minX; bx <= maxX; ++bx)
                if (solid(blockAt(bx, by, bz))) return true;
    return false;
}

void Game::move(float dx, float dy, float dz) {
    float nx = px_ + dx;
    if (!collides(nx, py_, pz_)) px_ = nx;
    float nz = pz_ + dz;
    if (!collides(px_, py_, nz)) pz_ = nz;
    float ny = py_ + dy;
    if (!collides(px_, ny, pz_)) { py_ = ny; grounded_ = false; }
    else {
        if (dy < 0) grounded_ = true;
        velocityY_ = 0.0f;
    }
}

Game::Hit Game::raycast() const {
    float cp = std::cos(pitch_), sp = std::sin(pitch_);
    float cy = std::cos(yaw_), sy = std::sin(yaw_);
    float ox = px_, oy = py_ + EyeHeight, oz = pz_;
    float dx = sy * cp, dy = sp, dz = cy * cp;
    int x = static_cast<int>(std::floor(ox)), y = static_cast<int>(std::floor(oy)), z = static_cast<int>(std::floor(oz));
    int sx = dx >= 0 ? 1 : -1, syi = dy >= 0 ? 1 : -1, sz = dz >= 0 ? 1 : -1;
    float tx = dx == 0 ? 1e30f : ((dx >= 0 ? x + 1.0f - ox : ox - x) / std::abs(dx));
    float ty = dy == 0 ? 1e30f : ((dy >= 0 ? y + 1.0f - oy : oy - y) / std::abs(dy));
    float tz = dz == 0 ? 1e30f : ((dz >= 0 ? z + 1.0f - oz : oz - z) / std::abs(dz));
    float dtx = dx == 0 ? 1e30f : 1.0f / std::abs(dx), dty = dy == 0 ? 1e30f : 1.0f / std::abs(dy), dtz = dz == 0 ? 1e30f : 1.0f / std::abs(dz);
    int nx = 0, ny = 0, nz = 0;
    for (int i = 0; i < 96; ++i) {
        if (blockAt(x, y, z) != 0) return {true, x, y, z, nx, ny, nz, std::min(tx, std::min(ty, tz))};
        if (tx < ty && tx < tz) { x += sx; tx += dtx; nx = -sx; ny = 0; nz = 0; }
        else if (ty < tz) { y += syi; ty += dty; nx = 0; ny = -syi; nz = 0; }
        else { z += sz; tz += dtz; nx = 0; ny = 0; nz = -sz; }
        float distance = std::min(tx, std::min(ty, tz));
        if (distance > Reach) break;
    }
    return {};
}

void Game::update(const Input& in, float dt) {
    dt = std::min(dt, 0.05f);
    yaw_ += in.mouseDx * 0.0028f;
    pitch_ = std::clamp(pitch_ - in.mouseDy * 0.0028f, -HalfPi + 0.05f, HalfPi - 0.05f);
    if (in.selectPressed[0]) selected_ = 1;
    if (in.selectPressed[1]) selected_ = 2;
    if (in.selectPressed[2]) selected_ = 3;
    if (in.selectPressed[3]) selected_ = 4;
    float speed = 4.2f;
    float forward = (in.forward ? 1.0f : 0.0f) - (in.backward ? 1.0f : 0.0f);
    float strafe = (in.right ? 1.0f : 0.0f) - (in.left ? 1.0f : 0.0f);
    float len = std::sqrt(forward * forward + strafe * strafe);
    if (len > 0) { forward /= len; strafe /= len; }
    float cy = std::cos(yaw_), sy = std::sin(yaw_);
    move((sy * forward + cy * strafe) * speed * dt, 0, (cy * forward - sy * strafe) * speed * dt);
    // Allow a small contact margin so a jump is not lost between physics
    // frames when the player is standing exactly on a voxel face.
    if (in.jumpPressed && (grounded_ || collides(px_, py_ - 0.08f, pz_))) { velocityY_ = 6.0f; grounded_ = false; }
    velocityY_ -= 16.0f * dt;
    move(0, velocityY_ * dt, 0);
    target_ = raycast();
    if (target_.hit && in.breakPressed) setBlock(target_.x, target_.y, target_.z, 0);
    if (target_.hit && in.placePressed) {
        int bx = target_.x + target_.nx, by = target_.y + target_.ny, bz = target_.z + target_.nz;
        if (blockAt(bx, by, bz) == 0 && !collides(px_, py_, pz_)) setBlock(bx, by, bz, selected_);
    }
}

void Game::render(uint16_t* fb) const {
    float cp = std::cos(pitch_), sp = std::sin(pitch_), cy = std::cos(yaw_), sy = std::sin(yaw_);
    float fx = sy * cp, fy = sp, fz = cy * cp;
    float rx = cy, rz = -sy;
    for (int py = 0; py < ScreenHeight; ++py) for (int px = 0; px < ScreenWidth; ++px) {
        float sx = (px + 0.5f - ScreenWidth * 0.5f) / (ScreenHeight * 0.86f);
        float syy = (ScreenHeight * 0.5f - py - 0.5f) / (ScreenHeight * 0.86f);
        float dx = fx + rx * sx; float dy = fy + syy; float dz = fz + rz * sx;
        float inv = 1.0f / std::sqrt(dx*dx + dy*dy + dz*dz); dx*=inv; dy*=inv; dz*=inv;
        float ox=px_, oy=py_+EyeHeight, oz=pz_; int x=static_cast<int>(std::floor(ox)), y=static_cast<int>(std::floor(oy)), z=static_cast<int>(std::floor(oz));
        int ax=dx>=0?1:-1, ay=dy>=0?1:-1, az=dz>=0?1:-1;
        float tx=dx==0?1e30f:((dx>=0?x+1-ox:ox-x)/std::abs(dx)), ty=dy==0?1e30f:((dy>=0?y+1-oy:oy-y)/std::abs(dy)), tz=dz==0?1e30f:((dz>=0?z+1-oz:oz-z)/std::abs(dz));
        float dtx=dx==0?1e30f:1/std::abs(dx), dty=dy==0?1e30f:1/std::abs(dy), dtz=dz==0?1e30f:1/std::abs(dz); uint16_t color=0; int face=0;
        for (int step=0; step<96; ++step) { BlockId b=blockAt(x,y,z); if(b) { color=b==1?rgb(75,175,68):b==2?rgb(128,82,42):b==3?rgb(112,116,122):rgb(190,135,50); break; } if(tx<ty&&tx<tz){x+=ax;tx+=dtx;face=1;}else if(ty<tz){y+=ay;ty+=dty;face=2;}else{z+=az;tz+=dtz;face=3;} if(std::min(tx,std::min(ty,tz))>Reach) break; }
        if (!color) { int sky = std::clamp(130 + static_cast<int>(syy * 90), 65, 210); color=rgb(42,sky,220); }
        else color=shade(color, face==2?108:face==1?88:78);
        fb[py*ScreenWidth+px]=color;
    }
    int cx=ScreenWidth/2, cy0=ScreenHeight/2;
    for(int i=-4;i<=4;++i){ if(cx+i>=0&&cx+i<ScreenWidth) fb[cy0*ScreenWidth+cx+i]=rgb(245,245,210); if(cy0+i>=0&&cy0+i<ScreenHeight) fb[(cy0+i)*ScreenWidth+cx]=rgb(245,245,210); }
    // Minimal ESP32-friendly block selector: four colored swatches at the
    // bottom center, with the selected type outlined in white.
    const uint16_t swatches[4] = {rgb(75,175,68), rgb(128,82,42), rgb(112,116,122), rgb(190,135,50)};
    int barX = ScreenWidth / 2 - 22;
    for (int slot = 0; slot < 4; ++slot) {
        int x0 = barX + slot * 11;
        for (int yy = 0; yy < 7; ++yy) for (int xx = 0; xx < 9; ++xx) {
            bool edge = yy == 0 || yy == 6 || xx == 0 || xx == 8;
            fb[(ScreenHeight - 10 + yy) * ScreenWidth + x0 + xx] = (selected_ == slot + 1 && edge) ? rgb(255,255,220) : swatches[slot];
        }
    }
}
}
