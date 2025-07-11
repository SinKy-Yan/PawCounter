/**
 * @file AdvancedSettingsManager.h
 * @brief 高级设置管理系统 - 扩展现有ConfigManager功能
 * @details 基于现有ConfigManager和KeyboardConfig系统，提供更强大的设置管理功能
 * 
 * 核心功能：
 * - 用户配置文件管理（多用户支持）
 * - 设置分组和层次化管理
 * - 配置导入导出（JSON格式）
 * - 实时设置验证和应用
 * - 配置版本管理和迁移
 * - 键盘自定义功能增强
 * - 设置备份和恢复
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 2.0
 */

#ifndef ADVANCED_SETTINGS_MANAGER_H
#define ADVANCED_SETTINGS_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <vector>
#include <map>
#include <functional>
#include "ConfigManager.h"
#include "KeyboardConfig.h"
#include "Logger.h"

// 前向声明
class AdvancedSettingsManager;

/**
 * @brief 设置类别枚举
 */
enum class SettingsCategory {
    KEYBOARD = 0,        ///< 键盘设置
    DISPLAY_SETTINGS,    ///< 显示设置
    AUDIO,               ///< 音频设置
    SYSTEM,              ///< 系统设置
    USER_INTERFACE,      ///< 用户界面设置
    PERFORMANCE,         ///< 性能设置
    ACCESSIBILITY,       ///< 无障碍设置
    EXPERIMENTAL,        ///< 实验性设置
    MAX_CATEGORIES
};

/**
 * @brief 设置数据类型枚举
 */
enum class SettingType {
    BOOLEAN = 0,         ///< 布尔值
    INTEGER,             ///< 整数
    FLOAT,               ///< 浮点数
    STRING,              ///< 字符串
    COLOR,               ///< 颜色值
    KEYMAP,              ///< 按键映射
    RANGE,               ///< 范围值
    ENUM,                ///< 枚举值
    ARRAY,               ///< 数组
    OBJECT               ///< 对象
};

/**
 * @brief 设置验证规则
 */
struct SettingValidation {
    SettingType type;           ///< 数据类型
    float minValue = 0;         ///< 最小值（数值类型）
    float maxValue = 0;         ///< 最大值（数值类型）
    uint16_t maxLength = 0;     ///< 最大长度（字符串类型）
    std::vector<String> allowedValues; ///< 允许的值列表（枚举类型）
    std::function<bool(const String&)> customValidator; ///< 自定义验证函数
    String errorMessage;        ///< 验证失败错误信息
};

/**
 * @brief 设置项定义
 */
struct SettingDefinition {
    String key;                    ///< 设置键名
    String name;                   ///< 显示名称
    String description;            ///< 设置描述
    SettingsCategory category;     ///< 设置类别
    SettingValidation validation;  ///< 验证规则
    String defaultValue;           ///< 默认值
    bool requiresRestart = false;  ///< 是否需要重启生效
    bool isAdvanced = false;       ///< 是否为高级设置
    bool isExperimental = false;   ///< 是否为实验性设置
    uint16_t priority = 100;       ///< 显示优先级（数值越小优先级越高）
    
    // 回调函数
    std::function<void(const String&, const String&)> onChange; ///< 值变化回调
    std::function<String()> dynamicDefault; ///< 动态默认值生成器
};

/**
 * @brief 用户配置文件
 */
struct UserProfile {
    String profileId;              ///< 配置文件ID
    String profileName;            ///< 配置文件名称
    String description;            ///< 描述
    uint32_t createdTime;          ///< 创建时间
    uint32_t modifiedTime;         ///< 修改时间
    String version;                ///< 版本号
    std::map<String, String> settings; ///< 设置值映射
    bool isActive = false;         ///< 是否为当前活动配置
    bool isReadOnly = false;       ///< 是否只读
};

/**
 * @brief 设置备份信息
 */
struct SettingsBackup {
    String backupId;               ///< 备份ID
    String backupName;             ///< 备份名称
    uint32_t timestamp;            ///< 备份时间戳
    String description;            ///< 备份描述
    size_t dataSize;               ///< 数据大小
    uint32_t checksum;             ///< 数据校验和
    bool isAutoBackup = false;     ///< 是否为自动备份
};

/**
 * @brief 设置变更记录
 */
struct SettingChangeRecord {
    String settingKey;             ///< 设置键名
    String oldValue;               ///< 旧值
    String newValue;               ///< 新值
    uint32_t timestamp;            ///< 变更时间戳
    String reason;                 ///< 变更原因
    String source;                 ///< 变更源（用户/系统/导入等）
};

/**
 * @brief 设置导入导出选项
 */
struct ImportExportOptions {
    bool includeUserProfiles = true;     ///< 包含用户配置文件
    bool includeKeyboardLayouts = true;  ///< 包含键盘布局
    bool includeSystemSettings = true;   ///< 包含系统设置
    bool includeExperimental = false;    ///< 包含实验性设置
    bool compressData = true;             ///< 压缩数据
    bool encryptData = false;             ///< 加密数据
    String encryptionKey;                 ///< 加密密钥
    bool verifyIntegrity = true;          ///< 验证数据完整性
};

/**
 * @brief 高级设置管理器类
 */
class AdvancedSettingsManager {
public:
    /**
     * @brief 构造函数
     */
    AdvancedSettingsManager();
    
    /**
     * @brief 析构函数
     */
    ~AdvancedSettingsManager();
    
    /**
     * @brief 获取单例实例
     */
    static AdvancedSettingsManager& getInstance();
    
    /**
     * @brief 初始化设置管理器
     * @return true 成功，false 失败
     */
    bool initialize();
    
    /**
     * @brief 关闭设置管理器
     */
    void shutdown();
    
    // =================== 设置定义和注册 ===================
    
    /**
     * @brief 注册设置项定义
     * @param definition 设置项定义
     * @return true 成功，false 失败
     */
    bool registerSetting(const SettingDefinition& definition);
    
    /**
     * @brief 批量注册设置项
     * @param definitions 设置项定义列表
     * @return 成功注册的数量
     */
    size_t registerSettings(const std::vector<SettingDefinition>& definitions);
    
    /**
     * @brief 获取设置项定义
     * @param key 设置键名
     * @return 设置项定义指针，未找到返回nullptr
     */
    const SettingDefinition* getSettingDefinition(const String& key) const;
    
    /**
     * @brief 获取指定类别的所有设置项
     * @param category 设置类别
     * @return 设置项定义列表
     */
    std::vector<const SettingDefinition*> getSettingsByCategory(SettingsCategory category) const;
    
    // =================== 设置值管理 ===================
    
    /**
     * @brief 设置值
     * @param key 设置键名
     * @param value 设置值
     * @param validate 是否验证，默认true
     * @param source 变更源，默认"user"
     * @return true 成功，false 失败
     */
    bool setSetting(const String& key, const String& value, bool validate = true, const String& source = "user");
    
    /**
     * @brief 获取设置值
     * @param key 设置键名
     * @param defaultValue 默认值
     * @return 设置值
     */
    String getSetting(const String& key, const String& defaultValue = "") const;
    
    /**
     * @brief 获取布尔设置值
     * @param key 设置键名
     * @param defaultValue 默认值
     * @return 布尔值
     */
    bool getBoolSetting(const String& key, bool defaultValue = false) const;
    
    /**
     * @brief 获取整数设置值
     * @param key 设置键名
     * @param defaultValue 默认值
     * @return 整数值
     */
    int getIntSetting(const String& key, int defaultValue = 0) const;
    
    /**
     * @brief 获取浮点数设置值
     * @param key 设置键名
     * @param defaultValue 默认值
     * @return 浮点数值
     */
    float getFloatSetting(const String& key, float defaultValue = 0.0f) const;
    
    /**
     * @brief 批量设置值
     * @param settings 设置键值对映射
     * @param validate 是否验证
     * @param source 变更源
     * @return 成功设置的数量
     */
    size_t setSettings(const std::map<String, String>& settings, bool validate = true, const String& source = "user");
    
    /**
     * @brief 重置设置为默认值
     * @param key 设置键名
     * @return true 成功，false 失败
     */
    bool resetSetting(const String& key);
    
    /**
     * @brief 重置指定类别的所有设置
     * @param category 设置类别
     * @return 重置的设置数量
     */
    size_t resetCategorySettings(SettingsCategory category);
    
    /**
     * @brief 重置所有设置为默认值
     * @return 重置的设置数量
     */
    size_t resetAllSettings();
    
    // =================== 设置验证 ===================
    
    /**
     * @brief 验证设置值
     * @param key 设置键名
     * @param value 设置值
     * @param errorMessage 错误信息输出
     * @return true 验证通过，false 验证失败
     */
    bool validateSetting(const String& key, const String& value, String& errorMessage) const;
    
    /**
     * @brief 批量验证设置
     * @param settings 设置映射
     * @param errors 错误信息映射输出
     * @return true 全部验证通过，false 存在验证失败
     */
    bool validateSettings(const std::map<String, String>& settings, std::map<String, String>& errors) const;
    
    // =================== 用户配置文件管理 ===================
    
    /**
     * @brief 创建用户配置文件
     * @param profileName 配置文件名称
     * @param description 描述
     * @return 配置文件ID，失败返回空字符串
     */
    String createUserProfile(const String& profileName, const String& description = "");
    
    /**
     * @brief 删除用户配置文件
     * @param profileId 配置文件ID
     * @return true 成功，false 失败
     */
    bool deleteUserProfile(const String& profileId);
    
    /**
     * @brief 切换到指定配置文件
     * @param profileId 配置文件ID
     * @return true 成功，false 失败
     */
    bool switchToProfile(const String& profileId);
    
    /**
     * @brief 获取当前活动配置文件
     * @return 配置文件指针，未找到返回nullptr
     */
    const UserProfile* getCurrentProfile() const;
    
    /**
     * @brief 获取所有用户配置文件
     * @return 配置文件列表
     */
    std::vector<UserProfile> getAllProfiles() const;
    
    /**
     * @brief 复制配置文件
     * @param sourceProfileId 源配置文件ID
     * @param newProfileName 新配置文件名称
     * @return 新配置文件ID，失败返回空字符串
     */
    String cloneProfile(const String& sourceProfileId, const String& newProfileName);
    
    /**
     * @brief 重命名配置文件
     * @param profileId 配置文件ID
     * @param newName 新名称
     * @return true 成功，false 失败
     */
    bool renameProfile(const String& profileId, const String& newName);
    
    // =================== 配置导入导出 ===================
    
    /**
     * @brief 导出设置到JSON
     * @param options 导出选项
     * @return JSON字符串，失败返回空字符串
     */
    String exportToJSON(const ImportExportOptions& options = ImportExportOptions()) const;
    
    /**
     * @brief 从JSON导入设置
     * @param jsonData JSON数据
     * @param options 导入选项
     * @param errors 错误信息输出
     * @return true 成功，false 失败
     */
    bool importFromJSON(const String& jsonData, const ImportExportOptions& options = ImportExportOptions());
    bool importFromJSON(const String& jsonData, const ImportExportOptions& options, std::vector<String>& errors);
    
    /**
     * @brief 导出设置到文件
     * @param filename 文件名
     * @param options 导出选项
     * @return true 成功，false 失败
     */
    bool exportToFile(const String& filename, const ImportExportOptions& options = ImportExportOptions()) const;
    
    /**
     * @brief 从文件导入设置
     * @param filename 文件名
     * @param options 导入选项
     * @param errors 错误信息输出
     * @return true 成功，false 失败
     */
    bool importFromFile(const String& filename, const ImportExportOptions& options = ImportExportOptions());
    bool importFromFile(const String& filename, const ImportExportOptions& options, std::vector<String>& errors);
    
    // =================== 设置备份和恢复 ===================
    
    /**
     * @brief 创建设置备份
     * @param backupName 备份名称
     * @param description 备份描述
     * @param isAutoBackup 是否为自动备份
     * @return 备份ID，失败返回空字符串
     */
    String createBackup(const String& backupName, const String& description = "", bool isAutoBackup = false);
    
    /**
     * @brief 恢复设置备份
     * @param backupId 备份ID
     * @return true 成功，false 失败
     */
    bool restoreBackup(const String& backupId);
    
    /**
     * @brief 删除设置备份
     * @param backupId 备份ID
     * @return true 成功，false 失败
     */
    bool deleteBackup(const String& backupId);
    
    /**
     * @brief 获取所有备份列表
     * @return 备份信息列表
     */
    std::vector<SettingsBackup> getAllBackups() const;
    
    /**
     * @brief 自动清理旧备份
     * @param maxBackups 最大保留备份数量
     * @param maxAgeDays 最大保留天数
     * @return 清理的备份数量
     */
    size_t cleanupOldBackups(size_t maxBackups = 10, uint32_t maxAgeDays = 30);
    
    // =================== 键盘自定义增强 ===================
    
    /**
     * @brief 创建自定义键盘布局
     * @param layoutName 布局名称
     * @param baseLayout 基础布局ID，空字符串表示从当前布局创建
     * @return 布局ID，失败返回空字符串
     */
    String createCustomKeyboardLayout(const String& layoutName, const String& baseLayout = "");
    
    /**
     * @brief 设置按键自定义功能
     * @param layoutId 布局ID
     * @param keyPosition 按键位置
     * @param keyFunction 按键功能定义
     * @param layer 键盘层级
     * @return true 成功，false 失败
     */
    bool setCustomKeyFunction(const String& layoutId, uint8_t keyPosition, const String& keyFunction, KeyLayer layer = KeyLayer::PRIMARY);
    
    /**
     * @brief 设置按键LED效果
     * @param layoutId 布局ID
     * @param keyPosition 按键位置
     * @param color 颜色值
     * @param effect 效果类型
     * @return true 成功，false 失败
     */
    bool setKeyLEDEffect(const String& layoutId, uint8_t keyPosition, uint32_t color, const String& effect);
    
    /**
     * @brief 获取自定义键盘布局列表
     * @return 布局信息列表
     */
    std::vector<String> getCustomKeyboardLayouts() const;
    
    /**
     * @brief 应用自定义键盘布局
     * @param layoutId 布局ID
     * @return true 成功，false 失败
     */
    bool applyCustomKeyboardLayout(const String& layoutId);
    
    // =================== 设置监控和统计 ===================
    
    /**
     * @brief 获取设置变更历史
     * @param key 设置键名，空字符串获取全部
     * @param maxRecords 最大记录数
     * @return 变更记录列表
     */
    std::vector<SettingChangeRecord> getChangeHistory(const String& key = "", size_t maxRecords = 100) const;
    
    /**
     * @brief 获取设置使用统计
     * @return 使用统计映射
     */
    std::map<String, uint32_t> getUsageStatistics() const;
    
    /**
     * @brief 清理变更历史
     * @param maxRecords 保留的最大记录数
     * @param maxAgeDays 保留的最大天数
     * @return 清理的记录数量
     */
    size_t cleanupChangeHistory(size_t maxRecords = 1000, uint32_t maxAgeDays = 90);
    
    // =================== 实时设置应用 ===================
    
    /**
     * @brief 立即应用设置变更
     * @param key 设置键名
     * @param value 新值
     * @return true 成功，false 失败
     */
    bool applySettingChange(const String& key, const String& value);
    
    /**
     * @brief 批量应用设置变更
     * @param settings 设置映射
     * @return 成功应用的数量
     */
    size_t applySettingChanges(const std::map<String, String>& settings);
    
    /**
     * @brief 注册设置变更监听器
     * @param key 设置键名，空字符串监听所有设置
     * @param callback 回调函数
     * @return 监听器ID
     */
    uint32_t addSettingChangeListener(const String& key, std::function<void(const String&, const String&, const String&)> callback);
    
    /**
     * @brief 移除设置变更监听器
     * @param listenerId 监听器ID
     * @return true 成功，false 失败
     */
    bool removeSettingChangeListener(uint32_t listenerId);
    
    // =================== 状态查询 ===================
    
    /**
     * @brief 检查是否已初始化
     * @return true 已初始化，false 未初始化
     */
    bool isInitialized() const { return _initialized; }
    
    /**
     * @brief 检查是否有未保存的变更
     * @return true 有变更，false 无变更
     */
    bool hasUnsavedChanges() const { return _hasUnsavedChanges; }
    
    /**
     * @brief 获取设置数量统计
     * @return 设置数量映射
     */
    std::map<String, size_t> getSettingsStatistics() const;
    
    /**
     * @brief 保存所有变更
     * @return true 成功，false 失败
     */
    bool saveAllChanges();
    
    /**
     * @brief 重新加载设置
     * @return true 成功，false 失败
     */
    bool reloadSettings();
    
    /**
     * @brief 打印设置信息（调试用）
     * @param category 设置类别，空值打印全部
     */
    void printSettings(SettingsCategory category = SettingsCategory::MAX_CATEGORIES) const;

private:
    // 单例实例
    static AdvancedSettingsManager* _instance;
    
    // 核心组件引用
    ConfigManager& _configManager;
    KeyboardConfigManager& _keyboardConfig;
    
    // 状态变量
    bool _initialized;
    bool _hasUnsavedChanges;
    mutable SemaphoreHandle_t _mutex;
    
    // 设置定义存储
    std::map<String, SettingDefinition> _settingDefinitions;
    std::map<SettingsCategory, std::vector<String>> _settingsByCategory;
    
    // 用户配置文件存储
    std::map<String, UserProfile> _userProfiles;
    String _currentProfileId;
    
    // 设置备份存储
    std::map<String, SettingsBackup> _settingsBackups;
    
    // 变更监控
    std::vector<SettingChangeRecord> _changeHistory;
    std::map<String, uint32_t> _usageStatistics;
    std::map<uint32_t, std::pair<String, std::function<void(const String&, const String&, const String&)>>> _changeListeners;
    uint32_t _nextListenerId;
    
    // Preferences存储
    Preferences _preferences;
    static const char* PREF_NAMESPACE;
    static const char* PREF_PROFILES_KEY;
    static const char* PREF_BACKUPS_KEY;
    static const char* PREF_HISTORY_KEY;
    static const char* PREF_CURRENT_PROFILE_KEY;
    
    // 内部方法
    void loadDefaultSettings();
    void loadUserProfiles();
    void saveUserProfiles();
    void loadBackups();
    void saveBackups();
    void loadChangeHistory();
    void saveChangeHistory();
    
    bool validateAndApplySettings(const std::map<String, String>& settings, std::map<String, String>& errors);
    void recordSettingChange(const String& key, const String& oldValue, const String& newValue, const String& source);
    void notifySettingChange(const String& key, const String& oldValue, const String& newValue);
    
    String generateUniqueId() const;
    uint32_t calculateChecksum(const String& data) const;
    
    // 线程安全方法
    void lock() const;
    void unlock() const;
    bool tryLock(uint32_t timeoutMs = 100) const;
    
    // JSON序列化辅助方法
    void serializeUserProfile(const UserProfile& profile, JsonDocument& doc) const;
    bool deserializeUserProfile(const JsonDocument& doc, UserProfile& profile) const;
    void serializeSettingDefinition(const SettingDefinition& definition, JsonDocument& doc) const;
    bool deserializeSettingDefinition(const JsonDocument& doc, SettingDefinition& definition) const;
    
    // 设置应用辅助方法
    bool applyKeyboardSetting(const String& key, const String& value);
    bool applyDisplaySetting(const String& key, const String& value);
    bool applyAudioSetting(const String& key, const String& value);
    bool applySystemSetting(const String& key, const String& value);
    
    // 常量定义
    static constexpr size_t MAX_PROFILES = 10;
    static constexpr size_t MAX_BACKUPS = 20;
    static constexpr size_t MAX_CHANGE_HISTORY = 1000;
    static constexpr size_t MAX_JSON_SIZE = 32768;  // 32KB
    static constexpr uint32_t AUTO_SAVE_INTERVAL = 30000; // 30秒
};

// 全局实例访问函数
#define AdvancedSettings AdvancedSettingsManager::getInstance()

// 设置类别工具函数
namespace SettingsUtils {
    String categoryToString(SettingsCategory category);
    SettingsCategory stringToCategory(const String& str);
    String settingTypeToString(SettingType type);
    SettingType stringToSettingType(const String& str);
    
    // 常用设置键名常量
    namespace Keys {
        // 键盘设置
        extern const String KEYBOARD_LAYOUT;
        extern const String KEYBOARD_REPEAT_DELAY;
        extern const String KEYBOARD_REPEAT_RATE;
        extern const String KEYBOARD_LONGPRESS_DELAY;
        extern const String KEYBOARD_LAYER_SWITCH_TIMEOUT;
        
        // 显示设置
        extern const String DISPLAY_BRIGHTNESS;
        extern const String DISPLAY_CONTRAST;
        extern const String DISPLAY_ORIENTATION;
        extern const String DISPLAY_SLEEP_TIMEOUT;
        
        // 音频设置
        extern const String AUDIO_MASTER_VOLUME;
        extern const String AUDIO_BUZZER_ENABLED;
        extern const String AUDIO_BUZZER_VOLUME;
        extern const String AUDIO_KEY_SOUNDS;
        
        // 系统设置
        extern const String SYSTEM_AUTO_SAVE;
        extern const String SYSTEM_LOG_LEVEL;
        extern const String SYSTEM_LANGUAGE;
        extern const String SYSTEM_TIMEZONE;
    }
}

#endif // ADVANCED_SETTINGS_MANAGER_H