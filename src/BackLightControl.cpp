#include "BackLightControl.h"

// 动态计算最大占空比，基于 LEDC_TIMER_BIT 位数
#define MAX_DUTY_CYCLE ((1 << LEDC_TIMER_BIT) - 1)

void BacklightControl::begin() {
    if (!_initialized) {
        // 配置 LEDC 通道并附加到 GPIO
        ledcAttachChannel(LCD_BL, LEDC_BASE_FREQ, LEDC_TIMER_BIT, LEDC_CHANNEL_0);
        
        // 设置初始值
        _currentBrightness = 0;
        ledcWrite(LCD_BL, _currentBrightness);
        
        _initialized = true;
        
        #ifdef DEBUG_MODE
        Serial.println("Backlight initialized on channel " + String(LEDC_CHANNEL_0));
        #endif
    }
}

void BacklightControl::update() {
    if (!_initialized || !_isFading) return;

    uint32_t currentTime = millis();
    uint32_t elapsedTime = currentTime - _fadeStartTime;

    if (elapsedTime >= _fadeDuration) {
        // 渐变结束
        _currentBrightness = _targetBrightness;
        ledcWrite(LCD_BL, _currentBrightness);
        _isFading = false;
        
        #ifdef DEBUG_MODE
        Serial.printf("Fade complete. Brightness: %d\n", _currentBrightness);
        #endif
    } else {
        // 计算当前应该的亮度，使用浮点数确保平滑渐变
        float progress = (float)elapsedTime / _fadeDuration;
        float brightnessDiff = _targetBrightness - _startBrightness;  // 使用起始亮度
        _currentBrightness = _startBrightness + (brightnessDiff * progress);
        
        // 将当前计算的亮度写入LEDC
        ledcWrite(LCD_BL, _currentBrightness);
        
        #ifdef DEBUG_MODE
        if(elapsedTime % 100 == 0) {  // 每100ms打印一次调试信息
            Serial.printf("Fading... Progress: %.2f%%, Brightness: %d\n", 
                        progress * 100, _currentBrightness);
        }
        #endif
    }
}

bool BacklightControl::setBacklight(uint8_t targetPercent, float fadeTime) {
    if (!_initialized) {
        begin();
    }

    // 限制目标亮度百分比在 0-100 之间
    targetPercent = constrain(targetPercent, 0, 100);

    // 将百分比转换为 0 到 MAX_DUTY_CYCLE 的 PWM 值
    uint8_t newTargetBrightness = map(targetPercent, 0, 100, 0, MAX_DUTY_CYCLE);

    // 如果目标亮度与当前亮度相同，不需要渐变
    if (newTargetBrightness == _currentBrightness) {
        return true;
    }

    // 保存起始亮度，用于渐变计算
    _startBrightness = _currentBrightness;
    _targetBrightness = newTargetBrightness;
    _fadeDuration = fadeTime;
    _fadeStartTime = millis();
    _isFading = true;

    #ifdef DEBUG_MODE
    Serial.printf("Starting fade from %d to %d over %.0fms\n", 
                 _startBrightness, _targetBrightness, fadeTime);
    #endif

    return true;
}

// 全局函数实现，用于保持与现有代码的兼容性
void initLEDC() {
    BacklightControl::getInstance().begin();
}

bool setBacklight(uint8_t targetPercent, float fadeTime) {
    return BacklightControl::getInstance().setBacklight(targetPercent, fadeTime);
}