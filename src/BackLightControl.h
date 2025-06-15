#ifndef BACKLIGHT_CONTROL_H
#define BACKLIGHT_CONTROL_H

#ifndef ARDUINO_H
#include <Arduino.h>
#endif

#include "config.h"

// 检查是否定义了 LCD_BL
#ifndef LCD_BL
    #error "LCD_BL pin is not defined. Please define the LCD_BL pin in config.h or another header."
#endif

class BacklightControl {
public:
    static BacklightControl& getInstance() {
        static BacklightControl instance;
        return instance;
    }

    void begin();
    void update();  // 新增：需要在主循环中调用以更新渐变效果
    bool setBacklight(uint8_t targetPercent, float fadeTime = 500);
    uint8_t getCurrentBrightness() const { return _currentBrightness; }

private:
    BacklightControl() {}  // 私有构造函数
    
    bool _initialized = false;
    uint8_t _currentBrightness = 0;
    uint8_t _startBrightness = 0;   // 新增：渐变开始时的亮度
    uint8_t _targetBrightness = 0;
    uint32_t _fadeStartTime = 0;
    uint32_t _fadeDuration = 0;
    bool _isFading = false;
};

// 为了保持与现有代码兼容的全局函数
void initLEDC();
bool setBacklight(uint8_t targetPercent, float fadeTime = 500);

#endif // BACKLIGHT_CONTROL_H