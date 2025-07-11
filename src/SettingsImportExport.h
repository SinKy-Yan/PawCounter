/**
 * @file SettingsImportExport.h
 * @brief 设置导入导出功能模块
 * @details 提供完整的设置配置导入导出、备份恢复和迁移功能
 * 
 * 核心功能：
 * - 多格式导入导出（JSON、XML、二进制、纯文本）
 * - 增量导入导出
 * - 版本兼容性处理
 * - 数据压缩和加密
 * - 完整性验证
 * - 自动备份和恢复
 * - 配置迁移向导
 * - 云同步支持
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef SETTINGS_IMPORT_EXPORT_H
#define SETTINGS_IMPORT_EXPORT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <map>
#include <functional>
#include "Logger.h"

/**
 * @brief 导入导出格式
 */
enum class ExportFormat {
    JSON = 0,           ///< JSON格式
    XML,                ///< XML格式
    BINARY,             ///< 二进制格式
    INI,                ///< INI配置文件格式
    CSV,                ///< CSV表格格式
    YAML,               ///< YAML格式
    COMPRESSED_JSON,    ///< 压缩JSON格式
    ENCRYPTED_JSON,     ///< 加密JSON格式
    CUSTOM              ///< 自定义格式
};

/**
 * @brief 导入导出范围
 */
enum class ExportScope {
    ALL_SETTINGS = 0,   ///< 所有设置
    USER_SETTINGS,      ///< 用户设置
    SYSTEM_SETTINGS,    ///< 系统设置
    KEYBOARD_SETTINGS,  ///< 键盘设置
    DISPLAY_SETTINGS,   ///< 显示设置
    AUDIO_SETTINGS,     ///< 音频设置
    CUSTOM_SCOPE        ///< 自定义范围
};

/**
 * @brief 导入导出选项
 */
struct ImportExportOptions {
    ExportFormat format = ExportFormat::JSON;           ///< 导出格式
    ExportScope scope = ExportScope::ALL_SETTINGS;     ///< 导出范围
    
    // 数据处理选项
    bool includeMetadata = true;                        ///< 包含元数据
    bool includeComments = true;                        ///< 包含注释
    bool includeDefaults = false;                       ///< 包含默认值
    bool includeTimestamps = true;                      ///< 包含时间戳
    bool includeVersionInfo = true;                     ///< 包含版本信息
    bool includeValidationInfo = false;                 ///< 包含验证信息
    
    // 安全选项
    bool enableCompression = false;                     ///< 启用压缩
    bool enableEncryption = false;                      ///< 启用加密
    String encryptionKey;                               ///< 加密密钥
    String encryptionAlgorithm = "AES-256-GCM";        ///< 加密算法
    
    // 质量控制
    bool verifyIntegrity = true;                        ///< 验证完整性
    bool validateBeforeExport = true;                   ///< 导出前验证
    bool validateAfterImport = true;                    ///< 导入后验证
    bool createBackupBeforeImport = true;               ///< 导入前创建备份
    
    // 过滤选项
    std::vector<String> includeKeys;                    ///< 包含的键名列表
    std::vector<String> excludeKeys;                    ///< 排除的键名列表
    std::vector<String> includeCategories;              ///< 包含的分类
    std::vector<String> excludeCategories;              ///< 排除的分类
    
    // 兼容性选项
    String targetVersion;                               ///< 目标版本
    bool enableVersionMigration = true;                 ///< 启用版本迁移
    bool strictCompatibilityCheck = false;              ///< 严格兼容性检查
    
    // 增量导入导出
    bool incrementalMode = false;                       ///< 增量模式
    String baselineId;                                  ///< 基线ID
    uint32_t lastModifiedTime = 0;                      ///< 上次修改时间
    
    // 回调函数
    std::function<void(float)> progressCallback;        ///< 进度回调
    std::function<bool(const String&)> confirmCallback; ///< 确认回调
    std::function<void(const String&)> logCallback;     ///< 日志回调
};

/**
 * @brief 导入导出结果
 */
struct ImportExportResult {
    bool success = false;                               ///< 是否成功
    String message;                                     ///< 结果消息
    String errorDetails;                                ///< 错误详情
    
    // 统计信息
    size_t totalItems = 0;                              ///< 总项目数
    size_t processedItems = 0;                          ///< 已处理项目数
    size_t successfulItems = 0;                         ///< 成功项目数
    size_t failedItems = 0;                             ///< 失败项目数
    size_t skippedItems = 0;                            ///< 跳过项目数
    
    // 时间信息
    uint32_t startTime = 0;                             ///< 开始时间
    uint32_t endTime = 0;                               ///< 结束时间
    uint32_t processingTime = 0;                        ///< 处理时间
    
    // 数据信息
    size_t originalSize = 0;                            ///< 原始数据大小
    size_t compressedSize = 0;                          ///< 压缩后大小
    float compressionRatio = 0.0f;                      ///< 压缩比率
    String dataChecksum;                                ///< 数据校验和
    
    // 详细结果
    std::vector<String> warnings;                       ///< 警告列表
    std::vector<String> errors;                         ///< 错误列表
    std::map<String, String> importedSettings;          ///< 导入的设置
    std::map<String, String> failedSettings;            ///< 失败的设置
    std::vector<String> migratedKeys;                   ///< 迁移的键名
    
    // 备份信息
    String backupId;                                    ///< 备份ID
    String backupPath;                                  ///< 备份路径
};

/**
 * @brief 配置备份信息
 */
struct ConfigBackup {
    String backupId;                                    ///< 备份ID
    String name;                                        ///< 备份名称
    String description;                                 ///< 备份描述
    uint32_t timestamp;                                 ///< 备份时间戳
    String version;                                     ///< 版本信息
    ExportFormat format;                                ///< 备份格式
    
    // 备份内容
    size_t dataSize;                                    ///< 数据大小
    String checksum;                                    ///< 校验和
    bool isCompressed = false;                          ///< 是否压缩
    bool isEncrypted = false;                           ///< 是否加密
    
    // 元数据
    String source;                                      ///< 备份来源
    bool isAutoBackup = false;                          ///< 是否自动备份
    uint32_t retentionDays = 30;                        ///< 保留天数
    String filePath;                                    ///< 文件路径
    
    // 标签和分类
    std::vector<String> tags;                           ///< 标签
    String category;                                    ///< 分类
    uint8_t priority = 5;                               ///< 优先级（1-10）
};

/**
 * @brief 版本迁移规则
 */
struct MigrationRule {
    String ruleId;                                      ///< 规则ID
    String sourceVersion;                               ///< 源版本
    String targetVersion;                               ///< 目标版本
    String description;                                 ///< 描述
    
    // 迁移动作
    enum class Action {
        RENAME_KEY,         ///< 重命名键
        TRANSFORM_VALUE,    ///< 转换值
        REMOVE_KEY,         ///< 删除键
        ADD_KEY,            ///< 添加键
        MERGE_KEYS,         ///< 合并键
        SPLIT_KEY,          ///< 拆分键
        CONDITIONAL         ///< 条件迁移
    } action;
    
    String sourceKey;                                   ///< 源键名
    String targetKey;                                   ///< 目标键名
    std::function<String(const String&)> transformer;  ///< 值转换函数
    std::function<bool(const String&, const String&)> condition; ///< 条件函数
    
    bool isRequired = true;                             ///< 是否必需
    String fallbackValue;                               ///< 回退值
};

/**
 * @brief 设置导入导出管理器
 */
class SettingsImportExport {
public:
    /**
     * @brief 构造函数
     */
    SettingsImportExport();
    
    /**
     * @brief 析构函数
     */
    ~SettingsImportExport();
    
    /**
     * @brief 初始化导入导出管理器
     * @return true 成功，false 失败
     */
    bool initialize();
    
    /**
     * @brief 关闭管理器
     */
    void shutdown();
    
    // =================== 导出功能 ===================
    
    /**
     * @brief 导出设置到字符串
     * @param settings 设置映射
     * @param options 导出选项
     * @return 导出结果
     */
    ImportExportResult exportToString(const std::map<String, String>& settings, const ImportExportOptions& options = ImportExportOptions()) const;
    
    /**
     * @brief 导出设置到文件
     * @param settings 设置映射
     * @param filePath 文件路径
     * @param options 导出选项
     * @return 导出结果
     */
    ImportExportResult exportToFile(const std::map<String, String>& settings, const String& filePath, const ImportExportOptions& options = ImportExportOptions()) const;
    
    /**
     * @brief 导出设置到流
     * @param settings 设置映射
     * @param stream 输出流
     * @param options 导出选项
     * @return 导出结果
     */
    ImportExportResult exportToStream(const std::map<String, String>& settings, Stream& stream, const ImportExportOptions& options = ImportExportOptions()) const;
    
    /**
     * @brief 增量导出
     * @param currentSettings 当前设置
     * @param baselineSettings 基线设置
     * @param options 导出选项
     * @return 导出结果
     */
    ImportExportResult exportIncremental(const std::map<String, String>& currentSettings, const std::map<String, String>& baselineSettings, const ImportExportOptions& options = ImportExportOptions()) const;
    
    // =================== 导入功能 ===================
    
    /**
     * @brief 从字符串导入设置
     * @param data 数据字符串
     * @param settings 导入的设置（输出）
     * @param options 导入选项
     * @return 导入结果
     */
    ImportExportResult importFromString(const String& data, std::map<String, String>& settings, const ImportExportOptions& options = ImportExportOptions());
    
    /**
     * @brief 从文件导入设置
     * @param filePath 文件路径
     * @param settings 导入的设置（输出）
     * @param options 导入选项
     * @return 导入结果
     */
    ImportExportResult importFromFile(const String& filePath, std::map<String, String>& settings, const ImportExportOptions& options = ImportExportOptions());
    
    /**
     * @brief 从流导入设置
     * @param stream 输入流
     * @param settings 导入的设置（输出）
     * @param options 导入选项
     * @return 导入结果
     */
    ImportExportResult importFromStream(Stream& stream, std::map<String, String>& settings, const ImportExportOptions& options = ImportExportOptions());
    
    /**
     * @brief 合并导入设置
     * @param data 数据字符串
     * @param currentSettings 当前设置（输入输出）
     * @param options 导入选项
     * @return 导入结果
     */
    ImportExportResult mergeImport(const String& data, std::map<String, String>& currentSettings, const ImportExportOptions& options = ImportExportOptions());
    
    // =================== 备份管理 ===================
    
    /**
     * @brief 创建设置备份
     * @param settings 设置映射
     * @param name 备份名称
     * @param description 备份描述
     * @param options 备份选项
     * @return 备份ID，失败返回空字符串
     */
    String createBackup(const std::map<String, String>& settings, const String& name, const String& description = "", const ImportExportOptions& options = ImportExportOptions());
    
    /**
     * @brief 恢复设置备份
     * @param backupId 备份ID
     * @param settings 恢复的设置（输出）
     * @param options 恢复选项
     * @return 恢复结果
     */
    ImportExportResult restoreBackup(const String& backupId, std::map<String, String>& settings, const ImportExportOptions& options = ImportExportOptions());
    
    /**
     * @brief 删除备份
     * @param backupId 备份ID
     * @return true 成功，false 失败
     */
    bool deleteBackup(const String& backupId);
    
    /**
     * @brief 获取所有备份
     * @return 备份列表
     */
    std::vector<ConfigBackup> getAllBackups() const;
    
    /**
     * @brief 获取备份信息
     * @param backupId 备份ID
     * @return 备份信息指针，未找到返回nullptr
     */
    const ConfigBackup* getBackupInfo(const String& backupId) const;
    
    /**
     * @brief 自动清理旧备份
     * @param maxBackups 最大备份数量
     * @param maxAgeDays 最大保留天数
     * @return 清理的备份数量
     */
    size_t cleanupOldBackups(size_t maxBackups = 10, uint32_t maxAgeDays = 30);
    
    // =================== 版本迁移 ===================
    
    /**
     * @brief 注册迁移规则
     * @param rule 迁移规则
     * @return true 成功，false 失败
     */
    bool registerMigrationRule(const MigrationRule& rule);
    
    /**
     * @brief 执行版本迁移
     * @param settings 设置映射（输入输出）
     * @param sourceVersion 源版本
     * @param targetVersion 目标版本
     * @return 迁移结果
     */
    ImportExportResult migrateVersion(std::map<String, String>& settings, const String& sourceVersion, const String& targetVersion);
    
    /**
     * @brief 检测设置版本
     * @param settings 设置映射
     * @return 版本字符串
     */
    String detectVersion(const std::map<String, String>& settings) const;
    
    /**
     * @brief 获取支持的版本列表
     * @return 版本列表
     */
    std::vector<String> getSupportedVersions() const;
    
    /**
     * @brief 检查版本兼容性
     * @param sourceVersion 源版本
     * @param targetVersion 目标版本
     * @return true 兼容，false 不兼容
     */
    bool isVersionCompatible(const String& sourceVersion, const String& targetVersion) const;
    
    // =================== 格式转换 ===================
    
    /**
     * @brief 转换设置格式
     * @param data 输入数据
     * @param fromFormat 源格式
     * @param toFormat 目标格式
     * @return 转换后的数据
     */
    String convertFormat(const String& data, ExportFormat fromFormat, ExportFormat toFormat) const;
    
    /**
     * @brief 检测数据格式
     * @param data 数据字符串
     * @return 检测到的格式
     */
    ExportFormat detectFormat(const String& data) const;
    
    /**
     * @brief 验证数据格式
     * @param data 数据字符串
     * @param format 期望格式
     * @return true 格式正确，false 格式错误
     */
    bool validateFormat(const String& data, ExportFormat format) const;
    
    // =================== 模板和预设 ===================
    
    /**
     * @brief 创建导出模板
     * @param templateName 模板名称
     * @param options 导出选项
     * @return true 成功，false 失败
     */
    bool createExportTemplate(const String& templateName, const ImportExportOptions& options);
    
    /**
     * @brief 应用导出模板
     * @param templateName 模板名称
     * @return 导出选项
     */
    ImportExportOptions applyExportTemplate(const String& templateName) const;
    
    /**
     * @brief 获取内置预设
     * @return 预设列表
     */
    std::vector<String> getBuiltinPresets() const;
    
    /**
     * @brief 获取预设选项
     * @param presetName 预设名称
     * @return 导出选项
     */
    ImportExportOptions getPresetOptions(const String& presetName) const;
    
    // =================== 云同步支持 ===================
    
    /**
     * @brief 上传设置到云端
     * @param settings 设置映射
     * @param cloudService 云服务名称
     * @param options 上传选项
     * @return 上传结果
     */
    ImportExportResult uploadToCloud(const std::map<String, String>& settings, const String& cloudService, const ImportExportOptions& options = ImportExportOptions());
    
    /**
     * @brief 从云端下载设置
     * @param cloudService 云服务名称
     * @param settings 下载的设置（输出）
     * @param options 下载选项
     * @return 下载结果
     */
    ImportExportResult downloadFromCloud(const String& cloudService, std::map<String, String>& settings, const ImportExportOptions& options = ImportExportOptions());
    
    /**
     * @brief 同步设置到云端
     * @param localSettings 本地设置
     * @param cloudService 云服务名称
     * @param options 同步选项
     * @return 同步结果
     */
    ImportExportResult syncWithCloud(std::map<String, String>& localSettings, const String& cloudService, const ImportExportOptions& options = ImportExportOptions());
    
    // =================== 状态和统计 ===================
    
    /**
     * @brief 检查是否已初始化
     * @return true 已初始化，false 未初始化
     */
    bool isInitialized() const { return _initialized; }
    
    /**
     * @brief 获取操作统计
     * @return 统计信息映射
     */
    std::map<String, uint32_t> getOperationStatistics() const;
    
    /**
     * @brief 获取性能指标
     * @return 性能指标映射
     */
    std::map<String, float> getPerformanceMetrics() const;
    
    /**
     * @brief 重置统计信息
     */
    void resetStatistics();
    
    /**
     * @brief 生成操作报告
     * @param result 导入导出结果
     * @return HTML格式报告
     */
    String generateOperationReport(const ImportExportResult& result) const;

private:
    bool _initialized;
    
    // 备份存储
    std::map<String, ConfigBackup> _backups;
    String _backupDirectory;
    
    // 迁移规则
    std::map<String, std::vector<MigrationRule>> _migrationRules; // 版本 -> 规则列表
    
    // 模板和预设
    std::map<String, ImportExportOptions> _exportTemplates;
    std::map<String, ImportExportOptions> _builtinPresets;
    
    // 统计信息
    mutable uint32_t _totalExports;
    mutable uint32_t _totalImports;
    mutable uint32_t _totalBackups;
    mutable uint32_t _totalMigrations;
    mutable uint32_t _totalErrors;
    mutable uint64_t _totalBytesProcessed;
    mutable uint32_t _totalProcessingTime;
    
    // 内部方法
    ImportExportResult exportToJSON(const std::map<String, String>& settings, const ImportExportOptions& options) const;
    ImportExportResult exportToXML(const std::map<String, String>& settings, const ImportExportOptions& options) const;
    ImportExportResult exportToBinary(const std::map<String, String>& settings, const ImportExportOptions& options) const;
    ImportExportResult exportToINI(const std::map<String, String>& settings, const ImportExportOptions& options) const;
    
    ImportExportResult importFromJSON(const String& data, std::map<String, String>& settings, const ImportExportOptions& options);
    ImportExportResult importFromXML(const String& data, std::map<String, String>& settings, const ImportExportOptions& options);
    ImportExportResult importFromBinary(const String& data, std::map<String, String>& settings, const ImportExportOptions& options);
    ImportExportResult importFromINI(const String& data, std::map<String, String>& settings, const ImportExportOptions& options);
    
    String compressData(const String& data) const;
    String decompressData(const String& compressedData) const;
    String encryptData(const String& data, const String& key, const String& algorithm) const;
    String decryptData(const String& encryptedData, const String& key, const String& algorithm) const;
    
    String calculateChecksum(const String& data) const;
    bool verifyChecksum(const String& data, const String& expectedChecksum) const;
    
    void applyFilters(std::map<String, String>& settings, const ImportExportOptions& options) const;
    void addMetadata(JsonDocument& doc, const ImportExportOptions& options) const;
    
    void loadBackups();
    void saveBackups();
    void createBuiltinPresets();
    void createBuiltinMigrationRules();
    
    String generateUniqueId() const;
    void updateStatistics(const ImportExportResult& result) const;
    
    // 文件系统辅助方法
    bool ensureDirectoryExists(const String& path) const;
    bool writeToFile(const String& filePath, const String& data) const;
    String readFromFile(const String& filePath) const;
    bool deleteFile(const String& filePath) const;
    
    // 格式检测辅助方法
    bool looksLikeJSON(const String& data) const;
    bool looksLikeXML(const String& data) const;
    bool looksLikeINI(const String& data) const;
    bool looksLikeCSV(const String& data) const;
    
    // 常量定义
    static constexpr size_t MAX_BACKUPS = 100;
    static constexpr size_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB
    static constexpr uint32_t OPERATION_TIMEOUT_MS = 30000; // 30秒
    static const char* BACKUP_DIRECTORY;
    static const char* METADATA_VERSION;
};

// 全局实例访问
extern SettingsImportExport settingsImportExport;

// 工具函数
namespace ImportExportUtils {
    String formatToString(ExportFormat format);
    ExportFormat stringToFormat(const String& str);
    String scopeToString(ExportScope scope);
    ExportScope stringToScope(const String& str);
    
    // 数据验证工具
    bool isValidJSON(const String& json);
    bool isValidXML(const String& xml);
    bool isValidBase64(const String& base64);
    
    // 文件名工具
    String generateBackupFileName(const String& name, ExportFormat format);
    String getFileExtension(ExportFormat format);
    String sanitizeFileName(const String& name);
    
    // 压缩工具
    String compressString(const String& input);
    String decompressString(const String& compressed);
    float calculateCompressionRatio(size_t original, size_t compressed);
    
    // 进度计算
    void updateProgress(float progress, const std::function<void(float)>& callback);
    String formatProgressMessage(const String& operation, float progress, size_t current, size_t total);
}

#endif // SETTINGS_IMPORT_EXPORT_H