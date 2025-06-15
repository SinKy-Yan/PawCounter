/**
 * @file main.cpp
 * @brief 基本按键测试程序
 * @details 测试按键的基本功能：按下、长按、释放
 */

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "KeypadControl.h"
#include "config.h"
#include "Initialization.h"
#include "BackLightControl.h"

// 创建键盘控制实例
KeypadControl keypad;

/**
 * @brief 按键事件回调函数
 */
void onKeyEvent(KeyEventType type, uint8_t key, uint8_t* combo, uint8_t count) {
    // 清除显示区域
    gfx->fillRect(10, 40, DISPLAY_WIDTH-20, 20, BLACK);
    gfx->setTextColor(GREEN);
    gfx->setCursor(10, 40);
    
    // 显示按键事件
    switch(type) {
        case KEY_EVENT_PRESS:
            gfx->printf("Key %d Pressed", key);
            break;
            
        case KEY_EVENT_RELEASE:
            gfx->printf("Key %d Released", key);
            break;
            
        case KEY_EVENT_LONGPRESS:
            gfx->printf("Key %d Long Press", key);
            break;
            
        default:
            break;
    }
}

/**
 * @brief 设置初始化
 */
void setup() {
    Serial.begin(115200);
    
    // 初始化显示
    initDisplay();

    // 初始化背光控制
    initLEDC();
    setBacklight(30); // 设置100%亮度，500ms渐变时间
    
    // 初始化键盘
    keypad.begin();
    keypad.setKeyEventCallback(onKeyEvent);
    
    // 显示测试说明
    gfx->fillScreen(BLACK);
    gfx->setTextSize(2);
    gfx->setTextColor(YELLOW);
    gfx->setCursor(10, 10);
    gfx->println("Key Test");
}

/**
 * @brief 主循环
 */
void loop() {
    // 更新键盘状态
    keypad.update();
    
    // 更新背光控制
    BacklightControl::getInstance().update();
    
    // 检查串口输入，用于调试
    #ifdef DEBUG_MODE
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 't' || cmd == 'T') {
            keypad.testAllBits();
        }
    }
    #endif
    
    // 短暂延时，保持响应性
    delay(1);
}