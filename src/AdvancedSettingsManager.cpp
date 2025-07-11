/**
 * @file AdvancedSettingsManager.cpp
 * @brief 高级设置管理系统实现
 * 
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "AdvancedSettingsManager.h"
#include <SPIFFS.h>
#include <algorithm>
#include <esp_crc.h>

// 静态常量定义
const char* AdvancedSettingsManager::PREF_NAMESPACE = "adv_settings";
const char* AdvancedSettingsManager::PREF_PROFILES_KEY = "profiles";
const char* AdvancedSettingsManager::PREF_BACKUPS_KEY = "backups";
const char* AdvancedSettingsManager::PREF_HISTORY_KEY = "history";
const char* AdvancedSettingsManager::PREF_CURRENT_PROFILE_KEY = "current_prof";

// 静态实例
AdvancedSettingsManager* AdvancedSettingsManager::_instance = nullptr;

// 设置键名常量定义
namespace SettingsUtils {
    namespace Keys {
        // 键盘设置
        const String KEYBOARD_LAYOUT = "keyboard.layout";
        const String KEYBOARD_REPEAT_DELAY = "keyboard.repeat_delay";
        const String KEYBOARD_REPEAT_RATE = "keyboard.repeat_rate";
        const String KEYBOARD_LONGPRESS_DELAY = "keyboard.longpress_delay";
        const String KEYBOARD_LAYER_SWITCH_TIMEOUT = "keyboard.layer_switch_timeout";
        
        // 显示设置
        const String DISPLAY_BRIGHTNESS = "display.brightness";
        const String DISPLAY_CONTRAST = "display.contrast";
        const String DISPLAY_ORIENTATION = "display.orientation";
        const String DISPLAY_SLEEP_TIMEOUT = "display.sleep_timeout";
        
        // 音频设置
        const String AUDIO_MASTER_VOLUME = "audio.master_volume";
        const String AUDIO_BUZZER_ENABLED = "audio.buzzer_enabled";
        const String AUDIO_BUZZER_VOLUME = "audio.buzzer_volume";
        const String AUDIO_KEY_SOUNDS = "audio.key_sounds";
        
        // 系统设置
        const String SYSTEM_AUTO_SAVE = "system.auto_save";
        const String SYSTEM_LOG_LEVEL = "system.log_level";
        const String SYSTEM_LANGUAGE = "system.language";
        const String SYSTEM_TIMEZONE = "system.timezone";
    }
}

AdvancedSettingsManager::AdvancedSettingsManager()
    : _configManager(ConfigManager::getInstance())
    , _keyboardConfig(keyboardConfig)
    , _initialized(false)
    , _hasUnsavedChanges(false)
    , _mutex(nullptr)
    , _nextListenerId(1) {
    
    // 创建互斥锁
    _mutex = xSemaphoreCreateMutex();
    if (!_mutex) {
        LOG_E("ADV_SETTINGS", "无法创建高级设置管理器互斥锁");
    }
}

AdvancedSettingsManager::~AdvancedSettingsManager() {
    if (_mutex) {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

AdvancedSettingsManager& AdvancedSettingsManager::getInstance() {
    if (_instance == nullptr) {
        _instance = new AdvancedSettingsManager();
    }
    return *_instance;
}

bool AdvancedSettingsManager::initialize() {
    if (_initialized) {
        return true;
    }
    
    LOG_I("ADV_SETTINGS", "初始化高级设置管理器...");
    
    lock();
    
    // 初始化SPIFFS文件系统（如果需要）
    if (!SPIFFS.begin(true)) {
        LOG_W("ADV_SETTINGS", "SPIFFS初始化失败，某些功能可能不可用");
    }
    
    // 打开Preferences
    if (!_preferences.begin(PREF_NAMESPACE, false)) {
        LOG_E("ADV_SETTINGS", "无法打开高级设置Preferences存储");
        unlock();
        return false;
    }
    
    // 加载默认设置定义
    loadDefaultSettings();
    
    // 加载用户配置文件
    loadUserProfiles();
    
    // 加载备份信息
    loadBackups();
    
    // 加载变更历史
    loadChangeHistory();
    
    // 如果没有当前配置文件，创建默认配置文件
    if (_currentProfileId.isEmpty() || _userProfiles.find(_currentProfileId) == _userProfiles.end()) {
        String defaultProfileId = createUserProfile("默认配置", "系统默认用户配置文件");
        if (!defaultProfileId.isEmpty()) {
            switchToProfile(defaultProfileId);
        }
    }
    
    _initialized = true;
    unlock();
    
    LOG_I("ADV_SETTINGS", "高级设置管理器初始化完成");
    return true;
}

void AdvancedSettingsManager::shutdown() {
    if (!_initialized) {
        return;
    }
    
    LOG_I("ADV_SETTINGS", "关闭高级设置管理器...");
    
    lock();
    
    // 保存所有未保存的变更
    if (_hasUnsavedChanges) {
        saveAllChanges();
    }
    
    // 清理监听器
    _changeListeners.clear();
    
    // 关闭Preferences
    _preferences.end();
    
    _initialized = false;
    unlock();
    
    LOG_I("ADV_SETTINGS", "高级设置管理器已关闭");
}

bool AdvancedSettingsManager::registerSetting(const SettingDefinition& definition) {
    if (!_initialized) {
        LOG_E("ADV_SETTINGS", "设置管理器未初始化");
        return false;
    }
    
    lock();
    
    // 检查设置键名是否已存在
    if (_settingDefinitions.find(definition.key) != _settingDefinitions.end()) {
        LOG_W("ADV_SETTINGS", "设置键名已存在: %s", definition.key.c_str());
        unlock();
        return false;
    }
    
    // 注册设置定义
    _settingDefinitions[definition.key] = definition;
    _settingsByCategory[definition.category].push_back(definition.key);
    
    unlock();
    
    LOG_D("ADV_SETTINGS", "已注册设置: %s", definition.key.c_str());
    return true;
}

size_t AdvancedSettingsManager::registerSettings(const std::vector<SettingDefinition>& definitions) {
    size_t successCount = 0;
    for (const auto& definition : definitions) {
        if (registerSetting(definition)) {
            successCount++;
        }
    }
    
    LOG_I("ADV_SETTINGS", "批量注册设置完成: %d/%d", successCount, definitions.size());
    return successCount;
}

const SettingDefinition* AdvancedSettingsManager::getSettingDefinition(const String& key) const {
    lock();
    auto it = _settingDefinitions.find(key);
    const SettingDefinition* result = (it != _settingDefinitions.end()) ? &it->second : nullptr;
    unlock();
    return result;
}

std::vector<const SettingDefinition*> AdvancedSettingsManager::getSettingsByCategory(SettingsCategory category) const {
    std::vector<const SettingDefinition*> result;
    
    lock();
    auto it = _settingsByCategory.find(category);
    if (it != _settingsByCategory.end()) {
        for (const String& key : it->second) {
            auto defIt = _settingDefinitions.find(key);
            if (defIt != _settingDefinitions.end()) {
                result.push_back(&defIt->second);
            }
        }
    }
    unlock();
    
    return result;
}

bool AdvancedSettingsManager::setSetting(const String& key, const String& value, bool validate, const String& source) {
    if (!_initialized) {
        LOG_E("ADV_SETTINGS", "设置管理器未初始化");
        return false;
    }
    
    lock();
    
    // 获取设置定义
    auto defIt = _settingDefinitions.find(key);
    if (defIt == _settingDefinitions.end()) {
        LOG_E("ADV_SETTINGS", "未知的设置键名: %s", key.c_str());
        unlock();
        return false;
    }
    
    const SettingDefinition& definition = defIt->second;
    
    // 验证设置值
    if (validate) {
        String errorMessage;
        if (!validateSetting(key, value, errorMessage)) {
            LOG_E("ADV_SETTINGS", "设置验证失败: %s - %s", key.c_str(), errorMessage.c_str());
            unlock();
            return false;
        }
    }
    
    // 获取当前值
    String oldValue = getSetting(key);
    
    // 如果值没有变化，直接返回
    if (oldValue == value) {
        unlock();
        return true;
    }
    
    // 更新当前配置文件中的设置值
    if (!_currentProfileId.isEmpty()) {
        auto profileIt = _userProfiles.find(_currentProfileId);
        if (profileIt != _userProfiles.end()) {
            profileIt->second.settings[key] = value;
            profileIt->second.modifiedTime = millis();
        }
    }
    
    // 记录变更
    recordSettingChange(key, oldValue, value, source);
    
    // 立即应用设置（如果不需要重启）
    if (!definition.requiresRestart) {
        applySettingChange(key, value);
    }
    
    // 触发变更回调
    if (definition.onChange) {
        definition.onChange(oldValue, value);
    }
    
    // 通知监听器
    notifySettingChange(key, oldValue, value);
    
    _hasUnsavedChanges = true;
    unlock();
    
    LOG_D("ADV_SETTINGS", "设置已更新: %s = %s", key.c_str(), value.c_str());
    return true;
}

String AdvancedSettingsManager::getSetting(const String& key, const String& defaultValue) const {
    if (!_initialized) {
        return defaultValue;
    }
    
    lock();
    
    // 首先从当前配置文件获取
    if (!_currentProfileId.isEmpty()) {
        auto profileIt = _userProfiles.find(_currentProfileId);
        if (profileIt != _userProfiles.end()) {
            auto settingIt = profileIt->second.settings.find(key);
            if (settingIt != profileIt->second.settings.end()) {
                String result = settingIt->second;
                unlock();
                return result;
            }
        }
    }
    
    // 从设置定义获取默认值
    auto defIt = _settingDefinitions.find(key);
    if (defIt != _settingDefinitions.end()) {
        const SettingDefinition& definition = defIt->second;
        
        // 如果有动态默认值生成器，使用它
        if (definition.dynamicDefault) {
            String result = definition.dynamicDefault();
            unlock();
            return result;
        }
        
        // 使用静态默认值
        if (!definition.defaultValue.isEmpty()) {
            String result = definition.defaultValue;
            unlock();
            return result;
        }
    }
    
    unlock();
    return defaultValue;
}

bool AdvancedSettingsManager::getBoolSetting(const String& key, bool defaultValue) const {
    String value = getSetting(key, defaultValue ? "true" : "false");
    return value.equalsIgnoreCase("true") || value.equals("1");
}

int AdvancedSettingsManager::getIntSetting(const String& key, int defaultValue) const {
    String value = getSetting(key, String(defaultValue));
    return value.toInt();
}

float AdvancedSettingsManager::getFloatSetting(const String& key, float defaultValue) const {
    String value = getSetting(key, String(defaultValue));
    return value.toFloat();
}

size_t AdvancedSettingsManager::setSettings(const std::map<String, String>& settings, bool validate, const String& source) {
    size_t successCount = 0;
    
    // 如果需要验证，先进行批量验证
    if (validate) {
        std::map<String, String> errors;
        if (!validateSettings(settings, errors)) {
            LOG_E("ADV_SETTINGS", "批量设置验证失败，错误数量: %d", errors.size());
            for (const auto& error : errors) {
                LOG_E("ADV_SETTINGS", "验证错误: %s - %s", error.first.c_str(), error.second.c_str());
            }
            return 0;
        }
    }
    
    // 批量设置值
    for (const auto& setting : settings) {
        if (setSetting(setting.first, setting.second, false, source)) {
            successCount++;
        }
    }
    
    LOG_I("ADV_SETTINGS", "批量设置完成: %d/%d", successCount, settings.size());
    return successCount;
}

bool AdvancedSettingsManager::validateSetting(const String& key, const String& value, String& errorMessage) const {
    lock();
    
    auto defIt = _settingDefinitions.find(key);
    if (defIt == _settingDefinitions.end()) {
        errorMessage = "未知的设置键名";
        unlock();
        return false;
    }
    
    const SettingDefinition& definition = defIt->second;
    const SettingValidation& validation = definition.validation;
    
    // 检查数据类型
    switch (validation.type) {
        case SettingType::BOOLEAN:
            if (!value.equalsIgnoreCase("true") && !value.equalsIgnoreCase("false") && 
                !value.equals("1") && !value.equals("0")) {
                errorMessage = "布尔值必须为 true/false 或 1/0";
                unlock();
                return false;
            }
            break;
            
        case SettingType::INTEGER: {
            int intValue = value.toInt();
            if (value != String(intValue)) {
                errorMessage = "必须为有效的整数";
                unlock();
                return false;
            }
            if (validation.minValue != validation.maxValue) {
                if (intValue < validation.minValue || intValue > validation.maxValue) {
                    errorMessage = "值必须在 " + String((int)validation.minValue) + " 到 " + String((int)validation.maxValue) + " 之间";
                    unlock();
                    return false;
                }
            }
            break;
        }
        
        case SettingType::FLOAT: {
            float floatValue = value.toFloat();
            if (validation.minValue != validation.maxValue) {
                if (floatValue < validation.minValue || floatValue > validation.maxValue) {
                    errorMessage = "值必须在 " + String(validation.minValue) + " 到 " + String(validation.maxValue) + " 之间";
                    unlock();
                    return false;
                }
            }
            break;
        }
        
        case SettingType::STRING:
            if (validation.maxLength > 0 && value.length() > validation.maxLength) {
                errorMessage = "字符串长度不能超过 " + String(validation.maxLength) + " 个字符";
                unlock();
                return false;
            }
            break;
            
        case SettingType::ENUM:
            if (!validation.allowedValues.empty()) {
                bool found = false;
                for (const String& allowedValue : validation.allowedValues) {
                    if (value.equals(allowedValue)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    errorMessage = "值必须为以下之一: ";
                    for (size_t i = 0; i < validation.allowedValues.size(); i++) {
                        if (i > 0) errorMessage += ", ";
                        errorMessage += validation.allowedValues[i];
                    }
                    unlock();
                    return false;
                }
            }
            break;
            
        default:
            break;
    }
    
    // 执行自定义验证
    if (validation.customValidator) {
        if (!validation.customValidator(value)) {
            errorMessage = validation.errorMessage.isEmpty() ? "自定义验证失败" : validation.errorMessage;
            unlock();
            return false;
        }
    }
    
    unlock();
    return true;
}

bool AdvancedSettingsManager::validateSettings(const std::map<String, String>& settings, std::map<String, String>& errors) const {
    errors.clear();
    
    for (const auto& setting : settings) {
        String errorMessage;
        if (!validateSetting(setting.first, setting.second, errorMessage)) {
            errors[setting.first] = errorMessage;
        }
    }
    
    return errors.empty();
}

String AdvancedSettingsManager::createUserProfile(const String& profileName, const String& description) {
    if (!_initialized) {
        LOG_E("ADV_SETTINGS", "设置管理器未初始化");
        return "";
    }
    
    lock();
    
    // 检查配置文件数量限制
    if (_userProfiles.size() >= MAX_PROFILES) {
        LOG_E("ADV_SETTINGS", "用户配置文件数量已达上限: %d", MAX_PROFILES);
        unlock();
        return "";
    }
    
    // 检查名称是否已存在
    for (const auto& profile : _userProfiles) {
        if (profile.second.profileName.equals(profileName)) {
            LOG_E("ADV_SETTINGS", "配置文件名称已存在: %s", profileName.c_str());
            unlock();
            return "";
        }
    }
    
    // 创建新的配置文件
    UserProfile newProfile;
    newProfile.profileId = generateUniqueId();
    newProfile.profileName = profileName;
    newProfile.description = description;
    newProfile.createdTime = millis();
    newProfile.modifiedTime = newProfile.createdTime;
    newProfile.version = "1.0";
    newProfile.isActive = false;
    newProfile.isReadOnly = false;
    
    // 复制当前设置作为初始值
    if (!_currentProfileId.isEmpty()) {
        auto currentProfileIt = _userProfiles.find(_currentProfileId);
        if (currentProfileIt != _userProfiles.end()) {
            newProfile.settings = currentProfileIt->second.settings;
        }
    }
    
    _userProfiles[newProfile.profileId] = newProfile;
    _hasUnsavedChanges = true;
    
    String profileId = newProfile.profileId;
    unlock();
    
    LOG_I("ADV_SETTINGS", "已创建用户配置文件: %s (%s)", profileName.c_str(), profileId.c_str());
    return profileId;
}

bool AdvancedSettingsManager::switchToProfile(const String& profileId) {
    if (!_initialized) {
        LOG_E("ADV_SETTINGS", "设置管理器未初始化");
        return false;
    }
    
    lock();
    
    auto profileIt = _userProfiles.find(profileId);
    if (profileIt == _userProfiles.end()) {
        LOG_E("ADV_SETTINGS", "未找到配置文件: %s", profileId.c_str());
        unlock();
        return false;
    }
    
    // 取消当前活动配置文件
    if (!_currentProfileId.isEmpty()) {
        auto currentProfileIt = _userProfiles.find(_currentProfileId);
        if (currentProfileIt != _userProfiles.end()) {
            currentProfileIt->second.isActive = false;
        }
    }
    
    // 设置新的活动配置文件
    profileIt->second.isActive = true;
    _currentProfileId = profileId;
    _hasUnsavedChanges = true;
    
    unlock();
    
    // 应用配置文件中的所有设置
    applySettingChanges(profileIt->second.settings);
    
    LOG_I("ADV_SETTINGS", "已切换到配置文件: %s", profileIt->second.profileName.c_str());
    return true;
}

const UserProfile* AdvancedSettingsManager::getCurrentProfile() const {
    if (!_initialized || _currentProfileId.isEmpty()) {
        return nullptr;
    }
    
    lock();
    auto it = _userProfiles.find(_currentProfileId);
    const UserProfile* result = (it != _userProfiles.end()) ? &it->second : nullptr;
    unlock();
    return result;
}

bool AdvancedSettingsManager::applySettingChange(const String& key, const String& value) {
    // 根据设置类别应用不同的逻辑
    const SettingDefinition* definition = getSettingDefinition(key);
    if (!definition) {
        return false;
    }
    
    switch (definition->category) {
        case SettingsCategory::KEYBOARD:
            return applyKeyboardSetting(key, value);
        case SettingsCategory::DISPLAY_SETTINGS:
            return applyDisplaySetting(key, value);
        case SettingsCategory::AUDIO:
            return applyAudioSetting(key, value);
        case SettingsCategory::SYSTEM:
            return applySystemSetting(key, value);
        default:
            LOG_D("ADV_SETTINGS", "设置应用: %s = %s (无特殊处理)", key.c_str(), value.c_str());
            return true;
    }
}

size_t AdvancedSettingsManager::applySettingChanges(const std::map<String, String>& settings) {
    size_t successCount = 0;
    for (const auto& setting : settings) {
        if (applySettingChange(setting.first, setting.second)) {
            successCount++;
        }
    }
    return successCount;
}

bool AdvancedSettingsManager::saveAllChanges() {
    if (!_initialized) {
        return false;
    }
    
    lock();
    
    bool success = true;
    
    // 保存用户配置文件
    saveUserProfiles();
    
    // 保存备份信息
    saveBackups();
    
    // 保存变更历史
    saveChangeHistory();
    
    // 保存当前配置文件ID
    _preferences.putString(PREF_CURRENT_PROFILE_KEY, _currentProfileId);
    
    _hasUnsavedChanges = false;
    unlock();
    
    LOG_I("ADV_SETTINGS", "所有变更已保存");
    return success;
}

void AdvancedSettingsManager::loadDefaultSettings() {
    LOG_D("ADV_SETTINGS", "加载默认设置定义...");
    
    std::vector<SettingDefinition> defaultSettings;
    
    // 键盘设置
    {
        SettingDefinition def;
        def.key = SettingsUtils::Keys::KEYBOARD_REPEAT_DELAY;
        def.name = "按键重复延迟";
        def.description = "按键开始重复前的延迟时间";
        def.category = SettingsCategory::KEYBOARD;
        def.validation.type = SettingType::INTEGER;
        def.validation.minValue = 100;
        def.validation.maxValue = 2000;
        def.defaultValue = "500";
        def.onChange = [this](const String& oldValue, const String& newValue) {
            _configManager.setRepeatDelay(newValue.toInt());
        };
        defaultSettings.push_back(def);
    }
    
    {
        SettingDefinition def;
        def.key = SettingsUtils::Keys::KEYBOARD_REPEAT_RATE;
        def.name = "按键重复速率";
        def.description = "按键重复的间隔时间";
        def.category = SettingsCategory::KEYBOARD;
        def.validation.type = SettingType::INTEGER;
        def.validation.minValue = 50;
        def.validation.maxValue = 500;
        def.defaultValue = "100";
        def.onChange = [this](const String& oldValue, const String& newValue) {
            _configManager.setRepeatRate(newValue.toInt());
        };
        defaultSettings.push_back(def);
    }
    
    // 显示设置
    {
        SettingDefinition def;
        def.key = SettingsUtils::Keys::DISPLAY_BRIGHTNESS;
        def.name = "显示亮度";
        def.description = "屏幕背光亮度";
        def.category = SettingsCategory::DISPLAY_SETTINGS;
        def.validation.type = SettingType::INTEGER;
        def.validation.minValue = 0;
        def.validation.maxValue = 255;
        def.defaultValue = "100";
        def.onChange = [this](const String& oldValue, const String& newValue) {
            _configManager.setBacklightBrightness(newValue.toInt());
        };
        defaultSettings.push_back(def);
    }
    
    // 音频设置
    {
        SettingDefinition def;
        def.key = SettingsUtils::Keys::AUDIO_BUZZER_ENABLED;
        def.name = "蜂鸣器启用";
        def.description = "是否启用按键蜂鸣器";
        def.category = SettingsCategory::AUDIO;
        def.validation.type = SettingType::BOOLEAN;
        def.defaultValue = "true";
        def.onChange = [this](const String& oldValue, const String& newValue) {
            _configManager.setBuzzerEnabled(getBoolSetting(SettingsUtils::Keys::AUDIO_BUZZER_ENABLED));
        };
        defaultSettings.push_back(def);
    }
    
    {
        SettingDefinition def;
        def.key = SettingsUtils::Keys::AUDIO_BUZZER_VOLUME;
        def.name = "蜂鸣器音量";
        def.description = "蜂鸣器音量等级";
        def.category = SettingsCategory::AUDIO;
        def.validation.type = SettingType::ENUM;
        def.validation.allowedValues = {"0", "1", "2", "3"};
        def.defaultValue = "2";
        def.onChange = [this](const String& oldValue, const String& newValue) {
            _configManager.setBuzzerVolume(newValue.toInt());
        };
        defaultSettings.push_back(def);
    }
    
    // 系统设置
    {
        SettingDefinition def;
        def.key = SettingsUtils::Keys::SYSTEM_AUTO_SAVE;
        def.name = "自动保存";
        def.description = "是否自动保存设置变更";
        def.category = SettingsCategory::SYSTEM;
        def.validation.type = SettingType::BOOLEAN;
        def.defaultValue = "true";
        def.onChange = [this](const String& oldValue, const String& newValue) {
            _configManager.setAutoSave(getBoolSetting(SettingsUtils::Keys::SYSTEM_AUTO_SAVE));
        };
        defaultSettings.push_back(def);
    }
    
    // 批量注册设置
    registerSettings(defaultSettings);
    
    LOG_I("ADV_SETTINGS", "已加载 %d 个默认设置定义", defaultSettings.size());
}

void AdvancedSettingsManager::loadUserProfiles() {
    // 实现用户配置文件加载逻辑
    size_t dataSize = _preferences.getBytesLength(PREF_PROFILES_KEY);
    if (dataSize == 0) {
        LOG_I("ADV_SETTINGS", "未找到用户配置文件数据");
        return;
    }
    
    // 简化实现：暂时跳过复杂的序列化，在实际项目中可以使用JSON
    LOG_D("ADV_SETTINGS", "用户配置文件加载功能待实现");
    
    // 加载当前配置文件ID
    _currentProfileId = _preferences.getString(PREF_CURRENT_PROFILE_KEY, "");
}

void AdvancedSettingsManager::saveUserProfiles() {
    // 实现用户配置文件保存逻辑
    LOG_D("ADV_SETTINGS", "用户配置文件保存功能待实现");
}

void AdvancedSettingsManager::recordSettingChange(const String& key, const String& oldValue, const String& newValue, const String& source) {
    SettingChangeRecord record;
    record.settingKey = key;
    record.oldValue = oldValue;
    record.newValue = newValue;
    record.timestamp = millis();
    record.reason = "用户修改";
    record.source = source;
    
    _changeHistory.push_back(record);
    
    // 限制历史记录数量
    if (_changeHistory.size() > MAX_CHANGE_HISTORY) {
        _changeHistory.erase(_changeHistory.begin());
    }
    
    // 更新使用统计
    _usageStatistics[key]++;
}

void AdvancedSettingsManager::notifySettingChange(const String& key, const String& oldValue, const String& newValue) {
    for (const auto& listener : _changeListeners) {
        if (listener.second.first.isEmpty() || listener.second.first.equals(key)) {
            listener.second.second(key, oldValue, newValue);
        }
    }
}

String AdvancedSettingsManager::generateUniqueId() const {
    // 生成基于时间戳和随机数的唯一ID
    uint32_t timestamp = millis();
    uint32_t random = esp_random();
    return String(timestamp, HEX) + String(random, HEX);
}

uint32_t AdvancedSettingsManager::calculateChecksum(const String& data) const {
    // 简单的CRC32校验和计算
    return 0; // 实际实现中应该使用真正的CRC32算法
}

void AdvancedSettingsManager::lock() const {
    if (_mutex) {
        xSemaphoreTake(_mutex, portMAX_DELAY);
    }
}

void AdvancedSettingsManager::unlock() const {
    if (_mutex) {
        xSemaphoreGive(_mutex);
    }
}

bool AdvancedSettingsManager::tryLock(uint32_t timeoutMs) const {
    if (_mutex) {
        return xSemaphoreTake(_mutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
    }
    return false;
}

// 设置应用辅助方法实现
bool AdvancedSettingsManager::applyKeyboardSetting(const String& key, const String& value) {
    if (key == SettingsUtils::Keys::KEYBOARD_REPEAT_DELAY) {
        _configManager.setRepeatDelay(value.toInt());
        return true;
    } else if (key == SettingsUtils::Keys::KEYBOARD_REPEAT_RATE) {
        _configManager.setRepeatRate(value.toInt());
        return true;
    } else if (key == SettingsUtils::Keys::KEYBOARD_LONGPRESS_DELAY) {
        _configManager.setLongPressDelay(value.toInt());
        return true;
    }
    return false;
}

bool AdvancedSettingsManager::applyDisplaySetting(const String& key, const String& value) {
    if (key == SettingsUtils::Keys::DISPLAY_BRIGHTNESS) {
        _configManager.setBacklightBrightness(value.toInt());
        return true;
    } else if (key == SettingsUtils::Keys::DISPLAY_SLEEP_TIMEOUT) {
        _configManager.setSleepTimeout(value.toInt());
        return true;
    }
    return false;
}

bool AdvancedSettingsManager::applyAudioSetting(const String& key, const String& value) {
    if (key == SettingsUtils::Keys::AUDIO_BUZZER_ENABLED) {
        _configManager.setBuzzerEnabled(value.equalsIgnoreCase("true") || value.equals("1"));
        return true;
    } else if (key == SettingsUtils::Keys::AUDIO_BUZZER_VOLUME) {
        _configManager.setBuzzerVolume(value.toInt());
        return true;
    }
    return false;
}

bool AdvancedSettingsManager::applySystemSetting(const String& key, const String& value) {
    if (key == SettingsUtils::Keys::SYSTEM_AUTO_SAVE) {
        _configManager.setAutoSave(value.equalsIgnoreCase("true") || value.equals("1"));
        return true;
    } else if (key == SettingsUtils::Keys::SYSTEM_LOG_LEVEL) {
        _configManager.setLogLevel(value.toInt());
        return true;
    }
    return false;
}

// 占位符实现 - 在完整实现中需要展开
void AdvancedSettingsManager::loadBackups() {
    LOG_D("ADV_SETTINGS", "备份加载功能待实现");
}

void AdvancedSettingsManager::saveBackups() {
    LOG_D("ADV_SETTINGS", "备份保存功能待实现");
}

void AdvancedSettingsManager::loadChangeHistory() {
    LOG_D("ADV_SETTINGS", "变更历史加载功能待实现");
}

void AdvancedSettingsManager::saveChangeHistory() {
    LOG_D("ADV_SETTINGS", "变更历史保存功能待实现");
}

// 工具函数实现
namespace SettingsUtils {
    String categoryToString(SettingsCategory category) {
        switch (category) {
            case SettingsCategory::KEYBOARD: return "keyboard";
            case SettingsCategory::DISPLAY_SETTINGS: return "display";
            case SettingsCategory::AUDIO: return "audio";
            case SettingsCategory::SYSTEM: return "system";
            case SettingsCategory::USER_INTERFACE: return "ui";
            case SettingsCategory::PERFORMANCE: return "performance";
            case SettingsCategory::ACCESSIBILITY: return "accessibility";
            case SettingsCategory::EXPERIMENTAL: return "experimental";
            default: return "unknown";
        }
    }
    
    SettingsCategory stringToCategory(const String& str) {
        if (str == "keyboard") return SettingsCategory::KEYBOARD;
        if (str == "display") return SettingsCategory::DISPLAY_SETTINGS;
        if (str == "audio") return SettingsCategory::AUDIO;
        if (str == "system") return SettingsCategory::SYSTEM;
        if (str == "ui") return SettingsCategory::USER_INTERFACE;
        if (str == "performance") return SettingsCategory::PERFORMANCE;
        if (str == "accessibility") return SettingsCategory::ACCESSIBILITY;
        if (str == "experimental") return SettingsCategory::EXPERIMENTAL;
        return SettingsCategory::SYSTEM;
    }
}