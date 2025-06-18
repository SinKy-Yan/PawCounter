#ifndef CONFIG_H
#define CONFIG_H

#ifndef ARDUINO_H
#include <Arduino.h>
#endif

#include <FastLED.h> 

// =================== 功能开关 ===================
#define DEBUG_MODE                      // 启用调试模式
// #define ENABLE_BATTERY_MANAGER       // 启用电池管理（调试时可注释）

// =================== LCD 引脚定义 ===================
#define LCD_RST   48
#define LCD_CS    45
#define LCD_DC    38
#define LCD_SCK   39
#define LCD_BL    42
#define LCD_MOSI  41

// =================== 显示屏参数 ===================
#define DISPLAY_WIDTH 480
#define DISPLAY_HEIGHT 128

// =================== 按键和LED引脚定义 ===================
#define SCAN_PL_PIN   15
#define SCAN_CE_PIN   17
#define SCAN_CLK_PIN  16
#define SCAN_MISO_PIN 18
#define BUZZ_PIN      4

#define RGB_PIN 46
#define NUM_LEDS 22        // 总共22个LED
#define LED_BRIGHTNESS 100 // LED默认亮度 (0-255)
#define LED_FADE_DURATION 500  // LED渐变持续时间(ms)
extern CRGB leds[NUM_LEDS];

// =================== 电池管理引脚定义 ===================
// TP4056状态引脚定义
#define TP4056_STDBY_PIN  9   // 充电完成指示
#define TP4056_CHRG_PIN   10  // 充电状态指示

// MAX17048引脚定义
#define MAX17048_SDA_PIN  11  // I2C SDA
#define MAX17048_SCL_PIN  12  // I2C SCL

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