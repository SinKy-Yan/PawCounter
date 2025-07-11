/**
 * @file SettingsDefaultManager.h
 * @brief 设置默认值管理器 - 提供完整的默认设置管理和恢复功能
 * @details 管理系统默认设置、用户自定义默认值和恢复策略
 * 
 * 核心功能：
 * - 多层次默认值管理（系统、用户、情境）
 * - 智能默认值生成
 * - 选择性恢复功能
 * - 默认值继承和覆盖
 * - 动态默认值计算
 * - 默认值验证和优化
 * - 恢复策略自定义
 * - 默认值变更历史
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef SETTINGS_DEFAULT_MANAGER_H
#define SETTINGS_DEFAULT_MANAGER_H

#include <Arduino.h>
#include <map>
#include <vector>
#include <functional>
#include "Logger.h"

/**
 * @brief 默认值类型
 */
enum class DefaultValueType {
    SYSTEM_DEFAULT = 0,     ///< 系统默认值
    USER_DEFAULT,           ///< 用户自定义默认值
    CONTEXT_DEFAULT,        ///< 情境默认值
    CALCULATED_DEFAULT,     ///< 计算默认值
    INHERITED_DEFAULT,      ///< 继承默认值
    FACTORY_DEFAULT,        ///< 出厂默认值
    ADAPTIVE_DEFAULT        ///< 自适应默认值
};

/**
 * @brief 恢复范围
 */
enum class RestoreScope {
    ALL_SETTINGS = 0,       ///< 所有设置
    CATEGORY,               ///< 指定类别
    SPECIFIC_KEYS,          ///< 指定键名
    MODIFIED_ONLY,          ///< 仅修改过的设置
    INVALID_ONLY,           ///< 仅无效设置
    USER_LEVEL_ONLY,        ///< 仅用户级设置
    SYSTEM_LEVEL_ONLY,      ///< 仅系统级设置
    EXPERIMENTAL_ONLY       ///< 仅实验性设置
};

/**
 * @brief 恢复策略
 */
enum class RestoreStrategy {
    IMMEDIATE = 0,          ///< 立即恢复
    CONFIRM_FIRST,          ///< 确认后恢复
    BACKUP_FIRST,           ///< 备份后恢复
    MERGE_PRESERVE,         ///< 合并保留
    SELECTIVE_RESTORE,      ///< 选择性恢复
    GRADUAL_RESTORE,        ///< 渐进式恢复
    SAFE_RESTORE            ///< 安全恢复模式
};

/**
 * @brief 默认值定义
 */
struct DefaultValueDefinition {
    String key;                                     ///< 设置键名
    String value;                                   ///< 默认值
    DefaultValueType type;                          ///< 默认值类型
    String description;                             ///< 描述
    String category;                                ///< 类别
    
    // 计算和依赖
    std::function<String()> calculator;             ///< 动态计算函数
    std::vector<String> dependencies;               ///< 依赖的设置键
    std::map<String, String> conditions;            ///< 条件映射
    
    // 继承和覆盖
    String inheritFrom;                             ///< 继承源
    bool canBeOverridden = true;                    ///< 是否可被覆盖
    uint8_t priority = 5;                           ///< 优先级（1-10）
    
    // 验证和约束
    std::function<bool(const String&)> validator;   ///< 验证函数
    String validationMessage;                       ///< 验证消息
    
    // 元数据
    bool isExperimental = false;                    ///< 是否为实验性
    bool requiresRestart = false;                   ///< 是否需要重启
    String version;                                 ///< 引入版本
    uint32_t lastModified = 0;                      ///< 最后修改时间
    
    // 使用统计
    uint32_t accessCount = 0;                       ///< 访问次数
    uint32_t restoreCount = 0;                      ///< 恢复次数
};

/**
 * @brief 恢复操作记录
 */
struct RestoreOperation {
    String operationId;                             ///< 操作ID
    uint32_t timestamp;                             ///< 时间戳
    RestoreScope scope;                             ///< 恢复范围
    RestoreStrategy strategy;                       ///< 恢复策略
    String reason;                                  ///< 恢复原因
    
    // 操作详情
    std::vector<String> restoredKeys;               ///< 已恢复的键
    std::vector<String> skippedKeys;                ///< 跳过的键
    std::vector<String> failedKeys;                 ///< 失败的键
    std::map<String, String> oldValues;             ///< 旧值
    std::map<String, String> newValues;             ///< 新值
    
    // 结果信息
    bool success = false;                           ///< 是否成功
    String errorMessage;                            ///< 错误消息
    uint32_t duration = 0;                          ///< 操作耗时
    String backupId;                                ///< 备份ID
    
    // 验证信息
    bool validated = false;                         ///< 是否已验证
    std::vector<String> validationErrors;           ///< 验证错误
};

/**
 * @brief 默认值上下文
 */
struct DefaultValueContext {
    String userId;                                  ///< 用户ID
    String deviceId;                                ///< 设备ID
    String environment;                             ///< 环境（开发/测试/生产）
    String language;                                ///< 语言
    String region;                                  ///< 地区
    
    // 硬件信息
    String hardwareModel;                           ///< 硬件型号
    uint32_t memorySize = 0;                        ///< 内存大小
    uint32_t storageSize = 0;                       ///< 存储大小
    String capabilities;                            ///< 功能支持
    
    // 使用模式
    String usagePattern;                            ///< 使用模式
    String userLevel;                               ///< 用户级别
    std::map<String, String> preferences;           ///< 用户偏好
    
    // 时间信息
    uint32_t sessionTime = 0;                       ///< 会话时间
    uint32_t totalUsageTime = 0;                    ///< 总使用时间
    String timeZone;                                ///< 时区
};

/**
 * @brief 设置默认值管理器
 */
class SettingsDefaultManager {
public:
    /**
     * @brief 构造函数
     */
    SettingsDefaultManager();
    
    /**
     * @brief 析构函数
     */
    ~SettingsDefaultManager();
    
    /**
     * @brief 初始化默认值管理器
     * @return true 成功，false 失败
     */
    bool initialize();
    
    /**
     * @brief 关闭管理器
     */
    void shutdown();
    
    // =================== 默认值定义管理 ===================
    
    /**
     * @brief 注册默认值定义
     * @param definition 默认值定义
     * @return true 成功，false 失败
     */
    bool registerDefaultValue(const DefaultValueDefinition& definition);
    
    /**
     * @brief 批量注册默认值定义
     * @param definitions 默认值定义列表
     * @return 成功注册的数量
     */
    size_t registerDefaultValues(const std::vector<DefaultValueDefinition>& definitions);
    
    /**
     * @brief 更新默认值定义
     * @param key 设置键名
     * @param definition 新的默认值定义
     * @return true 成功，false 失败
     */
    bool updateDefaultValue(const String& key, const DefaultValueDefinition& definition);
    
    /**
     * @brief 移除默认值定义
     * @param key 设置键名
     * @return true 成功，false 失败
     */
    bool removeDefaultValue(const String& key);
    
    /**
     * @brief 获取默认值定义
     * @param key 设置键名
     * @return 默认值定义指针，未找到返回nullptr
     */
    const DefaultValueDefinition* getDefaultValueDefinition(const String& key) const;
    
    /**
     * @brief 获取所有默认值定义
     * @param type 默认值类型过滤，空值获取全部
     * @return 默认值定义列表
     */
    std::vector<DefaultValueDefinition> getAllDefaultValues(DefaultValueType type = static_cast<DefaultValueType>(-1)) const;
    
    // =================== 默认值获取 ===================
    
    /**
     * @brief 获取默认值
     * @param key 设置键名
     * @param context 上下文
     * @return 默认值，未找到返回空字符串
     */
    String getDefaultValue(const String& key, const DefaultValueContext& context = DefaultValueContext()) const;
    
    /**
     * @brief 获取计算的默认值
     * @param key 设置键名
     * @param context 上下文
     * @return 计算的默认值
     */
    String getCalculatedDefaultValue(const String& key, const DefaultValueContext& context = DefaultValueContext()) const;
    
    /**
     * @brief 获取自适应默认值
     * @param key 设置键名
     * @param currentValue 当前值
     * @param usageHistory 使用历史
     * @param context 上下文
     * @return 自适应默认值
     */
    String getAdaptiveDefaultValue(const String& key, const String& currentValue, const std::map<String, uint32_t>& usageHistory, const DefaultValueContext& context = DefaultValueContext()) const;
    
    /**
     * @brief 批量获取默认值
     * @param keys 设置键名列表
     * @param context 上下文
     * @return 默认值映射
     */
    std::map<String, String> getDefaultValues(const std::vector<String>& keys, const DefaultValueContext& context = DefaultValueContext()) const;
    
    /**
     * @brief 获取类别的所有默认值
     * @param category 类别名称
     * @param context 上下文
     * @return 默认值映射
     */
    std::map<String, String> getCategoryDefaultValues(const String& category, const DefaultValueContext& context = DefaultValueContext()) const;
    
    // =================== 恢复功能 ===================
    
    /**
     * @brief 恢复单个设置为默认值
     * @param key 设置键名
     * @param currentSettings 当前设置（输入输出）
     * @param strategy 恢复策略
     * @param context 上下文
     * @return 恢复操作记录
     */
    RestoreOperation restoreSetting(const String& key, std::map<String, String>& currentSettings, RestoreStrategy strategy = RestoreStrategy::IMMEDIATE, const DefaultValueContext& context = DefaultValueContext());
    
    /**
     * @brief 恢复指定范围的设置
     * @param scope 恢复范围
     * @param currentSettings 当前设置（输入输出）
     * @param strategy 恢复策略
     * @param scopeParams 范围参数（如类别名称、键名列表）
     * @param context 上下文
     * @return 恢复操作记录
     */
    RestoreOperation restoreSettings(RestoreScope scope, std::map<String, String>& currentSettings, RestoreStrategy strategy = RestoreStrategy::IMMEDIATE, const std::map<String, String>& scopeParams = std::map<String, String>(), const DefaultValueContext& context = DefaultValueContext());
    
    /**
     * @brief 恢复所有设置为默认值
     * @param currentSettings 当前设置（输入输出）
     * @param strategy 恢复策略
     * @param context 上下文
     * @return 恢复操作记录
     */
    RestoreOperation restoreAllSettings(std::map<String, String>& currentSettings, RestoreStrategy strategy = RestoreStrategy::BACKUP_FIRST, const DefaultValueContext& context = DefaultValueContext());
    
    /**
     * @brief 选择性恢复设置
     * @param keys 要恢复的键名列表
     * @param currentSettings 当前设置（输入输出）
     * @param strategy 恢复策略
     * @param context 上下文
     * @return 恢复操作记录
     */
    RestoreOperation selectiveRestore(const std::vector<String>& keys, std::map<String, String>& currentSettings, RestoreStrategy strategy = RestoreStrategy::CONFIRM_FIRST, const DefaultValueContext& context = DefaultValueContext());
    
    /**
     * @brief 智能恢复（仅恢复无效或有问题的设置）
     * @param currentSettings 当前设置（输入输出）
     * @param context 上下文
     * @return 恢复操作记录
     */
    RestoreOperation smartRestore(std::map<String, String>& currentSettings, const DefaultValueContext& context = DefaultValueContext());
    
    // =================== 用户自定义默认值 ===================
    
    /**
     * @brief 设置用户自定义默认值
     * @param key 设置键名
     * @param value 默认值
     * @param description 描述
     * @return true 成功，false 失败
     */
    bool setUserDefaultValue(const String& key, const String& value, const String& description = "");
    
    /**
     * @brief 移除用户自定义默认值
     * @param key 设置键名
     * @return true 成功，false 失败
     */
    bool removeUserDefaultValue(const String& key);
    
    /**
     * @brief 获取用户自定义默认值
     * @param key 设置键名
     * @return 用户默认值，未设置返回空字符串
     */
    String getUserDefaultValue(const String& key) const;
    
    /**
     * @brief 获取所有用户自定义默认值
     * @return 用户默认值映射
     */
    std::map<String, String> getAllUserDefaultValues() const;
    
    /**
     * @brief 从当前设置创建用户默认值
     * @param currentSettings 当前设置
     * @param keys 要保存为默认值的键名列表，空列表表示全部
     * @return 创建的默认值数量
     */
    size_t createUserDefaultsFromCurrent(const std::map<String, String>& currentSettings, const std::vector<String>& keys = std::vector<String>());
    
    // =================== 恢复历史和撤销 ===================
    
    /**
     * @brief 获取恢复历史
     * @param maxRecords 最大记录数
     * @return 恢复操作记录列表
     */
    std::vector<RestoreOperation> getRestoreHistory(size_t maxRecords = 50) const;
    
    /**
     * @brief 撤销恢复操作
     * @param operationId 操作ID
     * @param currentSettings 当前设置（输入输出）
     * @return true 成功，false 失败
     */
    bool undoRestore(const String& operationId, std::map<String, String>& currentSettings);
    
    /**
     * @brief 清理恢复历史
     * @param maxAge 最大保留时间（毫秒）
     * @return 清理的记录数量
     */
    size_t cleanupRestoreHistory(uint32_t maxAge = 7 * 24 * 60 * 60 * 1000); // 7天
    
    // =================== 默认值验证和优化 ===================
    
    /**
     * @brief 验证默认值
     * @param key 设置键名
     * @param value 默认值
     * @param context 上下文
     * @return true 有效，false 无效
     */
    bool validateDefaultValue(const String& key, const String& value, const DefaultValueContext& context = DefaultValueContext()) const;
    
    /**
     * @brief 优化默认值
     * @param context 上下文
     * @return 优化的默认值数量
     */
    size_t optimizeDefaultValues(const DefaultValueContext& context = DefaultValueContext());
    
    /**
     * @brief 检测冲突的默认值
     * @return 冲突列表
     */
    std::vector<String> detectConflictingDefaults() const;
    
    /**
     * @brief 分析默认值使用情况
     * @return 使用情况统计
     */
    std::map<String, uint32_t> analyzeDefaultValueUsage() const;
    
    // =================== 预设和模板 ===================
    
    /**
     * @brief 创建默认值预设
     * @param presetName 预设名称
     * @param currentSettings 当前设置
     * @param description 描述
     * @return true 成功，false 失败
     */
    bool createDefaultValuePreset(const String& presetName, const std::map<String, String>& currentSettings, const String& description = "");
    
    /**
     * @brief 应用默认值预设
     * @param presetName 预设名称
     * @param currentSettings 当前设置（输入输出）
     * @return 应用的设置数量
     */
    size_t applyDefaultValuePreset(const String& presetName, std::map<String, String>& currentSettings);
    
    /**
     * @brief 删除默认值预设
     * @param presetName 预设名称
     * @return true 成功，false 失败
     */
    bool deleteDefaultValuePreset(const String& presetName);
    
    /**
     * @brief 获取所有预设名称
     * @return 预设名称列表
     */
    std::vector<String> getAllPresetNames() const;
    
    // =================== 导入导出 ===================
    
    /**
     * @brief 导出默认值定义
     * @param format 导出格式
     * @return 导出数据，失败返回空字符串
     */
    String exportDefaultValues(const String& format = "json") const;
    
    /**
     * @brief 导入默认值定义
     * @param data 导入数据
     * @param format 数据格式
     * @return 导入的定义数量
     */
    size_t importDefaultValues(const String& data, const String& format = "json");
    
    // =================== 状态和统计 ===================
    
    /**
     * @brief 检查是否已初始化
     * @return true 已初始化，false 未初始化
     */
    bool isInitialized() const { return _initialized; }
    
    /**
     * @brief 获取统计信息
     * @return 统计信息映射
     */
    std::map<String, uint32_t> getStatistics() const;
    
    /**
     * @brief 获取默认值覆盖率
     * @param currentSettings 当前设置
     * @return 覆盖率（0.0-1.0）
     */
    float getDefaultValueCoverage(const std::map<String, String>& currentSettings) const;
    
    /**
     * @brief 生成默认值报告
     * @param context 上下文
     * @return HTML格式报告
     */
    String generateDefaultValueReport(const DefaultValueContext& context = DefaultValueContext()) const;
    
    /**
     * @brief 保存配置
     * @return true 成功，false 失败
     */
    bool saveConfiguration();
    
    /**
     * @brief 重新加载配置
     * @return true 成功，false 失败
     */
    bool reloadConfiguration();

private:
    bool _initialized;
    
    // 默认值定义存储
    std::map<String, DefaultValueDefinition> _systemDefaults;
    std::map<String, DefaultValueDefinition> _userDefaults;
    std::map<String, std::map<String, String>> _contextDefaults; // 情境 -> 键值映射
    
    // 预设存储
    std::map<String, std::map<String, String>> _presets;
    std::map<String, String> _presetDescriptions;
    
    // 恢复历史
    std::vector<RestoreOperation> _restoreHistory;
    
    // 统计信息
    mutable uint32_t _totalRestores;
    mutable uint32_t _totalAccesses;
    mutable uint32_t _totalCalculations;
    mutable std::map<String, uint32_t> _keyAccessCounts;
    mutable std::map<String, uint32_t> _keyRestoreCounts;
    
    // 存储
    Preferences _preferences;
    static const char* PREF_NAMESPACE;
    static const char* PREF_USER_DEFAULTS_KEY;
    static const char* PREF_PRESETS_KEY;
    static const char* PREF_HISTORY_KEY;
    
    // 内部方法
    void loadSystemDefaults();
    void loadUserDefaults();
    void saveUserDefaults();
    void loadPresets();
    void savePresets();
    void loadRestoreHistory();
    void saveRestoreHistory();
    
    String calculateDynamicDefault(const DefaultValueDefinition& definition, const DefaultValueContext& context) const;
    String resolveInheritance(const String& key, const DefaultValueContext& context) const;
    bool checkConditions(const std::map<String, String>& conditions, const DefaultValueContext& context) const;
    
    void recordRestoreOperation(const RestoreOperation& operation);
    String generateOperationId() const;
    
    bool shouldRestore(const String& key, const String& currentValue, RestoreScope scope, const std::map<String, String>& scopeParams) const;
    
    void updateStatistics(const String& key, const String& operation) const;
    
    // 内置默认值创建
    void createBuiltinSystemDefaults();
    void createBuiltinKeyboardDefaults();
    void createBuiltinDisplayDefaults();
    void createBuiltinAudioDefaults();
    void createBuiltinSystemSettingDefaults();
    
    // 辅助方法
    bool isValidKey(const String& key) const;
    bool isValidValue(const String& value) const;
    String sanitizeValue(const String& value) const;
    
    // 常量定义
    static constexpr size_t MAX_RESTORE_HISTORY = 100;
    static constexpr size_t MAX_USER_DEFAULTS = 500;
    static constexpr size_t MAX_PRESETS = 20;
    static constexpr uint32_t DEFAULT_CLEANUP_AGE = 30 * 24 * 60 * 60 * 1000; // 30天
};

// 全局实例访问
extern SettingsDefaultManager settingsDefaultManager;

// 工具函数
namespace DefaultManagerUtils {
    String defaultTypeToString(DefaultValueType type);
    DefaultValueType stringToDefaultType(const String& str);
    String restoreScopeToString(RestoreScope scope);
    RestoreScope stringToRestoreScope(const String& str);
    String restoreStrategyToString(RestoreStrategy strategy);
    RestoreStrategy stringToRestoreStrategy(const String& str);
    
    // 上下文检测
    DefaultValueContext detectContext();
    String getDeviceFingerprint();
    String getUserUsagePattern(const std::map<String, uint32_t>& usageHistory);
    
    // 值计算辅助
    String calculateHardwareOptimizedValue(const String& key, const DefaultValueContext& context);
    String calculateUsageOptimizedValue(const String& key, const std::map<String, uint32_t>& usageHistory);
    String calculateEnvironmentOptimizedValue(const String& key, const DefaultValueContext& context);
    
    // 预定义计算器
    std::function<String()> createMemoryBasedCalculator(const String& baseValue, float memoryFactor);
    std::function<String()> createPerformanceBasedCalculator(const String& baseValue, float performanceFactor);
    std::function<String()> createUsageBasedCalculator(const String& baseValue, const std::map<String, uint32_t>& usageHistory);
}

#endif // SETTINGS_DEFAULT_MANAGER_H