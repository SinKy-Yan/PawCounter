/**
 * @file SettingsValidator.h
 * @brief 设置验证和错误处理模块
 * @details 提供完整的设置值验证、错误处理和数据完整性检查功能
 * 
 * 核心功能：
 * - 多层次设置验证（类型、范围、格式、逻辑）
 * - 自定义验证规则引擎
 * - 错误分类和处理策略
 * - 数据完整性检查
 * - 配置冲突检测
 * - 验证结果缓存
 * - 批量验证优化
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef SETTINGS_VALIDATOR_H
#define SETTINGS_VALIDATOR_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <functional>
#include <regex>
#include "Logger.h"

/**
 * @brief 验证错误级别
 */
enum class ValidationLevel {
    INFO = 0,           ///< 信息
    WARNING,            ///< 警告
    ERROR,              ///< 错误
    CRITICAL            ///< 严重错误
};

/**
 * @brief 验证错误类型
 */
enum class ValidationError {
    NONE = 0,           ///< 无错误
    TYPE_MISMATCH,      ///< 类型不匹配
    OUT_OF_RANGE,       ///< 超出范围
    INVALID_FORMAT,     ///< 格式无效
    MISSING_REQUIRED,   ///< 缺少必需值
    DEPENDENCY_FAILED,  ///< 依赖检查失败
    CONFLICT_DETECTED,  ///< 冲突检测
    CONSTRAINT_VIOLATED,///< 约束违反
    CUSTOM_FAILED,      ///< 自定义验证失败
    SYSTEM_ERROR,       ///< 系统错误
    UNKNOWN_KEY,        ///< 未知键名
    PERMISSION_DENIED,  ///< 权限拒绝
    RESOURCE_LIMIT,     ///< 资源限制
    DEPRECATED_VALUE    ///< 已弃用的值
};

/**
 * @brief 验证结果
 */
struct ValidationResult {
    bool isValid = true;                    ///< 是否验证通过
    ValidationLevel level = ValidationLevel::INFO; ///< 错误级别
    ValidationError errorType = ValidationError::NONE; ///< 错误类型
    String errorMessage;                    ///< 错误消息
    String suggestion;                      ///< 修复建议
    String validationContext;               ///< 验证上下文
    uint32_t timestamp = 0;                 ///< 验证时间戳
    
    // 详细信息
    String settingKey;                      ///< 设置键名
    String actualValue;                     ///< 实际值
    String expectedFormat;                  ///< 期望格式
    std::vector<String> allowedValues;      ///< 允许的值列表
    float minValue = 0;                     ///< 最小值
    float maxValue = 0;                     ///< 最大值
};

/**
 * @brief 批量验证结果
 */
struct BatchValidationResult {
    bool allValid = true;                   ///< 是否全部验证通过
    size_t totalChecked = 0;                ///< 总检查数量
    size_t passedCount = 0;                 ///< 通过数量
    size_t warningCount = 0;                ///< 警告数量
    size_t errorCount = 0;                  ///< 错误数量
    size_t criticalCount = 0;               ///< 严重错误数量
    uint32_t validationTime = 0;            ///< 验证耗时（毫秒）
    
    std::map<String, ValidationResult> results; ///< 详细结果
    std::vector<String> criticalErrors;     ///< 严重错误列表
    std::vector<String> recommendations;    ///< 修复建议
};

/**
 * @brief 验证规则类型
 */
enum class ValidationRuleType {
    TYPE_CHECK,         ///< 类型检查
    RANGE_CHECK,        ///< 范围检查
    FORMAT_CHECK,       ///< 格式检查
    ENUM_CHECK,         ///< 枚举检查
    LENGTH_CHECK,       ///< 长度检查
    DEPENDENCY_CHECK,   ///< 依赖检查
    CONFLICT_CHECK,     ///< 冲突检查
    CUSTOM_CHECK        ///< 自定义检查
};

/**
 * @brief 验证规则定义
 */
struct ValidationRule {
    String ruleId;                          ///< 规则ID
    String name;                            ///< 规则名称
    String description;                     ///< 规则描述
    ValidationRuleType type;                ///< 规则类型
    ValidationLevel severity = ValidationLevel::ERROR; ///< 严重级别
    bool isEnabled = true;                  ///< 是否启用
    
    // 规则参数
    std::map<String, String> parameters;   ///< 参数映射
    
    // 验证函数
    std::function<ValidationResult(const String& key, const String& value, const std::map<String, String>& context)> validator;
    
    // 依赖设置
    std::vector<String> dependencies;      ///< 依赖的设置键
    std::vector<String> conflicts;         ///< 冲突的设置键
    
    // 条件执行
    std::function<bool(const String& key, const String& value)> condition; ///< 执行条件
    
    uint16_t priority = 100;                ///< 执行优先级（数值越小优先级越高）
    uint32_t executionCount = 0;            ///< 执行次数
    uint32_t lastExecutionTime = 0;         ///< 上次执行时间
};

/**
 * @brief 验证上下文
 */
struct ValidationContext {
    std::map<String, String> allSettings;  ///< 所有设置值
    std::map<String, String> changedSettings; ///< 变更的设置
    String validationSource;                ///< 验证来源
    bool isInitialValidation = false;       ///< 是否为初始验证
    bool isBatchValidation = false;         ///< 是否为批量验证
    uint32_t validationId = 0;              ///< 验证ID
    uint32_t startTime = 0;                 ///< 开始时间
};

/**
 * @brief 设置验证器类
 */
class SettingsValidator {
public:
    /**
     * @brief 构造函数
     */
    SettingsValidator();
    
    /**
     * @brief 析构函数
     */
    ~SettingsValidator();
    
    /**
     * @brief 初始化验证器
     * @return true 成功，false 失败
     */
    bool initialize();
    
    /**
     * @brief 关闭验证器
     */
    void shutdown();
    
    // =================== 验证规则管理 ===================
    
    /**
     * @brief 注册验证规则
     * @param rule 验证规则
     * @return true 成功，false 失败
     */
    bool registerValidationRule(const ValidationRule& rule);
    
    /**
     * @brief 批量注册验证规则
     * @param rules 验证规则列表
     * @return 成功注册的数量
     */
    size_t registerValidationRules(const std::vector<ValidationRule>& rules);
    
    /**
     * @brief 移除验证规则
     * @param ruleId 规则ID
     * @return true 成功，false 失败
     */
    bool removeValidationRule(const String& ruleId);
    
    /**
     * @brief 启用/禁用验证规则
     * @param ruleId 规则ID
     * @param enabled 是否启用
     * @return true 成功，false 失败
     */
    bool setRuleEnabled(const String& ruleId, bool enabled);
    
    /**
     * @brief 获取验证规则
     * @param ruleId 规则ID
     * @return 验证规则指针，未找到返回nullptr
     */
    const ValidationRule* getValidationRule(const String& ruleId) const;
    
    /**
     * @brief 获取所有验证规则
     * @return 验证规则列表
     */
    std::vector<ValidationRule> getAllValidationRules() const;
    
    // =================== 单项验证 ===================
    
    /**
     * @brief 验证单个设置
     * @param key 设置键名
     * @param value 设置值
     * @param context 验证上下文
     * @return 验证结果
     */
    ValidationResult validateSetting(const String& key, const String& value, const ValidationContext& context = ValidationContext()) const;
    
    /**
     * @brief 快速验证设置（仅基础检查）
     * @param key 设置键名
     * @param value 设置值
     * @return 是否通过验证
     */
    bool quickValidate(const String& key, const String& value) const;
    
    /**
     * @brief 验证设置类型
     * @param key 设置键名
     * @param value 设置值
     * @param expectedType 期望类型
     * @return 验证结果
     */
    ValidationResult validateType(const String& key, const String& value, const String& expectedType) const;
    
    /**
     * @brief 验证设置范围
     * @param key 设置键名
     * @param value 设置值
     * @param minValue 最小值
     * @param maxValue 最大值
     * @return 验证结果
     */
    ValidationResult validateRange(const String& key, const String& value, float minValue, float maxValue) const;
    
    /**
     * @brief 验证设置格式
     * @param key 设置键名
     * @param value 设置值
     * @param pattern 正则表达式模式
     * @return 验证结果
     */
    ValidationResult validateFormat(const String& key, const String& value, const String& pattern) const;
    
    /**
     * @brief 验证枚举值
     * @param key 设置键名
     * @param value 设置值
     * @param allowedValues 允许的值列表
     * @return 验证结果
     */
    ValidationResult validateEnum(const String& key, const String& value, const std::vector<String>& allowedValues) const;
    
    // =================== 批量验证 ===================
    
    /**
     * @brief 批量验证设置
     * @param settings 设置映射
     * @param context 验证上下文
     * @return 批量验证结果
     */
    BatchValidationResult validateSettings(const std::map<String, String>& settings, const ValidationContext& context = ValidationContext()) const;
    
    /**
     * @brief 验证设置依赖关系
     * @param settings 设置映射
     * @return 验证结果
     */
    ValidationResult validateDependencies(const std::map<String, String>& settings) const;
    
    /**
     * @brief 检测设置冲突
     * @param settings 设置映射
     * @return 验证结果
     */
    ValidationResult detectConflicts(const std::map<String, String>& settings) const;
    
    /**
     * @brief 验证配置完整性
     * @param settings 设置映射
     * @return 验证结果
     */
    ValidationResult validateIntegrity(const std::map<String, String>& settings) const;
    
    // =================== 高级验证 ===================
    
    /**
     * @brief 验证配置迁移
     * @param oldSettings 旧设置
     * @param newSettings 新设置
     * @param migrationRules 迁移规则
     * @return 验证结果
     */
    ValidationResult validateMigration(const std::map<String, String>& oldSettings, 
                                      const std::map<String, String>& newSettings,
                                      const std::map<String, String>& migrationRules) const;
    
    /**
     * @brief 验证性能影响
     * @param settings 设置映射
     * @return 验证结果
     */
    ValidationResult validatePerformanceImpact(const std::map<String, String>& settings) const;
    
    /**
     * @brief 验证安全性
     * @param settings 设置映射
     * @return 验证结果
     */
    ValidationResult validateSecurity(const std::map<String, String>& settings) const;
    
    /**
     * @brief 验证兼容性
     * @param settings 设置映射
     * @param targetVersion 目标版本
     * @return 验证结果
     */
    ValidationResult validateCompatibility(const std::map<String, String>& settings, const String& targetVersion) const;
    
    // =================== 错误处理 ===================
    
    /**
     * @brief 自动修复设置错误
     * @param settings 设置映射（输入输出）
     * @param fixableOnly 是否仅修复可自动修复的错误
     * @return 修复的错误数量
     */
    size_t autoFixErrors(std::map<String, String>& settings, bool fixableOnly = true) const;
    
    /**
     * @brief 获取修复建议
     * @param result 验证结果
     * @return 修复建议列表
     */
    std::vector<String> getFixSuggestions(const ValidationResult& result) const;
    
    /**
     * @brief 生成默认值
     * @param key 设置键名
     * @param context 上下文
     * @return 默认值，失败返回空字符串
     */
    String generateDefaultValue(const String& key, const ValidationContext& context = ValidationContext()) const;
    
    /**
     * @brief 清理无效设置
     * @param settings 设置映射（输入输出）
     * @return 清理的设置数量
     */
    size_t cleanupInvalidSettings(std::map<String, String>& settings) const;
    
    // =================== 验证缓存 ===================
    
    /**
     * @brief 启用验证缓存
     * @param enabled 是否启用
     * @param maxCacheSize 最大缓存大小
     */
    void setValidationCache(bool enabled, size_t maxCacheSize = 1000);
    
    /**
     * @brief 清理验证缓存
     */
    void clearValidationCache();
    
    /**
     * @brief 获取缓存统计
     * @return 缓存统计映射
     */
    std::map<String, uint32_t> getCacheStatistics() const;
    
    // =================== 预定义验证器 ===================
    
    /**
     * @brief 创建常用的验证规则
     */
    void createBuiltinValidationRules();
    
    /**
     * @brief 获取键盘设置验证规则
     * @return 验证规则列表
     */
    std::vector<ValidationRule> getKeyboardValidationRules() const;
    
    /**
     * @brief 获取显示设置验证规则
     * @return 验证规则列表
     */
    std::vector<ValidationRule> getDisplayValidationRules() const;
    
    /**
     * @brief 获取音频设置验证规则
     * @return 验证规则列表
     */
    std::vector<ValidationRule> getAudioValidationRules() const;
    
    /**
     * @brief 获取系统设置验证规则
     * @return 验证规则列表
     */
    std::vector<ValidationRule> getSystemValidationRules() const;
    
    // =================== 状态和统计 ===================
    
    /**
     * @brief 检查是否已初始化
     * @return true 已初始化，false 未初始化
     */
    bool isInitialized() const { return _initialized; }
    
    /**
     * @brief 获取验证统计
     * @return 统计信息映射
     */
    std::map<String, uint32_t> getValidationStatistics() const;
    
    /**
     * @brief 重置验证统计
     */
    void resetStatistics();
    
    /**
     * @brief 获取性能分析数据
     * @return 性能数据映射
     */
    std::map<String, float> getPerformanceMetrics() const;
    
    /**
     * @brief 打印验证规则信息
     */
    void printValidationRules() const;
    
    /**
     * @brief 导出验证结果报告
     * @param result 批量验证结果
     * @return HTML格式报告
     */
    String generateValidationReport(const BatchValidationResult& result) const;

private:
    bool _initialized;
    
    // 验证规则存储
    std::map<String, ValidationRule> _validationRules;
    std::map<ValidationRuleType, std::vector<String>> _rulesByType;
    
    // 验证缓存
    bool _cacheEnabled;
    size_t _maxCacheSize;
    mutable std::map<String, std::pair<ValidationResult, uint32_t>> _validationCache;
    mutable uint32_t _cacheHits;
    mutable uint32_t _cacheMisses;
    
    // 统计信息
    mutable uint32_t _totalValidations;
    mutable uint32_t _successfulValidations;
    mutable uint32_t _failedValidations;
    mutable uint32_t _totalValidationTime;
    mutable std::map<ValidationError, uint32_t> _errorCounts;
    
    // 性能监控
    mutable std::map<String, std::vector<uint32_t>> _ruleExecutionTimes;
    
    // 内部方法
    ValidationResult executeValidationRule(const ValidationRule& rule, const String& key, const String& value, const ValidationContext& context) const;
    std::vector<ValidationRule> getApplicableRules(const String& key) const;
    String generateCacheKey(const String& key, const String& value, const ValidationContext& context) const;
    
    bool isCacheValid(const String& cacheKey, uint32_t maxAge = 60000) const; // 60秒缓存
    void addToCache(const String& cacheKey, const ValidationResult& result) const;
    void cleanupCache() const;
    
    void updateStatistics(const ValidationResult& result, uint32_t executionTime) const;
    
    // 类型检查辅助方法
    bool isValidBoolean(const String& value) const;
    bool isValidInteger(const String& value) const;
    bool isValidFloat(const String& value) const;
    bool isValidColor(const String& value) const;
    bool isValidEmail(const String& value) const;
    bool isValidURL(const String& value) const;
    bool isValidPath(const String& value) const;
    
    // 建议生成辅助方法
    String generateRangeSuggestion(float value, float min, float max) const;
    String generateFormatSuggestion(const String& value, const String& pattern) const;
    String generateEnumSuggestion(const String& value, const std::vector<String>& allowedValues) const;
    
    // 错误消息生成
    String generateErrorMessage(ValidationError errorType, const String& key, const String& value, const String& details = "") const;
    
    // 常量定义
    static constexpr uint32_t MAX_VALIDATION_TIME_MS = 5000; // 5秒最大验证时间
    static constexpr size_t MAX_VALIDATION_RULES = 1000;
    static constexpr size_t DEFAULT_CACHE_SIZE = 500;
    static constexpr uint32_t CACHE_EXPIRE_TIME_MS = 300000; // 5分钟缓存过期时间
};

// 全局实例访问
extern SettingsValidator settingsValidator;

// 工具函数
namespace ValidationUtils {
    String levelToString(ValidationLevel level);
    ValidationLevel stringToLevel(const String& str);
    String errorToString(ValidationError error);
    ValidationError stringToError(const String& str);
    
    // 常用验证函数
    bool validateIPAddress(const String& ip);
    bool validateMACAddress(const String& mac);
    bool validateJSON(const String& json);
    bool validateRegex(const String& pattern);
    bool validateDateFormat(const String& date, const String& format);
    bool validateTimeFormat(const String& time, const String& format);
    
    // 值转换和标准化
    String normalizeValue(const String& value, const String& type);
    String sanitizeValue(const String& value);
    
    // 错误恢复策略
    enum class RecoveryStrategy {
        USE_DEFAULT,        ///< 使用默认值
        USE_LAST_VALID,     ///< 使用上次有效值
        USER_PROMPT,        ///< 提示用户
        AUTO_CORRECT,       ///< 自动修正
        DISABLE_FEATURE     ///< 禁用功能
    };
    
    RecoveryStrategy getRecoveryStrategy(ValidationError error);
    String applyRecoveryStrategy(const String& key, const String& invalidValue, RecoveryStrategy strategy);
}

#endif // SETTINGS_VALIDATOR_H