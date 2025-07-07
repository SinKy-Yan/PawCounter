#include "ConfigManager.h"

// 单例实例
ConfigManager* ConfigManager::_instance = nullptr;

ConfigManager& ConfigManager::getInstance() {
    if (_instance == nullptr) {
        _instance = new ConfigManager();
    }
    return *_instance;
}

bool ConfigManager::begin() {
    if (_initialized) {
        return true;
    }
    
    LOG_I(TAG_CONFIG, "初始化配置管理器...");
    
    // 打开Preferences
    if (!_preferences.begin(CONFIG_NAMESPACE, false)) {
        LOG_E(TAG_CONFIG, "无法打开Preferences存储");
        return false;
    }
    
    // 加载配置
    if (!load()) {
        LOG_W(TAG_CONFIG, "配置加载失败，使用默认配置");
        loadDefaults();
    }
    
    _initialized = true;
    LOG_I(TAG_CONFIG, "配置管理器初始化完成");
    return true;
}

bool ConfigManager::load() {
    if (!_preferences.isKey(KEY_LED_BRIGHTNESS)) {
        LOG_I(TAG_CONFIG, "未找到保存的配置，使用默认配置");
        return false;
    }
    
    LOG_I(TAG_CONFIG, "正在加载配置...");
    
    // 加载LED配置
    _config.globalBrightness = _preferences.getUChar(KEY_LED_BRIGHTNESS, 255);
    _config.ledFadeDuration = _preferences.getUShort(KEY_LED_FADE_DUR, 500);
    
    // 加载蜂鸣器配置
    _config.buzzerEnabled = _preferences.getBool(KEY_BUZZER_EN, true);
    _config.buzzerFollowKeypress = _preferences.getBool(KEY_BUZZER_FOLLOW, true);
    _config.buzzerDualTone = _preferences.getBool(KEY_BUZZER_DUAL, false);
    _config.buzzerMode = _preferences.getUChar(KEY_BUZZER_MODE, 0);
    _config.buzzerVolume = _preferences.getUChar(KEY_BUZZER_VOL, 2);
    _config.buzzerPressFreq = _preferences.getUShort(KEY_BUZZER_PRESS_FREQ, 2000);
    _config.buzzerReleaseFreq = _preferences.getUShort(KEY_BUZZER_REL_FREQ, 1500);
    _config.buzzerDuration = _preferences.getUShort(KEY_BUZZER_DUR, 50);
    
    // 加载按键设置
    _config.repeatDelay = _preferences.getUShort(KEY_REPEAT_DELAY, 500);
    _config.repeatRate = _preferences.getUShort(KEY_REPEAT_RATE, 100);
    _config.longPressDelay = _preferences.getUShort(KEY_LONGPRESS_DELAY, 1000);
    
    // 加载背光设置
    _config.backlightBrightness = _preferences.getUChar(KEY_BACKLIGHT_BRIGHT, 100);
    
    // 加载休眠设置
    _config.sleepTimeout = _preferences.getULong(KEY_SLEEP_TIMEOUT, 10000);
    
    // 加载系统设置
    _config.autoSave = _preferences.getBool(KEY_AUTO_SAVE, true);
    _config.logEnabled = _preferences.getBool(KEY_LOG_EN, true);
    _config.logLevel = _preferences.getUChar(KEY_LOG_LEVEL, 3);
    
    _dirty = false;
    LOG_I(TAG_CONFIG, "配置加载完成");
    return true;
}

bool ConfigManager::save() {
    if (!_initialized) {
        LOG_E(TAG_CONFIG, "配置管理器未初始化");
        return false;
    }
    
    LOG_I(TAG_CONFIG, "正在保存配置...");
    
    // 保存LED配置
    _preferences.putUChar(KEY_LED_BRIGHTNESS, _config.globalBrightness);
    _preferences.putUShort(KEY_LED_FADE_DUR, _config.ledFadeDuration);
    
    // 保存蜂鸣器配置
    _preferences.putBool(KEY_BUZZER_EN, _config.buzzerEnabled);
    _preferences.putBool(KEY_BUZZER_FOLLOW, _config.buzzerFollowKeypress);
    _preferences.putBool(KEY_BUZZER_DUAL, _config.buzzerDualTone);
    _preferences.putUChar(KEY_BUZZER_MODE, _config.buzzerMode);
    _preferences.putUChar(KEY_BUZZER_VOL, _config.buzzerVolume);
    _preferences.putUShort(KEY_BUZZER_PRESS_FREQ, _config.buzzerPressFreq);
    _preferences.putUShort(KEY_BUZZER_REL_FREQ, _config.buzzerReleaseFreq);
    _preferences.putUShort(KEY_BUZZER_DUR, _config.buzzerDuration);
    
    // 保存按键设置
    _preferences.putUShort(KEY_REPEAT_DELAY, _config.repeatDelay);
    _preferences.putUShort(KEY_REPEAT_RATE, _config.repeatRate);
    _preferences.putUShort(KEY_LONGPRESS_DELAY, _config.longPressDelay);
    
    // 保存背光设置
    _preferences.putUChar(KEY_BACKLIGHT_BRIGHT, _config.backlightBrightness);
    
    // 保存休眠设置
    _preferences.putULong(KEY_SLEEP_TIMEOUT, _config.sleepTimeout);
    
    // 保存系统设置
    _preferences.putBool(KEY_AUTO_SAVE, _config.autoSave);
    _preferences.putBool(KEY_LOG_EN, _config.logEnabled);
    _preferences.putUChar(KEY_LOG_LEVEL, _config.logLevel);
    
    _dirty = false;
    LOG_I(TAG_CONFIG, "配置保存完成");
    return true;
}

bool ConfigManager::saveIfDirty() {
    if (_dirty && _config.autoSave) {
        return save();
    }
    return true;
}

void ConfigManager::reset() {
    LOG_I(TAG_CONFIG, "重置配置为默认值");
    loadDefaults();
    _dirty = true;
    if (_config.autoSave) {
        save();
    }
}

void ConfigManager::loadDefaults() {
    _config = PersistentConfig();  // 使用默认构造函数
    _dirty = true;
    LOG_I(TAG_CONFIG, "已加载默认配置");
}

void ConfigManager::markDirty() {
    _dirty = true;
}

// LED配置设置方法
void ConfigManager::setLEDBrightness(uint8_t brightness) {
    if (_config.globalBrightness != brightness) {
        _config.globalBrightness = brightness;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::setLEDFadeDuration(uint16_t duration) {
    if (_config.ledFadeDuration != duration) {
        _config.ledFadeDuration = duration;
        markDirty();
        if (_config.autoSave) save();
    }
}

// 蜂鸣器配置设置方法
void ConfigManager::setBuzzerEnabled(bool enabled) {
    if (_config.buzzerEnabled != enabled) {
        _config.buzzerEnabled = enabled;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::setBuzzerFollowKeypress(bool follow) {
    if (_config.buzzerFollowKeypress != follow) {
        _config.buzzerFollowKeypress = follow;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::setBuzzerDualTone(bool dual) {
    if (_config.buzzerDualTone != dual) {
        _config.buzzerDualTone = dual;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::setBuzzerMode(uint8_t mode) {
    if (_config.buzzerMode != mode) {
        _config.buzzerMode = mode;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::setBuzzerVolume(uint8_t volume) {
    if (_config.buzzerVolume != volume) {
        _config.buzzerVolume = volume;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::setBuzzerPressFreq(uint16_t freq) {
    if (_config.buzzerPressFreq != freq) {
        _config.buzzerPressFreq = freq;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::setBuzzerReleaseFreq(uint16_t freq) {
    if (_config.buzzerReleaseFreq != freq) {
        _config.buzzerReleaseFreq = freq;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::setBuzzerDuration(uint16_t duration) {
    if (_config.buzzerDuration != duration) {
        _config.buzzerDuration = duration;
        markDirty();
        if (_config.autoSave) save();
    }
}

// 按键设置方法
void ConfigManager::setRepeatDelay(uint16_t delay) {
    if (_config.repeatDelay != delay) {
        _config.repeatDelay = delay;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::setRepeatRate(uint16_t rate) {
    if (_config.repeatRate != rate) {
        _config.repeatRate = rate;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::setLongPressDelay(uint16_t delay) {
    if (_config.longPressDelay != delay) {
        _config.longPressDelay = delay;
        markDirty();
        if (_config.autoSave) save();
    }
}

// 背光设置方法
void ConfigManager::setBacklightBrightness(uint8_t brightness) {
    if (_config.backlightBrightness != brightness) {
        _config.backlightBrightness = brightness;
        markDirty();
        if (_config.autoSave) save();
    }
}

// 休眠设置方法
void ConfigManager::setSleepTimeout(uint32_t timeout) {
    if (_config.sleepTimeout != timeout) {
        _config.sleepTimeout = timeout;
        markDirty();
        if (_config.autoSave) save();
    }
}

// 系统设置方法
void ConfigManager::setAutoSave(bool autoSave) {
    if (_config.autoSave != autoSave) {
        _config.autoSave = autoSave;
        markDirty();
        // 注意：这里不能使用自动保存，因为可能正在关闭自动保存
        save();
    }
}

void ConfigManager::setLogEnabled(bool enabled) {
    if (_config.logEnabled != enabled) {
        _config.logEnabled = enabled;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::setLogLevel(uint8_t level) {
    if (_config.logLevel != level) {
        _config.logLevel = level;
        markDirty();
        if (_config.autoSave) save();
    }
}

void ConfigManager::printConfig() const {
    Serial.println("=== 当前配置 ===");
    Serial.printf("LED亮度: %d\n", _config.globalBrightness);
    Serial.printf("LED渐变时间: %d ms\n", _config.ledFadeDuration);
    Serial.printf("蜂鸣器启用: %s\n", _config.buzzerEnabled ? "是" : "否");
    Serial.printf("蜂鸣器跟随按键: %s\n", _config.buzzerFollowKeypress ? "是" : "否");
    Serial.printf("蜂鸣器双音调: %s\n", _config.buzzerDualTone ? "是" : "否");
    Serial.printf("蜂鸣器模式: %s\n", _config.buzzerMode ? "钢琴" : "普通");
    Serial.printf("蜂鸣器音量: %d\n", _config.buzzerVolume);
    Serial.printf("蜂鸣器按下频率: %d Hz\n", _config.buzzerPressFreq);
    Serial.printf("蜂鸣器释放频率: %d Hz\n", _config.buzzerReleaseFreq);
    Serial.printf("蜂鸣器持续时间: %d ms\n", _config.buzzerDuration);
    Serial.printf("按键重复延迟: %d ms\n", _config.repeatDelay);
    Serial.printf("按键重复速率: %d ms\n", _config.repeatRate);
    Serial.printf("长按延迟: %d ms\n", _config.longPressDelay);
    Serial.printf("背光亮度: %d\n", _config.backlightBrightness);
    Serial.printf("休眠超时: %lu ms\n", _config.sleepTimeout);
    Serial.printf("自动保存: %s\n", _config.autoSave ? "是" : "否");
    Serial.printf("日志启用: %s\n", _config.logEnabled ? "是" : "否");
    Serial.printf("日志级别: %d\n", _config.logLevel);
    Serial.printf("配置状态: %s\n", _dirty ? "已修改" : "未修改");
    Serial.println("===============");
}

size_t ConfigManager::getConfigSize() const {
    return sizeof(PersistentConfig);
}

bool ConfigManager::clearAll() {
    if (!_initialized) {
        LOG_E(TAG_CONFIG, "配置管理器未初始化");
        return false;
    }
    
    LOG_I(TAG_CONFIG, "清除所有配置");
    bool result = _preferences.clear();
    if (result) {
        loadDefaults();
        LOG_I(TAG_CONFIG, "配置已清除并重置为默认值");
    } else {
        LOG_E(TAG_CONFIG, "配置清除失败");
    }
    return result;
}