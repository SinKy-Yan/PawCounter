/**
 * @file LEDEffectManager.cpp
 * @brief RGB LED灯效管理系统实现文件
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "LEDEffectManager.h"
#include <math.h>

LEDEffectManager& LEDEffectManager::getInstance() {
    static LEDEffectManager instance;
    return instance;
}

bool LEDEffectManager::begin() {
    LOG_I(TAG_SYSTEM, "Initializing LED Effect Manager...");
    
    _initialized = false;
    _enabled = true;
    _globalBrightness = LED_BRIGHTNESS;
    _groupCount = 0;
    _energySaveActive = false;

    // 初始化LED效果实例
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        _ledEffects[i] = LEDEffectInstance();
        _savedState[i] = CRGB::Black;
    }

    // 初始化LED组
    for (uint8_t i = 0; i < 8; i++) {
        _ledGroups[i] = LEDGroup();
    }

    // 初始化全局效果
    _globalEffect = LEDEffectInstance();

    // 创建预定义LED组
    createDefaultGroups();

    _initialized = true;
    LOG_I(TAG_SYSTEM, "LED Effect Manager initialized successfully");
    
    return true;
}

void LEDEffectManager::update() {
    if (!_initialized || !_enabled) return;

    uint32_t currentTime = millis();

    // 更新全局效果
    if (_globalEffect.active) {
        updateGlobalEffect();
    }

    // 更新单个LED效果
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        if (_ledEffects[i].active) {
            updateSingleEffect(_ledEffects[i], i);
        }
    }

    // 应用更改到硬件
    FastLED.show();
}

bool LEDEffectManager::setLEDEffect(uint8_t ledIndex, const LEDEffectConfig& config) {
    if (!isValidLEDIndex(ledIndex) || !_enabled) return false;

    LEDEffectInstance& instance = _ledEffects[ledIndex];
    instance.config = config;
    instance.startTime = millis();
    instance.lastUpdate = instance.startTime;
    instance.currentRepeat = 0;
    instance.active = true;
    instance.progress = 0.0;
    instance.phase = 0;

    // 对于即时效果（SOLID和OFF），立即应用到LED，无需等待update循环
    if (config.type == LED_EFFECT_SOLID) {
        CRGB color = applyBrightness(config.primaryColor, config.brightness);
        leds[ledIndex] = color;
        instance.currentColor = color;
        FastLED.show();  // 立即显示
    } else if (config.type == LED_EFFECT_OFF) {
        leds[ledIndex] = CRGB::Black;
        instance.currentColor = CRGB::Black;
        instance.active = false;  // OFF效果立即停止
        FastLED.show();  // 立即显示
    }

    LOG_D(TAG_SYSTEM, "Set LED %d effect: type=%d, brightness=%d", 
          ledIndex, config.type, config.brightness);

    return true;
}

void LEDEffectManager::stopLEDEffect(uint8_t ledIndex) {
    if (!isValidLEDIndex(ledIndex)) return;

    _ledEffects[ledIndex].active = false;
    leds[ledIndex] = CRGB::Black;
    FastLED.show();  // 立即显示熄灭效果
    
    LOG_D(TAG_SYSTEM, "Stopped LED %d effect", ledIndex);
}

void LEDEffectManager::setLEDColor(uint8_t ledIndex, CRGB color, uint8_t brightness) {
    if (!isValidLEDIndex(ledIndex) || !_enabled) return;

    LEDEffectConfig config;
    config.type = LED_EFFECT_SOLID;
    config.primaryColor = color;
    config.brightness = brightness;
    config.duration = 0; // 持续显示

    setLEDEffect(ledIndex, config);
}

bool LEDEffectManager::createLEDGroup(const String& name, const uint8_t* ledIndices, uint8_t count) {
    if (_groupCount >= 8 || count == 0 || count > NUM_LEDS) return false;

    LEDGroup& group = _ledGroups[_groupCount];
    group.name = name;
    group.count = count;

    for (uint8_t i = 0; i < count; i++) {
        if (isValidLEDIndex(ledIndices[i])) {
            group.leds[i] = ledIndices[i];
        } else {
            return false; // 无效的LED索引
        }
    }

    _groupCount++;
    LOG_I(TAG_SYSTEM, "Created LED group '%s' with %d LEDs", name.c_str(), count);
    
    return true;
}

bool LEDEffectManager::setGroupEffect(const String& groupName, const LEDEffectConfig& config) {
    LEDGroup* group = findGroup(groupName);
    if (!group) return false;

    for (uint8_t i = 0; i < group->count; i++) {
        setLEDEffect(group->leds[i], config);
    }

    LOG_D(TAG_SYSTEM, "Set group '%s' effect: type=%d", groupName.c_str(), config.type);
    return true;
}

void LEDEffectManager::stopGroupEffect(const String& groupName) {
    LEDGroup* group = findGroup(groupName);
    if (!group) return;

    for (uint8_t i = 0; i < group->count; i++) {
        stopLEDEffect(group->leds[i]);
    }

    LOG_D(TAG_SYSTEM, "Stopped group '%s' effects", groupName.c_str());
}

void LEDEffectManager::setGlobalEffect(const LEDEffectConfig& config) {
    _globalEffect.config = config;
    _globalEffect.startTime = millis();
    _globalEffect.lastUpdate = _globalEffect.startTime;
    _globalEffect.currentRepeat = 0;
    _globalEffect.active = true;
    _globalEffect.progress = 0.0;
    _globalEffect.phase = 0;

    LOG_I(TAG_SYSTEM, "Set global effect: type=%d, duration=%d, speed=%d, active=%d", 
          config.type, config.duration, config.speed, _globalEffect.active);
    
    // 对于闪烁效果，立即应用一次以确保立即可见
    if (config.type == LED_EFFECT_BLINK) {
        applyGlobalEffectToAllLEDs();
        FastLED.show();
        LOG_I(TAG_SYSTEM, "Global blink effect applied immediately");
    }
}

void LEDEffectManager::stopAllEffects() {
    // 停止所有单个LED效果
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        _ledEffects[i].active = false;
        leds[i] = CRGB::Black;
    }

    // 停止全局效果
    _globalEffect.active = false;

    FastLED.show();
    LOG_I(TAG_SYSTEM, "Stopped all LED effects");
}

void LEDEffectManager::setGlobalBrightness(uint8_t brightness) {
    _globalBrightness = brightness;
    FastLED.setBrightness(brightness);
    LOG_D(TAG_SYSTEM, "Set global brightness: %d", brightness);
}

// === 预定义效果实现 ===

void LEDEffectManager::keyPressEffect(uint8_t ledIndex, CRGB color) {
    if (!_enabled) return;

    // 即时亮起，无渐变动画
    LEDEffectConfig config;
    config.type = LED_EFFECT_SOLID;
    config.primaryColor = color;
    config.brightness = 255;
    config.duration = 50;    // 很短的持续时间
    config.speed = 1;        // 最快响应
    config.priority = LED_PRIORITY_NORMAL;

    setLEDEffect(ledIndex, config);
}

void LEDEffectManager::keyReleaseEffect(uint8_t ledIndex) {
    if (!_enabled) return;

    // 即时熄灭，无渐变动画
    stopLEDEffect(ledIndex);  // 直接停止效果，立即熄灭
}

void LEDEffectManager::errorEffect(const uint8_t* ledIndices, uint8_t count) {
    if (!_enabled) return;

    LEDEffectConfig config;
    config.type = LED_EFFECT_BLINK;
    config.primaryColor = CRGB::Red;
    config.brightness = 255;
    config.duration = 1500;
    config.speed = 250;
    config.priority = LED_PRIORITY_HIGH;
    config.repeat = true;
    config.repeatCount = 3;

    if (ledIndices == nullptr || count == 0) {
        // 应用到所有LED
        setGlobalEffect(config);
    } else {
        // 应用到指定LED
        for (uint8_t i = 0; i < count; i++) {
            if (isValidLEDIndex(ledIndices[i])) {
                setLEDEffect(ledIndices[i], config);
            }
        }
    }
}

void LEDEffectManager::successEffect(const uint8_t* ledIndices, uint8_t count) {
    if (!_enabled) return;

    LEDEffectConfig config;
    config.type = LED_EFFECT_PULSE;
    config.primaryColor = CRGB::Green;
    config.brightness = 200;
    config.duration = 800;
    config.speed = 100;
    config.priority = LED_PRIORITY_HIGH;

    if (ledIndices == nullptr || count == 0) {
        setGlobalEffect(config);
    } else {
        for (uint8_t i = 0; i < count; i++) {
            if (isValidLEDIndex(ledIndices[i])) {
                setLEDEffect(ledIndices[i], config);
            }
        }
    }
}

void LEDEffectManager::warningEffect(const uint8_t* ledIndices, uint8_t count) {
    if (!_enabled) return;

    LEDEffectConfig config;
    config.type = LED_EFFECT_BLINK;
    config.primaryColor = CRGB::Yellow;
    config.brightness = 220;
    config.duration = 1000;
    config.speed = 200;
    config.priority = LED_PRIORITY_HIGH;
    config.repeat = true;
    config.repeatCount = 2;

    if (ledIndices == nullptr || count == 0) {
        setGlobalEffect(config);
    } else {
        for (uint8_t i = 0; i < count; i++) {
            if (isValidLEDIndex(ledIndices[i])) {
                setLEDEffect(ledIndices[i], config);
            }
        }
    }
}

void LEDEffectManager::startupEffect() {
    if (!_enabled) return;

    // 全部白光亮起0.5秒然后熄灭0.5秒
    LEDEffectConfig config;
    config.type = LED_EFFECT_BLINK;
    config.primaryColor = CRGB::White;
    config.brightness = 255;
    config.duration = 1000;  // 总持续时间1秒
    config.speed = 500;      // 每0.5秒切换一次（亮0.5秒，灭0.5秒）
    config.priority = LED_PRIORITY_HIGH;
    config.repeat = false;
    config.repeatCount = 1;

    setGlobalEffect(config);
}

void LEDEffectManager::shutdownEffect() {
    if (!_enabled) return;

    LEDEffectConfig config;
    config.type = LED_EFFECT_FADE_OUT;
    config.primaryColor = CRGB::White;
    config.brightness = 255;
    config.duration = 1500;
    config.speed = 50;
    config.priority = LED_PRIORITY_HIGH;

    setGlobalEffect(config);
}

void LEDEffectManager::lowBatteryEffect() {
    if (!_enabled) return;

    LEDEffectConfig config;
    config.type = LED_EFFECT_BREATH;
    config.primaryColor = CRGB::Red;
    config.brightness = 180;
    config.duration = 0; // 持续直到被停止
    config.speed = 1500; // 缓慢呼吸
    config.priority = LED_PRIORITY_CRITICAL;
    config.repeat = true;
    config.repeatCount = 0; // 无限重复

    setGlobalEffect(config);
}

void LEDEffectManager::chargingEffect() {
    if (!_enabled) return;

    LEDEffectConfig config;
    config.type = LED_EFFECT_WAVE;
    config.primaryColor = CRGB::Green;
    config.brightness = 120;
    config.duration = 0; // 持续直到被停止
    config.speed = 800;
    config.priority = LED_PRIORITY_HIGH;
    config.repeat = true;
    config.repeatCount = 0; // 无限重复

    setGlobalEffect(config);
}

void LEDEffectManager::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!enabled) {
        stopAllEffects();
    }
    LOG_I(TAG_SYSTEM, "LED effects %s", enabled ? "enabled" : "disabled");
}

void LEDEffectManager::saveState() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        _savedState[i] = leds[i];
    }
    LOG_D(TAG_SYSTEM, "LED state saved");
}

void LEDEffectManager::restoreState() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = _savedState[i];
    }
    FastLED.show();
    LOG_D(TAG_SYSTEM, "LED state restored");
}

// === 私有方法实现 ===

void LEDEffectManager::updateSingleEffect(LEDEffectInstance& instance, uint8_t ledIndex) {
    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - instance.startTime;

    // 检查是否超时
    if (instance.config.duration > 0 && elapsed >= instance.config.duration) {
        if (instance.config.repeat && 
            (instance.config.repeatCount == 0 || instance.currentRepeat < instance.config.repeatCount)) {
            // 重新开始
            instance.startTime = currentTime;
            instance.progress = 0.0;
            instance.phase = 0;
            instance.currentRepeat++;
        } else {
            // 效果结束
            instance.active = false;
            leds[ledIndex] = CRGB::Black;
            return;
        }
    }

    // 检查是否需要更新
    if (currentTime - instance.lastUpdate < instance.config.speed) {
        return;
    }

    instance.lastUpdate = currentTime;

    // 计算进度
    if (instance.config.duration > 0) {
        instance.progress = (float)elapsed / instance.config.duration;
        instance.progress = constrain(instance.progress, 0.0, 1.0);
    }

    // 计算颜色
    CRGB color = calculateEffectColor(instance);
    color = applyBrightness(color, instance.config.brightness);

    leds[ledIndex] = color;
    instance.currentColor = color;
}

void LEDEffectManager::updateGlobalEffect() {
    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - _globalEffect.startTime;

    // 检查是否超时
    if (_globalEffect.config.duration > 0 && elapsed >= _globalEffect.config.duration) {
        if (_globalEffect.config.repeat && 
            (_globalEffect.config.repeatCount == 0 || _globalEffect.currentRepeat < _globalEffect.config.repeatCount)) {
            // 重新开始
            _globalEffect.startTime = currentTime;
            _globalEffect.progress = 0.0;
            _globalEffect.phase = 0;
            _globalEffect.currentRepeat++;
        } else {
            // 效果结束
            _globalEffect.active = false;
            return;
        }
    }

    // 检查是否需要更新
    if (currentTime - _globalEffect.lastUpdate < _globalEffect.config.speed) {
        return;
    }

    _globalEffect.lastUpdate = currentTime;

    // 计算进度
    if (_globalEffect.config.duration > 0) {
        _globalEffect.progress = (float)elapsed / _globalEffect.config.duration;
        _globalEffect.progress = constrain(_globalEffect.progress, 0.0, 1.0);
    }

    // 应用全局效果到所有LED
    applyGlobalEffectToAllLEDs();
}

CRGB LEDEffectManager::calculateEffectColor(const LEDEffectInstance& instance) {
    CRGB result = CRGB::Black;

    switch (instance.config.type) {
        case LED_EFFECT_OFF:
            result = CRGB::Black;
            break;

        case LED_EFFECT_SOLID:
            result = instance.config.primaryColor;
            break;

        case LED_EFFECT_BLINK:
            {
                uint32_t blinkPeriod = instance.config.speed * 2;
                uint32_t elapsed = millis() - instance.startTime;
                if ((elapsed % blinkPeriod) < instance.config.speed) {
                    result = instance.config.primaryColor;
                } else {
                    result = CRGB::Black;
                }
            }
            break;

        case LED_EFFECT_FADE_IN:
            {
                uint8_t brightness = (uint8_t)(255 * instance.progress);
                result = instance.config.primaryColor;
                result.nscale8(brightness);
            }
            break;

        case LED_EFFECT_FADE_OUT:
            {
                uint8_t brightness = (uint8_t)(255 * (1.0 - instance.progress));
                result = instance.config.primaryColor;
                result.nscale8(brightness);
            }
            break;

        case LED_EFFECT_BREATH:
            {
                float breathValue = sin(instance.progress * 2 * PI) * 0.5 + 0.5;
                uint8_t brightness = (uint8_t)(255 * breathValue);
                result = instance.config.primaryColor;
                result.nscale8(brightness);
            }
            break;

        case LED_EFFECT_PULSE:
            {
                float pulseValue = sin(instance.progress * PI);
                uint8_t brightness = (uint8_t)(255 * pulseValue);
                result = instance.config.primaryColor;
                result.nscale8(brightness);
            }
            break;

        case LED_EFFECT_RAINBOW:
            {
                uint8_t hue = (uint8_t)(255 * instance.progress);
                result = getRainbowColor(hue);
            }
            break;

        default:
            result = instance.config.primaryColor;
            break;
    }

    return result;
}

CRGB LEDEffectManager::applyBrightness(CRGB color, uint8_t brightness) {
    if (brightness == 255) return color;
    
    color.nscale8(brightness);
    return color;
}

CRGB LEDEffectManager::blendColors(CRGB color1, CRGB color2, float ratio) {
    ratio = constrain(ratio, 0.0, 1.0);
    
    uint8_t r = (uint8_t)(color1.r * (1.0 - ratio) + color2.r * ratio);
    uint8_t g = (uint8_t)(color1.g * (1.0 - ratio) + color2.g * ratio);
    uint8_t b = (uint8_t)(color1.b * (1.0 - ratio) + color2.b * ratio);
    
    return CRGB(r, g, b);
}

CRGB LEDEffectManager::getRainbowColor(uint8_t hue) {
    return CHSV(hue, 255, 255);
}

LEDGroup* LEDEffectManager::findGroup(const String& name) {
    for (uint8_t i = 0; i < _groupCount; i++) {
        if (_ledGroups[i].name == name) {
            return &_ledGroups[i];
        }
    }
    return nullptr;
}

bool LEDEffectManager::isValidLEDIndex(uint8_t ledIndex) const {
    return ledIndex < NUM_LEDS;
}

void LEDEffectManager::createDefaultGroups() {
    // 创建数字键组 (0-9)
    uint8_t digitKeys[] = {4, 3, 8, 12, 2, 7, 11, 1, 6, 10}; // Key5,4,9,13,3,8,12,2,7,11对应0-9
    createLEDGroup("digits", digitKeys, 10);

    // 创建运算符组
    uint8_t operatorKeys[] = {17, 16, 15, 21}; // ADD, MUL, SUB, DIV
    createLEDGroup("operators", operatorKeys, 4);

    // 创建功能键组
    uint8_t functionKeys[] = {14, 18, 19, 21}; // C, DEL, +/-, EQ
    createLEDGroup("functions", functionKeys, 4);

    // 创建边框组（外围LED）
    uint8_t borderKeys[] = {0, 5, 9, 14, 18, 21, 20, 16, 15, 10, 6, 1}; 
    createLEDGroup("border", borderKeys, 12);
}

void LEDEffectManager::applyGlobalEffectToAllLEDs() {
    // 根据全局效果类型应用到所有LED
    switch (_globalEffect.config.type) {
        case LED_EFFECT_COLOR_WIPE:
            applyColorWipeEffect();
            break;
            
        case LED_EFFECT_WAVE:
            applyWaveEffect();
            break;
            
        case LED_EFFECT_RAINBOW_CYCLE:
            applyRainbowCycleEffect();
            break;
            
        case LED_EFFECT_BLINK:
            // 闪烁效果应用到所有LED，忽略单独效果
            {
                CRGB color = calculateEffectColor(_globalEffect);
                color = applyBrightness(color, _globalEffect.config.brightness);
                for (uint8_t i = 0; i < NUM_LEDS; i++) {
                    leds[i] = color;
                }
            }
            break;
            
        default:
            // 对于其他效果，应用到所有LED
            for (uint8_t i = 0; i < NUM_LEDS; i++) {
                if (!_ledEffects[i].active) { // 只更新没有单独效果的LED
                    CRGB color = calculateEffectColor(_globalEffect);
                    leds[i] = applyBrightness(color, _globalEffect.config.brightness);
                }
            }
            break;
    }
}

void LEDEffectManager::applyColorWipeEffect() {
    float progress = _globalEffect.progress;
    int totalLEDs = NUM_LEDS;
    int currentLED = (int)(progress * totalLEDs);
    
    for (int i = 0; i < totalLEDs; i++) {
        if (!_ledEffects[i].active) { // 只更新没有单独效果的LED
            if (i <= currentLED) {
                leds[i] = applyBrightness(_globalEffect.config.primaryColor, _globalEffect.config.brightness);
            } else {
                leds[i] = CRGB::Black;
            }
        }
    }
}

void LEDEffectManager::applyWaveEffect() {
    uint32_t elapsed = millis() - _globalEffect.startTime;
    float wavePosition = (float)(elapsed % _globalEffect.config.speed) / _globalEffect.config.speed;
    
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        if (!_ledEffects[i].active) { // 只更新没有单独效果的LED
            float ledPosition = (float)i / NUM_LEDS;
            float distance = abs(ledPosition - wavePosition);
            
            if (distance < 0.2) { // 波浪宽度
                uint8_t brightness = (uint8_t)(255 * (1.0 - distance / 0.2));
                CRGB color = _globalEffect.config.primaryColor;
                color.nscale8(brightness);
                leds[i] = applyBrightness(color, _globalEffect.config.brightness);
            } else {
                leds[i] = CRGB::Black;
            }
        }
    }
}

void LEDEffectManager::applyRainbowCycleEffect() {
    uint32_t elapsed = millis() - _globalEffect.startTime;
    uint8_t hueOffset = (uint8_t)((elapsed / 20) % 256); // 彩虹循环速度
    
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        if (!_ledEffects[i].active) { // 只更新没有单独效果的LED
            uint8_t hue = hueOffset + (i * 256 / NUM_LEDS);
            CRGB color = getRainbowColor(hue);
            leds[i] = applyBrightness(color, _globalEffect.config.brightness);
        }
    }
}