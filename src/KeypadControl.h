/**
 * @file KeypadControl.h
 * @brief 键盘控制库，用于处理按键输入、LED反馈和按键音效
 * 
 * 该库提供了完整的键盘控制功能，包括：
 * - 按键扫描和去抖
 * - 多种按键事件检测（按下、释放、长按、自动重复、组合键）
 * - LED反馈效果（即时、渐变、呼吸、闪烁）
 * - 蜂鸣器反馈
 * - 按键状态查询
 * 
 * @author Your Name
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef KEYPAD_CONTROL_H
#define KEYPAD_CONTROL_H

#include <Arduino.h>
#include "config.h"

/**
 * @brief LED效果模式枚举
 */
enum LEDMode {
    LED_INSTANT,    ///< 立即亮起立即熄灭
    LED_FADE,       ///< 渐变效果
    LED_BREATH,     ///< 呼吸效果
    LED_BLINK       ///< 闪烁效果
};

/**
 * @brief 按键事件类型枚举
 */
enum KeyEventType {
    KEY_EVENT_PRESS,      ///< 按键按下
    KEY_EVENT_RELEASE,    ///< 按键释放
    KEY_EVENT_LONGPRESS,  ///< 长按
    KEY_EVENT_REPEAT,     ///< 自动重复
    KEY_EVENT_COMBO       ///< 组合键
};

/**
 * @brief 蜂鸣器音量等级枚举
 */
enum BuzzerVolume {
    BUZZER_MUTE = 0,     ///< 静音
    BUZZER_LOW = 1,      ///< 低音量
    BUZZER_MEDIUM = 2,   ///< 中等音量
    BUZZER_HIGH = 3      ///< 高音量
};

/**
 * @brief 按键反馈配置结构体
 */
struct KeyFeedback {
    bool enabled;           ///< 是否启用反馈
    CRGB color;            ///< LED颜色
    LEDMode ledMode;       ///< LED模式
    uint16_t buzzFreq;     ///< 蜂鸣器频率
    uint16_t buzzDuration; ///< 蜂鸣持续时间
};

/**
 * @brief 蜂鸣器模式枚举
 */
enum BuzzerMode {
    BUZZER_MODE_NORMAL,     ///< 普通模式（固定音调）
    BUZZER_MODE_PIANO       ///< 钢琴模式（按键音阶）
};

/**
 * @brief 蜂鸣器配置结构体
 */
struct BuzzerConfig {
    bool enabled;            ///< 是否启用蜂鸣器
    bool followKeypress;     ///< 是否跟随按键
    bool dualTone;          ///< 是否使用双音效（按下/释放不同音调）
    BuzzerMode mode;        ///< 蜂鸣器模式
    BuzzerVolume volume;    ///< 音量等级
    uint16_t pressFreq;     ///< 按下音调频率（普通模式）
    uint16_t releaseFreq;   ///< 释放音调频率（仅在dualTone为true时使用）
    uint16_t duration;      ///< 蜂鸣持续时间
};

/**
 * @brief 按键事件回调函数类型
 */
typedef void (*KeyEventCallback)(KeyEventType type, uint8_t key, uint8_t* combo, uint8_t count);

/**
 * @brief 键盘控制类
 */
class KeypadControl {
public:
    /**
     * @brief 构造函数
     */
    KeypadControl();

    /**
     * @brief 初始化键盘硬件
     */
    void begin();

    /**
     * @brief 更新键盘状态
     * @details 需要在主循环中定期调用此函数以更新按键状态、LED效果和蜂鸣器
     */
    void update();

    /**
     * @brief 设置按键事件回调函数
     * @param callback 回调函数指针
     */
    void setKeyEventCallback(KeyEventCallback callback) { _eventCallback = callback; }

    /**
     * @brief 启用或禁用按键自动重复
     * @param key 按键编号（1-22）
     * @param enable true启用，false禁用
     */
    void enableAutoRepeat(uint8_t key, bool enable);

    /**
     * @brief 设置自动重复延迟时间
     * @param delay 延迟时间（毫秒）
     */
    void setRepeatDelay(uint16_t delay) { _repeatDelay = delay; }

    /**
     * @brief 设置自动重复速率
     * @param rate 重复间隔（毫秒）
     */
    void setRepeatRate(uint16_t rate) { _repeatRate = rate; }

    /**
     * @brief 设置长按判定时间
     * @param delay 判定时间（毫秒）
     */
    void setLongPressDelay(uint16_t delay) { _longPressDelay = delay; }

    /**
     * @brief 查询按键是否被按下
     * @param key 按键编号（1-22）
     * @return true表示按下，false表示未按下
     */
    bool isKeyPressed(uint8_t key) const;

    /**
     * @brief 查询按键是否处于长按状态
     * @param key 按键编号（1-22）
     * @return true表示长按，false表示未长按
     */
    bool isKeyLongPressed(uint8_t key) const;

    /**
     * @brief 获取当前按下的按键列表
     * @param keyBuffer 用于存储按键编号的缓冲区
     * @param bufferSize 缓冲区大小
     * @return 实际按下的按键数量
     */
    uint8_t getPressedKeys(uint8_t* keyBuffer, uint8_t bufferSize) const;

    /**
     * @brief 获取当前按键状态的原始值
     * @return 24位按键状态值
     */
    uint32_t getCurrentState() const { return _currentState; }

    /**
     * @brief 设置按键反馈配置
     * @param keyNumber 按键编号（1-22）
     * @param feedback 反馈配置
     */
    void setKeyFeedback(uint8_t keyNumber, const KeyFeedback& feedback);

    /**
     * @brief 更新LED效果
     */
    void updateLEDEffects();

    /**
     * @brief 设置全局LED亮度
     * @param brightness 亮度值（0-255）
     */
    void setGlobalBrightness(uint8_t brightness);

    /**
     * @brief 配置蜂鸣器
     * @param config 蜂鸣器配置
     */
    void configureBuzzer(const BuzzerConfig& config);

    /**
     * @brief 设置蜂鸣器音量
     * @param volume 音量等级
     */
    void setBuzzerVolume(BuzzerVolume volume);

    /**
     * @brief 设置蜂鸣器是否跟随按键
     * @param enable true启用，false禁用
     * @param dualTone true使用双音效，false使用单音效
     */
    void setBuzzerFollowKey(bool enable, bool dualTone = false);

    /**
     * @brief 设置蜂鸣器模式
     * @param mode 蜂鸣器模式
     */
    void setBuzzerMode(BuzzerMode mode);

    /**
     * @brief 启动蜂鸣器（用于测试）
     * @param freq 频率
     * @param duration 持续时间
     */
    void startBuzzer(uint16_t freq, uint16_t duration);

    #ifdef DEBUG_MODE
    /**
     * @brief 打印调试信息
     */
    void printDebugInfo();
    
    /**
     * @brief 测试所有24位状态
     */
    void testAllBits();
    #endif

private:
    // LED效果结构体
    struct LEDEffect {
        bool active;            ///< 效果是否激活
        uint32_t startTime;     ///< 效果开始时间
        uint8_t brightness;     ///< 当前亮度
        LEDMode mode;          ///< LED模式
        CRGB color;            ///< LED颜色
        uint16_t duration;      ///< 效果持续时间
    };

    // 按键状态结构体
    struct KeyState {
        bool pressed;           ///< 当前是否按下
        bool longPressed;       ///< 是否触发长按
        bool autoRepeat;        ///< 是否启用自动重复
        uint32_t pressTime;     ///< 按下时间
        uint32_t lastRepeat;    ///< 上次重复触发时间
    };

    // 常量定义
    static const uint8_t KEY_POSITIONS[22];  ///< 按键位置映射表
    static const uint16_t PIANO_TONES[22];   ///< 钢琴音阶频率表
    static const uint32_t DEBOUNCE_DELAY = 20;     ///< 去抖延迟(ms)
    static const uint32_t UPDATE_INTERVAL = 10;     ///< 更新间隔(ms)
    static const uint32_t DEFAULT_REPEAT_DELAY = 500;  ///< 默认重复延迟
    static const uint32_t DEFAULT_REPEAT_RATE = 100;   ///< 默认重复速率
    static const uint32_t DEFAULT_LONGPRESS_DELAY = 800; ///< 默认长按延迟

    // 成员变量
    uint32_t _currentState;     ///< 当前按键状态
    uint32_t _lastState;        ///< 上一次的按键状态
    uint32_t _debouncedState;   ///< 去抖后的状态
    uint32_t _lastDebounceTime; ///< 上次去抖时间
    uint32_t _lastUpdateTime;   ///< 上次更新时间
    
    KeyState _keyStates[22];    ///< 按键状态数组
    uint8_t _pressedKeys[22];   ///< 按下的按键数组
    uint8_t _pressedKeyCount;   ///< 按下的按键数量
    uint8_t _comboBuffer[5];    ///< 组合键缓冲区（最多5个键）
    
    KeyFeedback _keyFeedback[22]; ///< 每个按键的反馈配置
    LEDEffect _ledEffects[NUM_LEDS]; ///< LED效果数组
    
    KeyEventCallback _eventCallback; ///< 事件回调函数
    
    uint16_t _repeatDelay;      ///< 自动重复延迟
    uint16_t _repeatRate;       ///< 自动重复速率
    uint16_t _longPressDelay;   ///< 长按判定延迟
    uint8_t _globalBrightness;  ///< 全局LED亮度

    BuzzerConfig _buzzerConfig; ///< 蜂鸣器配置
    bool _buzzerActive;         ///< 蜂鸣器是否正在发声
    uint32_t _buzzerEndTime;    ///< 蜂鸣器结束时间

    // 内部函数
    /**
     * @brief 读取移位寄存器状态
     * @return 24位按键状态值
     */
    uint32_t readShiftRegisters();

    /**
     * @brief 检查按键状态变化
     * @param buttonState 当前按键状态
     */
    void checkKeyStates(uint32_t buttonState);

    /**
     * @brief 更新按键状态
     */
    void updateKeyStates();

    /**
     * @brief 处理按键事件
     * @param type 事件类型
     * @param key 按键编号
     */
    void handleKeyEvent(KeyEventType type, uint8_t key);

    /**
     * @brief 检查组合键
     */
    void checkComboKeys();

    /**
     * @brief 更新自动重复
     */
    void updateAutoRepeat();

    /**
     * @brief 处理LED效果
     * @param keyNumber 按键编号
     * @param mode LED模式
     * @param color LED颜色
     */
    void handleLEDEffect(uint8_t keyNumber, LEDMode mode, CRGB color);

    /**
     * @brief 获取音量对应的PWM占空比
     * @param volume 音量等级
     * @return PWM占空比值
     */
    uint8_t getVolumeDuty(BuzzerVolume volume) const;

    /**
     * @brief 更新蜂鸣器状态
     */
    void updateBuzzer();
};

#endif // KEYPAD_CONTROL_H