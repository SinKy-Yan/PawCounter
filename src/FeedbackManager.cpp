/**
 * @file FeedbackManager.cpp
 * @brief 硬件反馈管理系统实现文件
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "FeedbackManager.h"

FeedbackManager& FeedbackManager::getInstance() {
    static FeedbackManager instance;
    return instance;
}

bool FeedbackManager::begin() {
    LOG_I(TAG_SYSTEM, "Initializing Feedback Manager...");
    
    _initialized = false;
    _lastActivityTime = millis();
    _energySaveActive = false;
    _longPressActive = false;
    _longPressLED = 0;
    _longPressStartTime = 0;

    // 初始化子系统
    if (!LEDEffectManager::getInstance().begin()) {
        LOG_E(TAG_SYSTEM, "Failed to initialize LED Effect Manager");
        return false;
    }

    if (!BuzzerSoundManager::getInstance().begin()) {
        LOG_E(TAG_SYSTEM, "Failed to initialize Buzzer Sound Manager");
        return false;
    }

    // 初始化默认场景配置
    initializeDefaultScenes();

    // 初始化用户偏好为默认值
    _userPrefs = UserPreferences();

    // 应用用户偏好到子系统
    applyUserPreferences();

    // 初始化统计信息
    _stats = FeedbackStats();

    _initialized = true;
    LOG_I(TAG_SYSTEM, "Feedback Manager initialized successfully");
    
    // 不在这里播放启动反馈，由main.cpp在所有初始化完成后调用
    
    return true;
}

void FeedbackManager::update() {
    if (!_initialized) return;

    // 更新子系统
    LEDEffectManager::getInstance().update();
    BuzzerSoundManager::getInstance().update();

    // 检查长按状态：闪烁完成后切换到持续亮起
    if (_longPressActive && (millis() - _longPressStartTime) >= 400) {
        // 闪烁完成，设置为持续亮起
        LEDEffectConfig config;
        config.type = LED_EFFECT_SOLID;
        config.primaryColor = CRGB::White;
        config.brightness = 255;
        config.duration = 0;  // 持续亮起
        config.speed = 1;
        config.priority = LED_PRIORITY_NORMAL;
        
        LEDEffectManager::getInstance().setLEDEffect(_longPressLED, config);
        _longPressActive = false;  // 状态处理完成
    }

    // 检查节能模式
    checkEnergySaveMode();

    // 更新统计信息
    _stats.lastActivityTime = _lastActivityTime;
}

void FeedbackManager::triggerFeedback(FeedbackScene scene, int8_t targetLED, 
                                     const FeedbackConfig* customConfig) {
    if (!_initialized) return;

    // 获取场景配置
    FeedbackConfig config;
    if (customConfig) {
        config = *customConfig;
    } else {
        config = getSceneConfig(scene);
    }

    // 应用用户偏好调整
    config = adjustConfigForPreferences(config, scene);

    // 检查是否需要跳过反馈
    if (shouldSkipFeedback(scene, config)) {
        return;
    }

    // 播放反馈
    playFeedback(config, targetLED);

    // 更新统计信息
    updateActivity();
    _stats.totalFeedbacks++;

    LOG_D(TAG_SYSTEM, "Triggered feedback: scene=%d, LED=%d", scene, targetLED);
}

uint8_t FeedbackManager::keyToLED(uint8_t keyIndex) const {
    // 按键编号(1-22)到LED索引(0-21)的映射
    // 基于config.h中的KEY_MATRIX定义
    if (keyIndex == 0 || keyIndex > 22) return 255; // 无效索引
    
    const uint8_t KEY_TO_LED_MAP[] = {
        0,  // Key 1 → LED 0
        1,  // Key 2 → LED 1  
        2,  // Key 3 → LED 2
        3,  // Key 4 → LED 3
        4,  // Key 5 → LED 4
        5,  // Key 6 → LED 5
        6,  // Key 7 → LED 6
        7,  // Key 8 → LED 7
        8,  // Key 9 → LED 8
        9,  // Key 10 → LED 9
        10, // Key 11 → LED 10
        11, // Key 12 → LED 11
        12, // Key 13 → LED 12
        13, // Key 14 → LED 13
        14, // Key 15 → LED 14
        15, // Key 16 → LED 15
        16, // Key 17 → LED 16
        17, // Key 18 → LED 17
        18, // Key 19 → LED 18
        19, // Key 20 → LED 19
        20, // Key 21 → LED 20
        21  // Key 22 → LED 21
    };
    
    return KEY_TO_LED_MAP[keyIndex - 1]; // keyIndex是1-based，数组是0-based
}

void FeedbackManager::triggerKeyFeedback(uint8_t keyIndex, bool pressed, bool longPress) {
    if (!_initialized || keyIndex == 0 || keyIndex > 22) return;
    
    // 转换按键编号到LED索引
    uint8_t ledIndex = keyToLED(keyIndex);
    if (ledIndex == 255) return;
    
    LOG_D(TAG_SYSTEM, "Key %d mapped to LED %d", keyIndex, ledIndex);

    if (longPress) {
        // 长按特殊处理：先闪烁两下，然后持续亮起
        triggerFeedback(SCENE_KEY_LONG_PRESS, ledIndex);
        
        // 延迟后切换到持续亮起状态（在闪烁完成后）
        // 这里我们使用一个定时器来在400ms后设置为持续亮起
        _longPressLED = ledIndex;
        _longPressStartTime = millis();
        _longPressActive = true;
        
    } else if (pressed) {
        triggerFeedback(SCENE_KEY_PRESS, ledIndex);
    } else {
        triggerFeedback(SCENE_KEY_RELEASE, ledIndex);
        
        // 如果这个LED正在长按状态，也需要清除长按状态
        if (_longPressActive && _longPressLED == ledIndex) {
            _longPressActive = false;
        }
    }
}

void FeedbackManager::triggerCalculationFeedback(bool success, uint8_t errorType) {
    FeedbackScene scene = success ? SCENE_CALCULATION_SUCCESS : SCENE_CALCULATION_ERROR;
    triggerFeedback(scene);
}

void FeedbackManager::triggerSystemFeedback(FeedbackScene scene) {
    LOG_I(TAG_SYSTEM, "Triggering system feedback: scene=%d", scene);
    triggerFeedback(scene);
}

void FeedbackManager::triggerBatteryFeedback(uint8_t batteryLevel, bool charging) {
    FeedbackScene scene;
    
    if (charging) {
        scene = SCENE_BATTERY_CHARGING;
    } else if (batteryLevel <= 10) {
        scene = SCENE_BATTERY_LOW;
    } else if (batteryLevel >= 95) {
        scene = SCENE_BATTERY_FULL;
    } else {
        return; // 普通电量不需要反馈
    }

    triggerFeedback(scene);
}

void FeedbackManager::playLEDEffect(const uint8_t* ledIndices, uint8_t count, 
                                   const LEDEffectConfig& config) {
    if (!_initialized || !_userPrefs.visualEffects) return;

    for (uint8_t i = 0; i < count; i++) {
        if (ledIndices[i] < NUM_LEDS) {
            LEDEffectManager::getInstance().setLEDEffect(ledIndices[i], config);
        }
    }

    _stats.ledActivations++;
}

void FeedbackManager::playSound(const SoundEffectConfig& config, bool interrupt) {
    if (!_initialized || !_userPrefs.systemSounds) return;

    BuzzerSoundManager::getInstance().playSound(config, interrupt);
    _stats.soundActivations++;
}

void FeedbackManager::playFeedback(const FeedbackConfig& config, int8_t targetLED) {
    // 添加延迟
    if (config.delay > 0) {
        delay(config.delay);
    }

    // 播放LED效果
    if (config.ledEnabled && _userPrefs.visualEffects) {
        if (targetLED >= 0 && targetLED < NUM_LEDS) {
            // 单个LED
            LOG_D(TAG_SYSTEM, "Setting single LED %d effect", targetLED);
            LEDEffectManager::getInstance().setLEDEffect(targetLED, config.ledConfig);
        } else {
            // 全局效果
            LOG_I(TAG_SYSTEM, "Setting global LED effect: type=%d, color=(%d,%d,%d)", 
                  config.ledConfig.type, config.ledConfig.primaryColor.r, 
                  config.ledConfig.primaryColor.g, config.ledConfig.primaryColor.b);
            LEDEffectManager::getInstance().setGlobalEffect(config.ledConfig);
        }
        _stats.ledActivations++;
    } else {
        LOG_W(TAG_SYSTEM, "LED effect skipped: enabled=%d, visualEffects=%d", 
              config.ledEnabled, _userPrefs.visualEffects);
    }

    // 播放音效
    if (config.soundEnabled && (_userPrefs.systemSounds || _userPrefs.keyClickSound)) {
        bool interrupt = config.soundConfig.priority >= SOUND_PRIORITY_HIGH;
        BuzzerSoundManager::getInstance().playSound(config.soundConfig, interrupt);
        _stats.soundActivations++;
    }

    // 计算能耗
    _stats.totalEnergyUsed += calculateEnergyUsage(config);
}

void FeedbackManager::setSceneConfig(FeedbackScene scene, const FeedbackConfig& config) {
    uint8_t index = getSceneIndex(scene);
    if (index < MAX_SCENES) {
        _sceneConfigs[index] = config;
        LOG_D(TAG_SYSTEM, "Scene %d config updated", scene);
    }
}

const FeedbackConfig& FeedbackManager::getSceneConfig(FeedbackScene scene) const {
    uint8_t index = getSceneIndex(scene);
    if (index < MAX_SCENES) {
        return _sceneConfigs[index];
    }
    
    // 返回默认配置
    static FeedbackConfig defaultConfig;
    return defaultConfig;
}

void FeedbackManager::setUserPreferences(const UserPreferences& preferences) {
    _userPrefs = preferences;
    applyUserPreferences();
    LOG_I(TAG_SYSTEM, "User preferences updated");
}

void FeedbackManager::setFeedbackMode(FeedbackMode mode) {
    _userPrefs.mode = mode;
    applyUserPreferences();
    LOG_I(TAG_SYSTEM, "Feedback mode set to: %d", mode);
}

void FeedbackManager::setGlobalIntensity(FeedbackIntensity intensity) {
    _userPrefs.intensity = intensity;
    applyUserPreferences();
    LOG_I(TAG_SYSTEM, "Global intensity set to: %d", intensity);
}

void FeedbackManager::setKeyClickEnabled(bool enabled) {
    _userPrefs.keyClickSound = enabled;
    BuzzerSoundManager::getInstance().setKeyClickEnabled(enabled);
    LOG_I(TAG_SYSTEM, "Key click sound %s", enabled ? "enabled" : "disabled");
}

void FeedbackManager::setSystemSoundsEnabled(bool enabled) {
    _userPrefs.systemSounds = enabled;
    BuzzerSoundManager::getInstance().setEnabled(enabled);
    LOG_I(TAG_SYSTEM, "System sounds %s", enabled ? "enabled" : "disabled");
}

void FeedbackManager::setVisualEffectsEnabled(bool enabled) {
    _userPrefs.visualEffects = enabled;
    LEDEffectManager::getInstance().setEnabled(enabled);
    LOG_I(TAG_SYSTEM, "Visual effects %s", enabled ? "enabled" : "disabled");
}

void FeedbackManager::setEnergySaveMode(bool enabled) {
    _userPrefs.energySaveMode = enabled;
    if (enabled) {
        _energySaveActive = false; // 重置状态
        checkEnergySaveMode();
    }
    LOG_I(TAG_SYSTEM, "Energy save mode %s", enabled ? "enabled" : "disabled");
}

void FeedbackManager::resetStats() {
    _stats = FeedbackStats();
    LOG_I(TAG_SYSTEM, "Feedback statistics reset");
}

bool FeedbackManager::isSilentMode() const {
    return _userPrefs.mode == MODE_SILENT || _userPrefs.mode == MODE_VISUAL_ONLY;
}

uint32_t FeedbackManager::getEstimatedBatteryLife(uint8_t currentBatteryLevel) const {
    if (currentBatteryLevel == 0 || _stats.totalFeedbacks == 0) return 0;

    // 简单的估算：基于当前使用模式
    uint32_t avgEnergyPerHour = (_stats.totalEnergyUsed * 3600) / 
                               ((millis() - _stats.lastActivityTime) / 1000 + 1);
    
    // 假设电池容量和当前消耗的关系
    uint32_t estimatedHours = (currentBatteryLevel * 100) / (avgEnergyPerHour + 1);
    
    return estimatedHours * 60; // 转换为分钟
}

bool FeedbackManager::saveConfiguration() {
    // TODO: 实现配置保存到EEPROM或Flash
    LOG_I(TAG_SYSTEM, "Configuration saved");
    return true;
}

bool FeedbackManager::loadConfiguration() {
    // TODO: 实现从EEPROM或Flash加载配置
    LOG_I(TAG_SYSTEM, "Configuration loaded");
    return true;
}

void FeedbackManager::restoreDefaults() {
    _userPrefs = UserPreferences();
    initializeDefaultScenes();
    applyUserPreferences();
    LOG_I(TAG_SYSTEM, "Configuration restored to defaults");
}

String FeedbackManager::exportConfiguration() const {
    // TODO: 实现JSON导出
    return "{}";
}

bool FeedbackManager::importConfiguration(const String& jsonConfig) {
    // TODO: 实现JSON导入
    LOG_I(TAG_SYSTEM, "Configuration imported");
    return true;
}

// === 私有方法实现 ===

void FeedbackManager::initializeDefaultScenes() {
    // 按键按下 - 持续亮起直到释放
    FeedbackConfig& keyPress = _sceneConfigs[getSceneIndex(SCENE_KEY_PRESS)];
    keyPress.ledConfig.type = LED_EFFECT_SOLID;
    keyPress.ledConfig.primaryColor = CRGB::White;
    keyPress.ledConfig.brightness = 255;
    keyPress.ledConfig.duration = 0;    // 持续亮起，直到手动停止
    keyPress.ledConfig.speed = 1;       // 最快速度，无延迟
    keyPress.soundConfig.type = SOUND_CLICK;
    keyPress.soundConfig.volume = 30;
    keyPress.synchronized = true;

    // 按键释放 - 即时熄灭
    FeedbackConfig& keyRelease = _sceneConfigs[getSceneIndex(SCENE_KEY_RELEASE)];
    keyRelease.ledConfig.type = LED_EFFECT_OFF;
    keyRelease.ledConfig.primaryColor = CRGB::Black;
    keyRelease.ledConfig.brightness = 0;
    keyRelease.ledConfig.duration = 1;   // 立即生效
    keyRelease.ledConfig.speed = 1;      // 最快速度
    keyRelease.soundEnabled = false;     // 释放通常不播放音效

    // 长按 - 闪烁两下然后持续亮起
    FeedbackConfig& longPress = _sceneConfigs[getSceneIndex(SCENE_KEY_LONG_PRESS)];
    longPress.ledConfig.type = LED_EFFECT_BLINK;
    longPress.ledConfig.primaryColor = CRGB::White;
    longPress.ledConfig.brightness = 255;
    longPress.ledConfig.duration = 400;   // 闪烁两下的时间
    longPress.ledConfig.speed = 100;      // 100ms闪烁间隔（2次闪烁=400ms）
    longPress.ledConfig.repeat = false;
    longPress.ledConfig.repeatCount = 2;  // 闪烁两下
    longPress.soundConfig.type = SOUND_DOUBLE_BEEP;
    longPress.soundConfig.volume = 40;

    // 计算成功
    FeedbackConfig& success = _sceneConfigs[getSceneIndex(SCENE_CALCULATION_SUCCESS)];
    success.ledConfig.type = LED_EFFECT_PULSE;
    success.ledConfig.primaryColor = CRGB::Green;
    success.ledConfig.brightness = 150;
    success.ledConfig.duration = 600;
    success.soundConfig.type = SOUND_SUCCESS;
    success.soundConfig.volume = 50;

    // 计算错误
    FeedbackConfig& error = _sceneConfigs[getSceneIndex(SCENE_CALCULATION_ERROR)];
    error.ledConfig.type = LED_EFFECT_BLINK;
    error.ledConfig.primaryColor = CRGB::Red;
    error.ledConfig.brightness = 200;
    error.ledConfig.duration = 800;
    error.soundConfig.type = SOUND_ERROR;
    error.soundConfig.volume = 60;

    // 系统启动 - 使用和LEDEffectManager一致的白色闪烁
    FeedbackConfig& startup = _sceneConfigs[getSceneIndex(SCENE_SYSTEM_STARTUP)];
    startup.ledConfig.type = LED_EFFECT_BLINK;
    startup.ledConfig.primaryColor = CRGB::White;
    startup.ledConfig.brightness = 255;
    startup.ledConfig.duration = 1000;  // 总持续1秒
    startup.ledConfig.speed = 500;      // 0.5秒亮，0.5秒灭
    startup.ledConfig.repeat = false;
    startup.ledConfig.repeatCount = 1;  // 闪烁一次
    startup.soundConfig.type = SOUND_STARTUP;
    startup.soundConfig.volume = 40;

    // 低电量
    FeedbackConfig& lowBattery = _sceneConfigs[getSceneIndex(SCENE_BATTERY_LOW)];
    lowBattery.ledConfig.type = LED_EFFECT_BREATH;
    lowBattery.ledConfig.primaryColor = CRGB::Red;
    lowBattery.ledConfig.brightness = 100;
    lowBattery.ledConfig.duration = 0; // 持续
    lowBattery.soundConfig.type = SOUND_WARNING;
    lowBattery.soundConfig.volume = 70;
    lowBattery.soundConfig.repeat = true;
    lowBattery.soundConfig.repeatCount = 3;

    LOG_D(TAG_SYSTEM, "Default scene configurations initialized");
}

void FeedbackManager::applyUserPreferences() {
    // 应用音量设置
    BuzzerSoundManager::getInstance().setGlobalVolume(_userPrefs.globalVolume);
    BuzzerSoundManager::getInstance().setKeyClickEnabled(_userPrefs.keyClickSound);
    BuzzerSoundManager::getInstance().setEnabled(_userPrefs.systemSounds);

    // 应用亮度设置
    LEDEffectManager::getInstance().setGlobalBrightness(_userPrefs.globalBrightness);
    LEDEffectManager::getInstance().setEnabled(_userPrefs.visualEffects);

    // 应用反馈模式
    switch (_userPrefs.mode) {
        case MODE_SILENT:
            BuzzerSoundManager::getInstance().setEnabled(false);
            LEDEffectManager::getInstance().setEnabled(false);
            break;
        case MODE_VIBRATION_ONLY:
            BuzzerSoundManager::getInstance().setEnabled(true);
            LEDEffectManager::getInstance().setEnabled(false);
            break;
        case MODE_VISUAL_ONLY:
            BuzzerSoundManager::getInstance().setEnabled(false);
            LEDEffectManager::getInstance().setEnabled(true);
            break;
        case MODE_FULL:
        case MODE_ADAPTIVE:
            BuzzerSoundManager::getInstance().setEnabled(true);
            LEDEffectManager::getInstance().setEnabled(true);
            break;
    }

    LOG_D(TAG_SYSTEM, "User preferences applied");
}

FeedbackConfig FeedbackManager::adjustConfigForPreferences(const FeedbackConfig& baseConfig, 
                                                          FeedbackScene scene) const {
    FeedbackConfig adjusted = baseConfig;

    // 应用全局强度调整
    float intensityMultiplier = 1.0;
    switch (_userPrefs.intensity) {
        case INTENSITY_OFF:
            intensityMultiplier = 0.0;
            break;
        case INTENSITY_SUBTLE:
            intensityMultiplier = 0.5;
            break;
        case INTENSITY_NORMAL:
            intensityMultiplier = 1.0;
            break;
        case INTENSITY_STRONG:
            intensityMultiplier = 1.5;
            break;
    }

    // 调整LED亮度
    adjusted.ledConfig.brightness = (uint8_t)(adjusted.ledConfig.brightness * intensityMultiplier);
    adjusted.ledConfig.brightness = constrain(adjusted.ledConfig.brightness, 0, 255);

    // 调整音效音量
    adjusted.soundConfig.volume = (uint8_t)(adjusted.soundConfig.volume * intensityMultiplier);
    adjusted.soundConfig.volume = constrain(adjusted.soundConfig.volume, 0, 100);

    // 节能模式调整
    if (_userPrefs.energySaveMode && _energySaveActive) {
        adjusted.ledConfig.brightness /= 2;
        adjusted.soundConfig.volume /= 2;
    }

    return adjusted;
}

bool FeedbackManager::shouldSkipFeedback(FeedbackScene scene, const FeedbackConfig& config) const {
    // 静音模式检查
    if (_userPrefs.mode == MODE_SILENT) {
        return true;
    }

    // 节能模式检查
    if (_userPrefs.energySaveMode && _energySaveActive) {
        // 在节能模式下只允许关键反馈
        return config.ledConfig.priority < LED_PRIORITY_HIGH && 
               config.soundConfig.priority < SOUND_PRIORITY_HIGH;
    }

    // 强度检查
    if (_userPrefs.intensity == INTENSITY_OFF) {
        return true;
    }

    return false;
}

void FeedbackManager::updateActivity() {
    _lastActivityTime = millis();
    if (_energySaveActive) {
        _energySaveActive = false;
        LOG_D(TAG_SYSTEM, "Energy save mode deactivated");
    }
}

void FeedbackManager::checkEnergySaveMode() {
    if (!_userPrefs.energySaveMode) return;

    uint32_t inactiveTime = (millis() - _lastActivityTime) / 1000;
    
    if (!_energySaveActive && inactiveTime >= _userPrefs.inactiveTimeout) {
        _energySaveActive = true;
        LOG_I(TAG_SYSTEM, "Energy save mode activated after %lu seconds", inactiveTime);
    }
}

uint8_t FeedbackManager::getSceneIndex(FeedbackScene scene) const {
    return (uint8_t)scene % MAX_SCENES;
}

uint32_t FeedbackManager::calculateEnergyUsage(const FeedbackConfig& config) const {
    uint32_t energy = 0;

    // LED能耗估算
    if (config.ledEnabled) {
        energy += (config.ledConfig.brightness * config.ledConfig.duration) / 1000;
    }

    // 音效能耗估算
    if (config.soundEnabled) {
        energy += (config.soundConfig.volume * 100) / 1000; // 简化估算
    }

    return energy;
}

bool FeedbackManager::validateConfig(const FeedbackConfig& config) const {
    // 验证LED配置
    if (config.ledConfig.brightness > 255) return false;
    if (config.ledConfig.duration > 60000) return false; // 最大60秒

    // 验证音效配置
    if (config.soundConfig.volume > 100) return false;

    return true;
}