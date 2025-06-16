/**
 * @file FeedbackManager.h
 * @brief 硬件反馈管理系统统一接口
 * @details 整合LED灯效和蜂鸣器音效，提供统一的人机交互反馈管理
 * 
 * 功能特性：
 * - 统一的反馈接口管理LED和蜂鸣器
 * - 场景化反馈配置（按键、状态、事件）
 * - 智能反馈调度和优先级管理
 * - 用户偏好设置和配置文件
 * - 能耗优化和性能控制
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef FEEDBACK_MANAGER_H
#define FEEDBACK_MANAGER_H

#include <Arduino.h>
#include "LEDEffectManager.h"
#include "BuzzerSoundManager.h"
#include "Logger.h"

/**
 * @brief 反馈场景枚举
 */
enum FeedbackScene {
    SCENE_IDLE = 0,             ///< 空闲状态
    SCENE_KEY_PRESS,            ///< 按键按下
    SCENE_KEY_RELEASE,          ///< 按键释放
    SCENE_KEY_LONG_PRESS,       ///< 长按
    SCENE_CALCULATION_START,    ///< 开始计算
    SCENE_CALCULATION_SUCCESS,  ///< 计算成功
    SCENE_CALCULATION_ERROR,    ///< 计算错误
    SCENE_MODE_SWITCH,          ///< 模式切换
    SCENE_SYSTEM_STARTUP,       ///< 系统启动
    SCENE_SYSTEM_SHUTDOWN,      ///< 系统关机
    SCENE_BATTERY_LOW,          ///< 电量低
    SCENE_BATTERY_CHARGING,     ///< 正在充电
    SCENE_BATTERY_FULL,         ///< 电量满
    SCENE_ERROR_WARNING,        ///< 错误警告
    SCENE_SUCCESS_NOTIFICATION, ///< 成功通知
    SCENE_CUSTOM                ///< 自定义场景
};

/**
 * @brief 反馈强度枚举
 */
enum FeedbackIntensity {
    INTENSITY_OFF = 0,          ///< 关闭
    INTENSITY_SUBTLE = 1,       ///< 微弱
    INTENSITY_NORMAL = 2,       ///< 普通
    INTENSITY_STRONG = 3        ///< 强烈
};

/**
 * @brief 反馈模式枚举
 */
enum FeedbackMode {
    MODE_SILENT = 0,            ///< 静音模式（仅LED）
    MODE_VIBRATION_ONLY,        ///< 仅蜂鸣器（假设振动）
    MODE_VISUAL_ONLY,           ///< 仅视觉（LED）
    MODE_FULL,                  ///< 完整反馈（LED+蜂鸣器）
    MODE_ADAPTIVE               ///< 自适应模式
};

/**
 * @brief 综合反馈配置结构体
 */
struct FeedbackConfig {
    // LED配置
    LEDEffectConfig ledConfig;
    bool ledEnabled;
    uint8_t ledIntensity;       ///< LED强度 (0-3)
    
    // 音效配置
    SoundEffectConfig soundConfig;
    bool soundEnabled;
    uint8_t soundIntensity;     ///< 音效强度 (0-3)
    
    // 综合配置
    FeedbackIntensity intensity;
    uint16_t delay;             ///< 延迟时间 (ms)
    bool synchronized;          ///< 是否同步LED和音效
    
    // 构造函数
    FeedbackConfig() :
        ledEnabled(true),
        ledIntensity(2),
        soundEnabled(true),
        soundIntensity(2),
        intensity(INTENSITY_NORMAL),
        delay(0),
        synchronized(true) {}
};

/**
 * @brief 用户偏好配置结构体
 */
struct UserPreferences {
    FeedbackMode mode;              ///< 反馈模式
    FeedbackIntensity intensity;    ///< 全局强度
    bool keyClickSound;             ///< 按键音开关
    bool systemSounds;              ///< 系统音开关
    bool visualEffects;             ///< 视觉效果开关
    uint8_t globalVolume;           ///< 全局音量 (0-100)
    uint8_t globalBrightness;       ///< 全局亮度 (0-255)
    bool autoAdjustBrightness;      ///< 自动调节亮度
    bool energySaveMode;            ///< 节能模式
    uint16_t inactiveTimeout;       ///< 无活动超时 (s)
    
    // 构造函数提供默认值
    UserPreferences() :
        mode(MODE_FULL),
        intensity(INTENSITY_NORMAL),
        keyClickSound(true),
        systemSounds(true),
        visualEffects(true),
        globalVolume(50),
        globalBrightness(128),
        autoAdjustBrightness(false),
        energySaveMode(false),
        inactiveTimeout(300) {}
};

/**
 * @brief 反馈统计信息结构体
 */
struct FeedbackStats {
    uint32_t totalFeedbacks;        ///< 总反馈次数
    uint32_t ledActivations;        ///< LED激活次数
    uint32_t soundActivations;      ///< 音效激活次数
    uint32_t lastActivityTime;      ///< 最后活动时间
    uint32_t totalEnergyUsed;       ///< 总能耗（估算）
    
    FeedbackStats() : 
        totalFeedbacks(0), ledActivations(0), soundActivations(0),
        lastActivityTime(0), totalEnergyUsed(0) {}
};

/**
 * @brief 硬件反馈管理器主类
 */
class FeedbackManager {
public:
    /**
     * @brief 获取反馈管理器单例
     * @return FeedbackManager& 管理器实例引用
     */
    static FeedbackManager& getInstance();

    /**
     * @brief 初始化反馈管理器
     * @return true 初始化成功，false 初始化失败
     */
    bool begin();

    /**
     * @brief 更新反馈管理器（在主循环中调用）
     */
    void update();

    // === 场景化反馈接口 ===

    /**
     * @brief 触发场景反馈
     * @param scene 反馈场景
     * @param targetLED 目标LED索引（可选，默认-1表示使用场景默认）
     * @param customConfig 自定义配置（可选，覆盖默认配置）
     */
    void triggerFeedback(FeedbackScene scene, int8_t targetLED = -1, 
                        const FeedbackConfig* customConfig = nullptr);

    /**
     * @brief 触发按键反馈
     * @param keyIndex 按键索引
     * @param pressed 是否按下（true按下，false释放）
     * @param longPress 是否长按
     */
    void triggerKeyFeedback(uint8_t keyIndex, bool pressed, bool longPress = false);

    /**
     * @brief 触发计算器状态反馈
     * @param success 是否成功
     * @param errorType 错误类型（仅在success=false时有效）
     */
    void triggerCalculationFeedback(bool success, uint8_t errorType = 0);

    /**
     * @brief 触发系统状态反馈
     * @param scene 系统场景
     */
    void triggerSystemFeedback(FeedbackScene scene);

    /**
     * @brief 触发电池状态反馈
     * @param batteryLevel 电池电量 (0-100)
     * @param charging 是否正在充电
     */
    void triggerBatteryFeedback(uint8_t batteryLevel, bool charging);

    // === 直接控制接口 ===

    /**
     * @brief 直接播放LED效果
     * @param ledIndices LED索引数组
     * @param count LED数量
     * @param config LED效果配置
     */
    void playLEDEffect(const uint8_t* ledIndices, uint8_t count, 
                      const LEDEffectConfig& config);

    /**
     * @brief 直接播放音效
     * @param config 音效配置
     * @param interrupt 是否中断当前音效
     */
    void playSound(const SoundEffectConfig& config, bool interrupt = false);

    /**
     * @brief 播放综合反馈
     * @param config 综合反馈配置
     * @param targetLED 目标LED（-1表示不指定）
     */
    void playFeedback(const FeedbackConfig& config, int8_t targetLED = -1);

    // === 配置管理 ===

    /**
     * @brief 设置场景反馈配置
     * @param scene 场景
     * @param config 反馈配置
     */
    void setSceneConfig(FeedbackScene scene, const FeedbackConfig& config);

    /**
     * @brief 获取场景反馈配置
     * @param scene 场景
     * @return FeedbackConfig& 配置引用
     */
    const FeedbackConfig& getSceneConfig(FeedbackScene scene) const;

    /**
     * @brief 设置用户偏好
     * @param preferences 用户偏好配置
     */
    void setUserPreferences(const UserPreferences& preferences);

    /**
     * @brief 获取用户偏好
     * @return const UserPreferences& 用户偏好引用
     */
    const UserPreferences& getUserPreferences() const { return _userPrefs; }

    /**
     * @brief 设置反馈模式
     * @param mode 反馈模式
     */
    void setFeedbackMode(FeedbackMode mode);

    /**
     * @brief 获取反馈模式
     * @return FeedbackMode 当前反馈模式
     */
    FeedbackMode getFeedbackMode() const { return _userPrefs.mode; }

    /**
     * @brief 设置全局强度
     * @param intensity 强度级别
     */
    void setGlobalIntensity(FeedbackIntensity intensity);

    /**
     * @brief 启用/禁用按键音
     * @param enabled true启用，false禁用
     */
    void setKeyClickEnabled(bool enabled);

    /**
     * @brief 启用/禁用系统音效
     * @param enabled true启用，false禁用
     */
    void setSystemSoundsEnabled(bool enabled);

    /**
     * @brief 启用/禁用视觉效果
     * @param enabled true启用，false禁用
     */
    void setVisualEffectsEnabled(bool enabled);

    /**
     * @brief 设置节能模式
     * @param enabled true启用，false禁用
     */
    void setEnergySaveMode(bool enabled);

    // === 状态查询 ===

    /**
     * @brief 获取反馈统计信息
     * @return const FeedbackStats& 统计信息引用
     */
    const FeedbackStats& getStats() const { return _stats; }

    /**
     * @brief 重置统计信息
     */
    void resetStats();

    /**
     * @brief 检查是否处于静音模式
     * @return true 静音模式，false 非静音模式
     */
    bool isSilentMode() const;

    /**
     * @brief 检查是否在节能模式
     * @return true 节能模式，false 非节能模式
     */
    bool isEnergySaveMode() const { return _userPrefs.energySaveMode; }

    /**
     * @brief 获取估算的剩余电量使用时间（基于当前反馈使用情况）
     * @param currentBatteryLevel 当前电量百分比
     * @return uint32_t 估算剩余时间（分钟）
     */
    uint32_t getEstimatedBatteryLife(uint8_t currentBatteryLevel) const;

    // === 配置文件管理 ===

    /**
     * @brief 保存配置到存储
     * @return true 保存成功，false 保存失败
     */
    bool saveConfiguration();

    /**
     * @brief 从存储加载配置
     * @return true 加载成功，false 加载失败
     */
    bool loadConfiguration();

    /**
     * @brief 恢复默认配置
     */
    void restoreDefaults();

    /**
     * @brief 导出配置为JSON字符串
     * @return String JSON配置字符串
     */
    String exportConfiguration() const;

    /**
     * @brief 从JSON字符串导入配置
     * @param jsonConfig JSON配置字符串
     * @return true 导入成功，false 导入失败
     */
    bool importConfiguration(const String& jsonConfig);

private:
    FeedbackManager() = default;
    ~FeedbackManager() = default;
    FeedbackManager(const FeedbackManager&) = delete;
    FeedbackManager& operator=(const FeedbackManager&) = delete;

    bool _initialized;                          ///< 初始化状态
    UserPreferences _userPrefs;                 ///< 用户偏好配置
    FeedbackStats _stats;                       ///< 统计信息
    uint32_t _lastActivityTime;                 ///< 最后活动时间
    bool _energySaveActive;                     ///< 节能模式激活状态
    
    // 长按状态管理
    bool _longPressActive;                      ///< 长按状态是否激活
    uint8_t _longPressLED;                      ///< 长按的LED索引
    uint32_t _longPressStartTime;               ///< 长按开始时间

    // 场景配置
    static const uint8_t MAX_SCENES = 16;       ///< 最大场景数量
    FeedbackConfig _sceneConfigs[MAX_SCENES];   ///< 场景配置数组

    /**
     * @brief 初始化默认场景配置
     */
    void initializeDefaultScenes();

    /**
     * @brief 应用用户偏好到子系统
     */
    void applyUserPreferences();

    /**
     * @brief 计算调整后的配置（基于用户偏好）
     * @param baseConfig 基础配置
     * @param scene 场景类型
     * @return FeedbackConfig 调整后的配置
     */
    FeedbackConfig adjustConfigForPreferences(const FeedbackConfig& baseConfig, 
                                             FeedbackScene scene) const;

    /**
     * @brief 检查是否需要跳过反馈（节能模式等）
     * @param scene 场景类型
     * @param config 反馈配置
     * @return true 跳过，false 不跳过
     */
    bool shouldSkipFeedback(FeedbackScene scene, const FeedbackConfig& config) const;

    /**
     * @brief 更新活动时间
     */
    void updateActivity();

    /**
     * @brief 检查并应用节能模式
     */
    void checkEnergySaveMode();

    /**
     * @brief 获取场景索引
     * @param scene 场景枚举
     * @return uint8_t 场景索引
     */
    uint8_t getSceneIndex(FeedbackScene scene) const;

    /**
     * @brief 计算能耗（估算）
     * @param config 反馈配置
     * @return uint32_t 能耗值（任意单位）
     */
    uint32_t calculateEnergyUsage(const FeedbackConfig& config) const;

    /**
     * @brief 验证配置有效性
     * @param config 配置
     * @return true 有效，false 无效
     */
    bool validateConfig(const FeedbackConfig& config) const;

    /**
     * @brief 将按键编号转换为LED索引
     * @param keyIndex 按键编号 (1-22)
     * @return uint8_t LED索引 (0-21)，无效时返回255
     */
    uint8_t keyToLED(uint8_t keyIndex) const;
};

// 便捷宏定义
#define FEEDBACK_MGR FeedbackManager::getInstance()

// 快捷反馈宏（用于代码中的快速反馈调用）
#define FEEDBACK_KEY_PRESS(keyIndex) FEEDBACK_MGR.triggerKeyFeedback(keyIndex, true)
#define FEEDBACK_KEY_RELEASE(keyIndex) FEEDBACK_MGR.triggerKeyFeedback(keyIndex, false)
#define FEEDBACK_SUCCESS() FEEDBACK_MGR.triggerFeedback(SCENE_CALCULATION_SUCCESS)
#define FEEDBACK_ERROR() FEEDBACK_MGR.triggerFeedback(SCENE_CALCULATION_ERROR)
#define FEEDBACK_WARNING() FEEDBACK_MGR.triggerFeedback(SCENE_ERROR_WARNING)

#endif // FEEDBACK_MANAGER_H