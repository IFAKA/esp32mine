#include "game.h"

#include "driver/uart.h"
#include "esp_check.h"
#include "esp_lcd_qemu_rgb.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstring>

namespace {
constexpr char Tag[] = "esp32mine";
// idf.py's console serial remains UART0; the launcher's additional TCP serial
// endpoint is the development side-channel exposed as UART1 by QEMU.
constexpr uart_port_t InputUart = UART_NUM_1;
constexpr uint8_t Sync = 0xA5;
// The core remains 240x135; QEMU presents a 3x nearest-neighbour view.
constexpr int QemuScale = 3;
constexpr int QemuWidth = mine::ScreenWidth * QemuScale;
constexpr int QemuHeight = mine::ScreenHeight * QemuScale;
static uint16_t CoreFramebuffer[mine::ScreenWidth * mine::ScreenHeight];

struct __attribute__((packed)) Packet {
    uint8_t sync;
    int16_t mouseDx;
    int16_t mouseDy;
    uint16_t buttons;
    uint8_t select;
    uint8_t reserved;
};

struct InputState {
    mine::Input game{};
    bool w = false, a = false, s = false, d = false;
    bool space = false;
    bool left = false, right = false;
};

void setupInput() {
    uart_config_t config{};
    config.baud_rate = 115200;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;
    ESP_ERROR_CHECK(uart_param_config(InputUart, &config));
    ESP_ERROR_CHECK(uart_driver_install(InputUart, 1024, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_flush_input(InputUart));
}

void pollInput(InputState& state) {
    static uint8_t buffer[sizeof(Packet)];
    static size_t count = 0;
    static bool loggedPacket = false;
    uint8_t bytes[64];
    int length = uart_read_bytes(InputUart, bytes, sizeof(bytes), 0);
    for (int i = 0; i < length; ++i) {
        if (count == 0 && bytes[i] != Sync) continue;
        buffer[count++] = bytes[i];
        if (count != sizeof(Packet)) continue;
        Packet packet{};
        std::memcpy(&packet, buffer, sizeof(packet));
        count = 0;
        if (packet.sync != Sync) continue;
        if (!loggedPacket) {
            ESP_LOGI(Tag, "QEMU input packet received on UART1");
            loggedPacket = true;
        }
        state.game.mouseDx = static_cast<float>(packet.mouseDx);
        state.game.mouseDy = static_cast<float>(packet.mouseDy);
        state.game.forward = packet.buttons & (1u << 0);
        state.game.backward = packet.buttons & (1u << 1);
        state.game.left = packet.buttons & (1u << 2);
        state.game.right = packet.buttons & (1u << 3);
        state.game.jumpPressed = packet.buttons & (1u << 4);
        state.game.breakPressed = packet.buttons & (1u << 5);
        state.game.placePressed = packet.buttons & (1u << 6);
        for (int slot = 0; slot < 4; ++slot) state.game.selectPressed[slot] = packet.select & (1u << slot);
    }
}
}

extern "C" void app_main() {
    ESP_LOGI(Tag, "ESP32-S3 firmware game starting; QEMU input is UART1 binary bridge");
    setupInput();

    esp_lcd_panel_handle_t panel = nullptr;
    const esp_lcd_rgb_qemu_config_t displayConfig = {
        .width = QemuWidth,
        .height = QemuHeight,
        .bpp = RGB_QEMU_BPP_16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_qemu(&displayConfig, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    void* framebuffer = nullptr;
    ESP_ERROR_CHECK(esp_lcd_rgb_qemu_get_frame_buffer(panel, &framebuffer));
    if (!framebuffer) {
        ESP_LOGE(Tag, "QEMU framebuffer unavailable");
        return;
    }

    mine::Game game;
    InputState input;
    int64_t previous = esp_timer_get_time();
    int64_t fpsPrevious = previous;
    int fpsFrames = 0;
    int fps = 0;
    while (true) {
        pollInput(input);
        int64_t now = esp_timer_get_time();
        float dt = static_cast<float>(now - previous) / 1000000.0f;
        previous = now;
        game.update(input.game, dt);
        game.render(CoreFramebuffer);
        ++fpsFrames;
        if (now - fpsPrevious >= 500000) {
            fps = static_cast<int>(fpsFrames * 1000000LL / (now - fpsPrevious));
            fpsFrames = 0;
            fpsPrevious = now;
        }
        mine::drawFps(CoreFramebuffer, fps);
        auto* qemuFramebuffer = static_cast<uint16_t*>(framebuffer);
        for (int y = 0; y < mine::ScreenHeight; ++y) {
            for (int x = 0; x < mine::ScreenWidth; ++x) {
                const uint16_t pixel = CoreFramebuffer[y * mine::ScreenWidth + x];
                for (int sy = 0; sy < QemuScale; ++sy) {
                    const int row = (y * QemuScale + sy) * QemuWidth;
                    for (int sx = 0; sx < QemuScale; ++sx)
                        qemuFramebuffer[row + x * QemuScale + sx] = pixel;
                }
            }
        }
        ESP_ERROR_CHECK(esp_lcd_rgb_qemu_refresh(panel));
        input.game.mouseDx = 0.0f;
        input.game.mouseDy = 0.0f;
        input.game.jumpPressed = false;
        input.game.breakPressed = false;
        input.game.placePressed = false;
        for (bool& selected : input.game.selectPressed) selected = false;
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
