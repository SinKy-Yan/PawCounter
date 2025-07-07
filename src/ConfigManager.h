#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "Logger.h"

// Preferences命名空间
#define CONFIG_NAMESPACE "pawcounter"

// 存储键名定义
#define KEY_LED_BRIGHTNESS "led_bright"
#define KEY_LED_FADE_DUR "led_fade"
#define KEY_BUZZER_EN "buzz_en"
#define KEY_BUZZER_FOLLOW "buzz_follow"
#define KEY_BUZZER_DUAL "buzz_dual"
#define KEY_BUZZER_MODE "buzz_mode"
#define KEY_BUZZER_VOL "buzz_vol"
#define KEY_BUZZER_PRESS_FREQ "buzz_press_f"
#define KEY_BUZZER_REL_FREQ "buzz_rel_f"
#define KEY_BUZZER_DUR "buzz_dur"
#define KEY_REPEAT_DELAY "repeat_del"
#define KEY_REPEAT_RATE "repeat_rate"
#define KEY_LONGPRESS_DELAY "long_del"
#define KEY_BACKLIGHT_BRIGHT "bl_bright"
#define KEY_SLEEP_TIMEOUT "sleep_to"
#define KEY_AUTO_SAVE "auto_save"
#define KEY_LOG_EN "log_en"
#define KEY_LOG_LEVEL "log_lvl"

// 可持久化配置结构体
struct PersistentConfig {
    // LED配置
    uint8_t globalBrightness = 255;
    uint16_t ledFadeDuration = 500;
    
    // 蜂鸣器配置
    bool buzzerEnabled = true;
    bool buzzerFollowKeypress = true;
    bool buzzerDualTone = false;
    uint8_t buzzerMode = 0;  // 0:普通, 1:钢琴
    uint8_t buzzerVolume = 2;  // 0:静音, 1:低, 2:中, 3:高
    uint16_t buzzerPressFreq = 2000;
    uint16_t buzzerReleaseFreq = 1500;
    uint16_t buzzerDuration = 50;
    
    // 按键设置
    uint16_t repeatDelay = 500;
    uint16_t repeatRate = 100;
    uint16_t longPressDelay = 1000;
    
    // 背光设置
    uint8_t backlightBrightness = 100;
    
    // 休眠设置
    uint32_t sleepTimeout = 10000;  // 毫秒
    
    // 用户偏好标志
    bool autoSave = true;
    bool logEnabled = true;
    uint8_t logLevel = 3;  // INFO级别
};

class ConfigManager {
private:
    static ConfigManager* _instance;
    Preferences _preferences;
    PersistentConfig _config;
    bool _initialized = false;
    bool _dirty = false;
    
    // 私有构造函数（单例模式）
    ConfigManager() = default;
    
    // 内部方法
    void loadDefaults();
    void markDirty();

public:
    // 获取单例实例
    static ConfigManager& getInstance();
    
    // 初始化配置管理器
    bool begin();
    
    // 配置加载和保存
    bool load();
    bool save();
    bool saveIfDirty();
    
    // 配置重置
    void reset();
    
    // 获取配置值
    const PersistentConfig& getConfig() const { return _config; }
    
    // LED配置
    uint8_t getLEDBrightness() const { return _config.globalBrightness; }
    void setLEDBrightness(uint8_t brightness);
    
    uint16_t getLEDFadeDuration() const { return _config.ledFadeDuration; }
    void setLEDFadeDuration(uint16_t duration);
    
    // 蜂鸣器配置
    bool getBuzzerEnabled() const { return _config.buzzerEnabled; }
    void setBuzzerEnabled(bool enabled);
    
    bool getBuzzerFollowKeypress() const { return _config.buzzerFollowKeypress; }
    void setBuzzerFollowKeypress(bool follow);
    
    bool getBuzzerDualTone() const { return _config.buzzerDualTone; }
    void setBuzzerDualTone(bool dual);
    
    uint8_t getBuzzerMode() const { return _config.buzzerMode; }
    void setBuzzerMode(uint8_t mode);
    
    uint8_t getBuzzerVolume() const { return _config.buzzerVolume; }
    void setBuzzerVolume(uint8_t volume);
    
    uint16_t getBuzzerPressFreq() const { return _config.buzzerPressFreq; }
    void setBuzzerPressFreq(uint16_t freq);
    
    uint16_t getBuzzerReleaseFreq() const { return _config.buzzerReleaseFreq; }
    void setBuzzerReleaseFreq(uint16_t freq);
    
    uint16_t getBuzzerDuration() const { return _config.buzzerDuration; }
    void setBuzzerDuration(uint16_t duration);
    
    // 按键设置
    uint16_t getRepeatDelay() const { return _config.repeatDelay; }
    void setRepeatDelay(uint16_t delay);
    
    uint16_t getRepeatRate() const { return _config.repeatRate; }
    void setRepeatRate(uint16_t rate);
    
    uint16_t getLongPressDelay() const { return _config.longPressDelay; }
    void setLongPressDelay(uint16_t delay);
    
    // 背光设置
    uint8_t getBacklightBrightness() const { return _config.backlightBrightness; }
    void setBacklightBrightness(uint8_t brightness);
    
    // 休眠设置
    uint32_t getSleepTimeout() const { return _config.sleepTimeout; }
    void setSleepTimeout(uint32_t timeout);
    
    // 系统设置
    bool getAutoSave() const { return _config.autoSave; }
    void setAutoSave(bool autoSave);
    
    bool getLogEnabled() const { return _config.logEnabled; }
    void setLogEnabled(bool enabled);
    
    uint8_t getLogLevel() const { return _config.logLevel; }
    void setLogLevel(uint8_t level);
    
    // 状态查询
    bool isInitialized() const { return _initialized; }
    bool isDirty() const { return _dirty; }
    
    // 调试功能
    void printConfig() const;
    size_t getConfigSize() const;
    
    // 清除所有配置（恢复出厂设置）
    bool clearAll();
};

// 日志标签
#define TAG_CONFIG "CONFIG"

#endif // CONFIG_MANAGER_H