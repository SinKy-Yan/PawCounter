#ifndef CONFIG_H
#define CONFIG_H

#ifndef ARDUINO_H
#include <Arduino.h>
#endif

#include <FastLED.h> 

// =================== 功能开关 ===================
#define DEBUG_MODE                      // 启用调试模式
#define USB_HID_ENABLED                 // 启用USB HID功能


// =================== WiFi和OTA配置 ===================
#ifdef OTA_ENABLED
#define WIFI_SSID     "2001C_2.4G"     // WiFi网络名称
#define WIFI_PASSWORD "741852963Su"    // WiFi密码
#endif

// =================== LCD 引脚定义 ===================
#define LCD_RST   48
#define LCD_CS    45
#define LCD_DC    38
#define LCD_SCK   39
#define LCD_BL    42
#define LCD_MOSI  41

// =================== 显示屏参数 ===================
#define DISPLAY_WIDTH 480
#define DISPLAY_HEIGHT 135

// =================== 按键和LED引脚定义 ===================
#define SCAN_PL_PIN   15
#define SCAN_CE_PIN   17
#define SCAN_CLK_PIN  16
#define SCAN_MISO_PIN 18
#define BUZZ_PIN      4

#define RGB_PIN 46
#define NUM_LEDS 22        // 总共22个LED
#define LED_BRIGHTNESS 255 // LED默认亮度 (0-255)
#define LED_FADE_DURATION 500  // LED渐变持续时间(ms)
extern CRGB leds[NUM_LEDS];

// =================== USB HID引脚定义 ===================
#ifdef USB_HID_ENABLED
#define USB_DN_PIN    19              // USB D- (GPIO19)
#define USB_DP_PIN    20              // USB D+ (GPIO20)
// 注意：ESP32-S3的GPIO19和GPIO20是专用USB引脚，
// 当启用USB HID功能时会自动配置为USB信号
#endif


// =================== 硬件控制参数 ===================
// 背光控制通道参数设置
#define LEDC_TIMER_BIT 8    // 8 位分辨率（0-255）
#define LEDC_BASE_FREQ 1000 // 1kHz 频率
#define LEDC_CHANNEL_0 1       // 修改 LEDC 通道为 1，避免与 RMT 冲突

// 蜂鸣器配置
#define BUZZER_CHANNEL 2        // 使用LEDC通道2

// =================== 按键布局矩阵 ===================
const int KEY_MATRIX[5][5] = {
    {0, 5, 9, 14, 18},    // KEY1,KEY6,KEY10,KEY15,KEY19
    {1, 6, 10, 15, 19},   // KEY2,KEY7,KEY11,KEY16,KEY20
    {2, 7, 11, 16, 20},   // KEY3,KEY8,KEY12,KEY17,KEY21
    {3, 8, 12, -1, -1},   // KEY4,KEY9,KEY13
    {4, -1, 13, 17, 21}   // KEY5,KEY14,KEY18,KEY22
};

#endif // CONFIG_H