/**
 * @file KeypadControl.cpp
 * @brief 键盘控制库的实现文件
 * 
 * 实现了键盘控制的各项功能，包括：
 * - 硬件初始化和配置
 * - 按键扫描和状态检测
 * - 按键事件处理
 * - LED效果控制
 * - 蜂鸣器控制
 * 
 * 硬件连接：
 * - 移位寄存器时钟线：SCAN_CLK_PIN
 * - 移位寄存器片选线：SCAN_CE_PIN
 * - 移位寄存器并行加载：SCAN_PL_PIN
 * - 移位寄存器数据输入：SCAN_MISO_PIN
 * - RGB LED数据线：RGB_PIN
 * - 蜂鸣器控制线：BUZZ_PIN
 * 
 * @author Your Name
 * @date 2024-01-07
 */

#include "KeypadControl.h"
#include "Logger.h"

// 按键位置映射表定义
const uint8_t KeypadControl::KEY_POSITIONS[] = {
    8,   // Key 1:  第8位
    7,   // Key 2:  第7位
    6,   // Key 3:  第6位
    5,   // Key 4:  第5位
    4,   // Key 5:  第4位
    3,   // Key 6:  第3位
    2,   // Key 7:  第2位
    1,   // Key 8:  第1位
    16,  // Key 9:  第16位
    15,  // Key 10: 第15位
    14,  // Key 11: 第14位
    13,  // Key 12: 第13位
    12,  // Key 13: 第12位
    11,  // Key 14: 第11位
    10,  // Key 15: 第10位
    9,   // Key 16: 第9位
    24,  // Key 17: 第24位
    23,  // Key 18: 第23位
    22,  // Key 19: 第22位
    21,  // Key 20: 第21位
    20,  // Key 21: 第20位
    19   // Key 22: 第19位
};

// 钢琴音阶频率表定义（C4到A5，22个音符）
const uint16_t KeypadControl::PIANO_TONES[] = {
    262,  // Key 1:  C4
    277,  // Key 2:  C#4
    294,  // Key 3:  D4
    311,  // Key 4:  D#4
    330,  // Key 5:  E4
    349,  // Key 6:  F4
    370,  // Key 7:  F#4
    392,  // Key 8:  G4
    415,  // Key 9:  G#4
    440,  // Key 10: A4
    466,  // Key 11: A#4
    494,  // Key 12: B4
    523,  // Key 13: C5
    554,  // Key 14: C#5
    587,  // Key 15: D5
    622,  // Key 16: D#5
    659,  // Key 17: E5
    698,  // Key 18: F5
    740,  // Key 19: F#5
    784,  // Key 20: G5
    831,  // Key 21: G#5
    880   // Key 22: A5
};

KeypadControl::KeypadControl()
    : _currentState(0xFFFFFF), 
      _lastState(0xFFFFFF),
      _debouncedState(0xFFFFFF),
      _lastDebounceTime(0),
      _lastUpdateTime(0),
      _pressedKeyCount(0),
      _eventCallback(nullptr),
      _repeatDelay(DEFAULT_REPEAT_DELAY),
      _repeatRate(DEFAULT_REPEAT_RATE),
      _longPressDelay(DEFAULT_LONGPRESS_DELAY),
      _globalBrightness(255) {
    
    // 初始化按键状态数组
    memset(_keyStates, 0, sizeof(_keyStates));
    // 初始化LED效果数组
    memset(_ledEffects, 0, sizeof(_ledEffects));
    
    // 初始化蜂鸣器配置
    _buzzerConfig = {
        .enabled = true,
        .followKeypress = true,
        .dualTone = false,
        .mode = BUZZER_MODE_NORMAL,  // 默认普通模式
        .volume = BUZZER_MEDIUM,
        .pressFreq = 2000,      // 2kHz按下音调
        .releaseFreq = 1500,    // 1.5kHz释放音调
        .duration = 50          // 50ms持续时间
    };
}

void KeypadControl::begin() {
    KEYPAD_LOG_I("正在初始化按键控制系统");
    
    // 初始化引脚
    pinMode(SCAN_PL_PIN, OUTPUT);
    pinMode(SCAN_CE_PIN, OUTPUT);
    pinMode(SCAN_CLK_PIN, OUTPUT);
    pinMode(SCAN_MISO_PIN, INPUT);
    KEYPAD_LOG_D("GPIO引脚配置完成");

    // 设置初始状态
    digitalWrite(SCAN_PL_PIN, HIGH);
    digitalWrite(SCAN_CE_PIN, LOW);
    digitalWrite(SCAN_CLK_PIN, LOW);
    KEYPAD_LOG_D("初始引脚状态设置完成");

    // 初始化蜂鸣器LEDC
    ledcSetup(BUZZER_CHANNEL, 2000, 8);  // 通道2，2kHz，8位分辨率
    ledcAttachPin(BUZZ_PIN, BUZZER_CHANNEL);
    _buzzerActive = false;
    _buzzerEndTime = 0;
    KEYPAD_LOG_D("蜂鸣器LEDC初始化完成");
    
    KEYPAD_LOG_I("按键控制系统初始化成功");
}

void KeypadControl::update() {
    uint32_t currentTime = millis();
    
    // 控制更新频率
    if (currentTime - _lastUpdateTime >= UPDATE_INTERVAL) {
        // 读取当前按键状态
        _currentState = readShiftRegisters();
        
        // 只在状态改变时输出调试信息，避免刷屏
        static uint32_t lastDebugState = 0xFFFFFF;
        if (_currentState != lastDebugState) {
            KEYPAD_LOG_V("原始扫描状态: 0x%06X", _currentState);
            lastDebugState = _currentState;
        }
        
        // 去抖动处理
        if (currentTime - _lastDebounceTime >= DEBOUNCE_DELAY) {
            if (_currentState != _lastState) {
                _lastDebounceTime = currentTime;
                _lastState = _currentState;
            } else {
                // 状态稳定，进行处理
                if (_currentState != _debouncedState) {
                    _debouncedState = _currentState;
                    checkKeyStates(_debouncedState);
                }
            }
        }
        
        // 更新按键状态
        updateKeyStates();
        
        // 检查组合键
        checkComboKeys();
        
        // 处理自动重复
        updateAutoRepeat();
        
        _lastUpdateTime = currentTime;
    }

    // 处理非阻塞蜂鸣器
    updateBuzzer();
}

void KeypadControl::checkKeyStates(uint32_t buttonState) {
    _pressedKeyCount = 0;
    
    KEYPAD_LOG_V("检查按键状态: 0x%06X", buttonState);
    
    // 检查每个按键的状态变化
    for (uint8_t i = 0; i < 22; i++) {
        bool isPressed = !(buttonState & (1UL << (KEY_POSITIONS[i] - 1)));
        
        // 输出详细的位检查信息
        if (isPressed) {
            KEYPAD_LOG_V("按键 %d (位置 %d, 位 %d, 掩码 0x%06X) 检测为按下状态", 
                        i + 1, KEY_POSITIONS[i], KEY_POSITIONS[i] - 1, 
                        1UL << (KEY_POSITIONS[i] - 1));
        }
        
        if (isPressed != _keyStates[i].pressed) {
            _keyStates[i].pressed = isPressed;
            
            if (isPressed) {
                _keyStates[i].pressTime = millis();
                _keyStates[i].longPressed = false;
                _pressedKeys[_pressedKeyCount++] = i + 1;
                handleKeyEvent(KEY_EVENT_PRESS, i + 1);
            } else {
                handleKeyEvent(KEY_EVENT_RELEASE, i + 1);
            }
        } else if (isPressed) {
            // 只有在状态没有变化但仍处于按下状态时，才添加到当前按下键列表
            // 这用于多键组合检测（避免重复添加）
            _pressedKeys[_pressedKeyCount++] = i + 1;
        }
    }
}

void KeypadControl::updateKeyStates() {
    uint32_t currentTime = millis();
    
    // 检查长按和自动重复
    for (uint8_t i = 0; i < 22; i++) {
        if (_keyStates[i].pressed) {
            // 检查长按
            if (!_keyStates[i].longPressed && 
                (currentTime - _keyStates[i].pressTime >= _longPressDelay)) {
                _keyStates[i].longPressed = true;
                handleKeyEvent(KEY_EVENT_LONGPRESS, i + 1);
            }
        }
    }
}

void KeypadControl::checkComboKeys() {
    if (_pressedKeyCount > 1 && _pressedKeyCount <= 5) {
        // 复制当前按下的键到组合键缓冲区
        memcpy(_comboBuffer, _pressedKeys, _pressedKeyCount);
        // 触发组合键事件 - 使用第一个键作为主键
        if (_eventCallback) {
            _eventCallback(KEY_EVENT_COMBO, _comboBuffer[0], _comboBuffer, _pressedKeyCount);
        }
    }
}

void KeypadControl::updateAutoRepeat() {
    uint32_t currentTime = millis();
    
    for (uint8_t i = 0; i < 22; i++) {
        if (_keyStates[i].pressed && _keyStates[i].autoRepeat) {
            uint32_t pressedTime = currentTime - _keyStates[i].pressTime;
            
            if (pressedTime >= _repeatDelay) {
                uint32_t timeSinceLastRepeat = currentTime - _keyStates[i].lastRepeat;
                if (_keyStates[i].lastRepeat == 0 || timeSinceLastRepeat >= _repeatRate) {
                    handleKeyEvent(KEY_EVENT_REPEAT, i + 1);
                    _keyStates[i].lastRepeat = currentTime;
                }
            }
        }
    }
}

void KeypadControl::handleKeyEvent(KeyEventType type, uint8_t key) {
    // 使用日志系统输出按键事件
    switch (type) {
        case KEY_EVENT_PRESS:
            KEYPAD_LOG_I("按键 %d 被按下", key);
            break;
        case KEY_EVENT_RELEASE:
            KEYPAD_LOG_I("按键 %d 被释放", key);
            break;
        case KEY_EVENT_LONGPRESS:
            KEYPAD_LOG_I("按键 %d 长按", key);
            break;
        case KEY_EVENT_REPEAT:
            KEYPAD_LOG_D("按键 %d 重复", key);
            break;
        case KEY_EVENT_COMBO:
            KEYPAD_LOG_I("按键 %d 组合键", key);
            break;
        default:
            KEYPAD_LOG_W("按键 %d 未知事件", key);
            break;
    }
    
    // 触发回调
    if (_eventCallback) {
        _eventCallback(type, key, nullptr, 0);
    }
    
    // 处理按键反馈
    if (_keyFeedback[key - 1].enabled) {
        // LED反馈
        if (_keyFeedback[key - 1].ledMode != LED_INSTANT) {
            handleLEDEffect(key - 1, _keyFeedback[key - 1].ledMode, 
                          _keyFeedback[key - 1].color);
        }
        
        // 蜂鸣器反馈
        if (_buzzerConfig.followKeypress) {
            uint16_t freq = _buzzerConfig.pressFreq;  // 默认频率
            
            // 根据蜂鸣器模式选择频率
            if (_buzzerConfig.mode == BUZZER_MODE_PIANO && key >= 1 && key <= 22) {
                freq = PIANO_TONES[key - 1];  // 使用钢琴音调
            }
            
            switch (type) {
                case KEY_EVENT_PRESS:
                    startBuzzer(freq, _buzzerConfig.duration);
                    break;
                    
                case KEY_EVENT_RELEASE:
                    if (_buzzerConfig.dualTone) {
                        uint16_t releaseFreq = (_buzzerConfig.mode == BUZZER_MODE_PIANO && key >= 1 && key <= 22) 
                                             ? PIANO_TONES[key - 1] * 0.8  // 钢琴模式下释放音调略低
                                             : _buzzerConfig.releaseFreq;
                        startBuzzer(releaseFreq, _buzzerConfig.duration);
                    }
                    break;
                    
                case KEY_EVENT_LONGPRESS:
                    startBuzzer(freq * 1.2, _buzzerConfig.duration * 1.5);  // 长按音调略高
                    break;
                    
                case KEY_EVENT_REPEAT:
                    startBuzzer(freq, _buzzerConfig.duration / 2);
                    break;
                    
                default:
                    break;
            }
        }
    }
}

void KeypadControl::configureBuzzer(const BuzzerConfig& config) {
    _buzzerConfig = config;
}

void KeypadControl::setBuzzerVolume(BuzzerVolume volume) {
    _buzzerConfig.volume = volume;
}

void KeypadControl::setBuzzerFollowKey(bool enable, bool dualTone) {
    _buzzerConfig.followKeypress = enable;
    _buzzerConfig.dualTone = dualTone;
}

void KeypadControl::setBuzzerMode(BuzzerMode mode) {
    _buzzerConfig.mode = mode;
    KEYPAD_LOG_I("蜂鸣器模式设置为: %s", (mode == BUZZER_MODE_PIANO) ? "钢琴模式" : "普通模式");
}

uint8_t KeypadControl::getVolumeDuty(BuzzerVolume volume) const {
    switch (volume) {
        case BUZZER_MUTE:   return 0;
        case BUZZER_LOW:    return 85;    // 33% duty cycle
        case BUZZER_MEDIUM: return 127;   // 50% duty cycle
        case BUZZER_HIGH:   return 255;   // 100% duty cycle
        default:            return 127;
    }
}

void KeypadControl::startBuzzer(uint16_t freq, uint16_t duration) {
    if (!_buzzerConfig.enabled) return;
    
    // 设置频率和占空比
    ledcWriteTone(BUZZER_CHANNEL, freq);
    uint8_t duty = getVolumeDuty(_buzzerConfig.volume);
    ledcWrite(BUZZER_CHANNEL, duty);
    
    // 设置结束时间
    _buzzerActive = true;
    _buzzerEndTime = millis() + duration;
    
    KEYPAD_LOG_D("蜂鸣器启动: 频率=%d Hz, 持续时间=%d ms, 占空比=%d", freq, duration, duty);
}

void KeypadControl::updateBuzzer() {
    if (_buzzerActive && millis() >= _buzzerEndTime) {
        // 停止蜂鸣器
        ledcWrite(BUZZER_CHANNEL, 0);
        _buzzerActive = false;
        KEYPAD_LOG_D("蜂鸣器停止");
    }
}

void KeypadControl::handleLEDEffect(uint8_t ledIndex, LEDMode mode, CRGB color) {
    if (ledIndex >= NUM_LEDS) return;
    
    LEDEffect& effect = _ledEffects[ledIndex];
    effect.active = true;
    effect.startTime = millis();
    effect.mode = mode;
    effect.color = color;
    
    switch (mode) {
        case LED_INSTANT:
            leds[ledIndex] = color;
            FastLED.show();
            break;
            
        case LED_FADE:
            effect.brightness = 255;
            effect.duration = LED_FADE_DURATION;
            leds[ledIndex] = color;
            FastLED.show();
            break;
            
        case LED_BREATH:
            effect.brightness = 0;
            effect.duration = 1000;
            break;
            
        case LED_BLINK:
            effect.duration = 200;
            leds[ledIndex] = color;
            FastLED.show();
            break;
    }
}

void KeypadControl::updateLEDEffects() {
    uint32_t currentTime = millis();
    bool needUpdate = false;

    for (int i = 0; i < NUM_LEDS; i++) {
        if (_ledEffects[i].active) {
            uint32_t elapsed = currentTime - _ledEffects[i].startTime;
            
            switch (_ledEffects[i].mode) {
                case LED_INSTANT:
                    if (elapsed >= 50) {  // 50ms后熄灭
                        leds[i] = CRGB::Black;
                        _ledEffects[i].active = false;
                        needUpdate = true;
                    }
                    break;

                case LED_FADE:
                    if (elapsed >= _ledEffects[i].duration) {
                        leds[i] = CRGB::Black;
                        _ledEffects[i].active = false;
                        needUpdate = true;
                    } else {
                        float fadeProgress = (float)elapsed / _ledEffects[i].duration;
                        uint8_t brightness = 255 * (1.0 - fadeProgress);
                        leds[i] = _ledEffects[i].color;
                        leds[i].nscale8(brightness);
                        needUpdate = true;
                    }
                    break;

                case LED_BREATH:
                    {
                        float breathProgress = (float)(elapsed % _ledEffects[i].duration) / _ledEffects[i].duration;
                        float brightness = sin(breathProgress * PI) * 255;
                        leds[i] = _ledEffects[i].color;
                        leds[i].nscale8(abs(brightness));
                        needUpdate = true;
                    }
                    break;

                case LED_BLINK:
                    if (elapsed >= _ledEffects[i].duration) {
                        _ledEffects[i].startTime = currentTime;
                        leds[i] = (elapsed / _ledEffects[i].duration) % 2 ? 
                                 _ledEffects[i].color : CRGB::Black;
                        needUpdate = true;
                    }
                    break;
            }
        }
    }

    if (needUpdate) {
        FastLED.show();
    }
}

// 其他基本功能实现
void KeypadControl::enableAutoRepeat(uint8_t key, bool enable) {
    if (key > 0 && key <= 22) {
        _keyStates[key - 1].autoRepeat = enable;
    }
}

bool KeypadControl::isKeyPressed(uint8_t key) const {
    if (key > 0 && key <= 22) {
        return _keyStates[key - 1].pressed;
    }
    return false;
}

bool KeypadControl::isKeyLongPressed(uint8_t key) const {
    if (key > 0 && key <= 22) {
        return _keyStates[key - 1].longPressed;
    }
    return false;
}

void KeypadControl::setKeyFeedback(uint8_t keyNumber, const KeyFeedback& feedback) {
    if (keyNumber > 0 && keyNumber <= 22) {
        _keyFeedback[keyNumber - 1] = feedback;
    }
}

void KeypadControl::setGlobalBrightness(uint8_t brightness) {
    _globalBrightness = brightness;
    FastLED.setBrightness(_globalBrightness);
    FastLED.show();
}

uint32_t KeypadControl::readShiftRegisters() {
    uint32_t result = 0;

    // 锁存数据
    digitalWrite(SCAN_PL_PIN, LOW);   
    delayMicroseconds(5);
    digitalWrite(SCAN_PL_PIN, HIGH);  

    // 读取24位数据
    for (uint8_t i = 0; i < 24; i++) {
        result <<= 1;
        if (digitalRead(SCAN_MISO_PIN)) {
            result |= 1;
        }
        digitalWrite(SCAN_CLK_PIN, HIGH);
        delayMicroseconds(5);
        digitalWrite(SCAN_CLK_PIN, LOW);
        delayMicroseconds(5);
    }

    return result;
}

uint8_t KeypadControl::getPressedKeys(uint8_t* keyBuffer, uint8_t bufferSize) const {
    uint8_t count = min(bufferSize, _pressedKeyCount);
    if (count > 0 && keyBuffer != nullptr) {
        memcpy(keyBuffer, _pressedKeys, count);
    }
    return count;
}

#ifdef DEBUG_MODE
void KeypadControl::testAllBits() {
    Serial.println(F("=== 测试所有 24 位 ==="));
    uint32_t state = readShiftRegisters();
    Serial.print(F("原始寄存器状态: 0x"));
    Serial.println(state, HEX);
    
    for (int bit = 0; bit < 24; bit++) {
        bool bitState = !(state & (1UL << bit));  // 按键按下时为低电平
        if (bitState) {
            Serial.print(F("位 "));
            Serial.print(bit);
            Serial.println(F(" 激活 (按下)"));
        }
    }
    Serial.println(F("=== 测试结束 ==="));
}

void KeypadControl::printDebugInfo() {
    Serial.println(F("键盘控制调试信息:"));
    Serial.print(F("当前状态: 0x"));
    Serial.println(_currentState, HEX);
    Serial.print(F("按下的按键: "));
    for (uint8_t i = 0; i < _pressedKeyCount; i++) {
        Serial.print(_pressedKeys[i]);
        Serial.print(" ");
    }
    Serial.println();
    
    Serial.println(F("活跃的LED效果:"));
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        if (_ledEffects[i].active) {
            Serial.print(F("LED "));
            Serial.print(i);
            Serial.print(F(": 模式="));
            Serial.print(_ledEffects[i].mode);
            Serial.println();
        }
    }

    Serial.println(F("蜂鸣器配置:"));
    Serial.print(F("启用: "));
    Serial.println(_buzzerConfig.enabled);
    Serial.print(F("跟随按键: "));
    Serial.println(_buzzerConfig.followKeypress);
    Serial.print(F("双音调: "));
    Serial.println(_buzzerConfig.dualTone);
    Serial.print(F("音量: "));
    Serial.println(_buzzerConfig.volume);
}
#endif