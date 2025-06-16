#include "Initialization.h"
#include "config.h"
#include "BackLightControl.h"
#include <FastLED.h>

// 定义 leds 数组
CRGB leds[NUM_LEDS];

// 声明全局变量
Arduino_DataBus *bus = nullptr;
Arduino_GFX *gfx = nullptr;
Arduino_GFX *gfx_canvas = nullptr;

// 初始化 LED 灯
void initLEDs() {
    FastLED.addLeds<WS2812, RGB_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear();
    FastLED.show();
    delay(10);  // 给FastLED一些时间完成初始化
}

// 按键引脚初始化
void initButtons() {
    pinMode(SCAN_PL_PIN, OUTPUT);
    pinMode(SCAN_CE_PIN, OUTPUT);
    pinMode(SCAN_CLK_PIN, OUTPUT);
    pinMode(SCAN_MISO_PIN, INPUT);
    pinMode(BUZZ_PIN, OUTPUT);

    digitalWrite(SCAN_PL_PIN, HIGH);
    digitalWrite(SCAN_CE_PIN, LOW);
    digitalWrite(SCAN_CLK_PIN, LOW);
}

// 显示屏初始化
void initDisplay() {
    if (gfx != nullptr) {
        return;
    }

    bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCK, LCD_A0, LCD_A1, LCD_A2, LCD_A3);
    if (!bus) {
        Serial.println("Failed to create bus!");
        return;
    }

    gfx = new Arduino_NV3041A(bus, LCD_RST, 0, false, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, 0, 0);
    if (!gfx) {
        Serial.println("Failed to create GFX!");
        return;
    }

    gfx_canvas = new Arduino_Canvas(DISPLAY_WIDTH, DISPLAY_HEIGHT, gfx);
    if (!gfx_canvas || !gfx_canvas->begin()) {
        Serial.println("Failed to initialize gfx canvas!");
        return;
    }

    // 初始化显示
    gfx_canvas->fillScreen(BLACK);
    gfx_canvas->flush();
    Serial.println("Display initialized successfully");
}


// 播放马里奥音效
void playMarioStartup() {
    const int marioNotes[] = {
        2637, 2637, 0, 2637, 0, 2093, 2637, 0,
        3136, 0, 0, 0, 1568, 0, 0, 0
    };
    const int marioDurations[] = {
        12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12
    };

    for (int i = 0; i < 16; i++) {
        if (marioNotes[i] == 0) {
            delay(1000 / marioDurations[i]);
        } else {
            tone(BUZZ_PIN, marioNotes[i], 1000 / marioDurations[i]);
            delay(1000 / marioDurations[i] * 1.3);
        }
        noTone(BUZZ_PIN);
    }
}

// 播放启动动画
void playStartupAnimation() {
    FastLED.clear();

    // 所有灯白色闪烁一次，然后全灭
    // 全部亮起白色
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::White;
    }
    FastLED.show();
    delay(500);  // 亮起0.5秒

    // 全部熄灭
    FastLED.clear();
    FastLED.show();
    delay(500);  // 熄灭0.5秒
}