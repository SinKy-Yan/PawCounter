#ifndef INITIALIZATION_H
#define INITIALIZATION_H

#ifndef ARDUINO_H
#include <Arduino.h>
#endif

#include <FastLED.h>
#include <Arduino_GFX_Library.h>
#include "FreeSansBold10pt7b.h"
#include "config.h"

// 创建LED数组
extern CRGB leds[NUM_LEDS];

// 显示屏初始化
extern Arduino_DataBus *bus;
extern Arduino_GFX *gfx;
extern Arduino_GFX *gfx_canvas;

// 初始化各个模块的函数声明
void initLEDs();             // LED 初始化
void initDisplay();          // 显示屏初始化
void playStartupAnimation(); // 播放启动动画
void playMarioStartup();     // 播放马里奥音效

#endif // INITIALIZATION_H