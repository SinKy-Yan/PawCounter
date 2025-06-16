/**
 * @file KeyboardConfig.h
 * @brief 键盘配置管理系统
 * @details 支持双层键位配置和Flash持久化存储
 * 
 * 特性：
 * - 双层键位系统（主层和第二层）
 * - Tab键切换层级（短按切换，长按自定义功能）
 * - Flash持久化配置存储
 * - 可自定义的按键映射
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef KEYBOARD_CONFIG_H
#define KEYBOARD_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include <memory>
#include "Logger.h"

/**
 * @brief 按键层级枚举
 */
enum class KeyLayer {
    PRIMARY = 0,        ///< 主层（默认层）
    SECONDARY,          ///< 第二层（Tab切换后的层）
    MAX_LAYERS
};

/**
 * @brief 按键类型枚举
 */
enum class KeyType {
    NUMBER = 0,         ///< 数字键 (0-9)
    OPERATOR,           ///< 运算符 (+, -, *, /)
    FUNCTION,           ///< 功能键 (=, C, CE, ±)
    DECIMAL,            ///< 小数点
    MODE_SWITCH,        ///< 模式切换
    LAYER_SWITCH,       ///< 层级切换（Tab键）
    CLEAR,              ///< 清除
    DELETE,             ///< 向前删除
    MEMORY,             ///< 内存操作
    POWER,              ///< 电源相关
    RESERVED            ///< 保留功能
};

/**
 * @brief 运算符类型
 */
enum class Operator {
    NONE = 0,
    ADD,                ///< 加法
    SUBTRACT,           ///< 减法
    MULTIPLY,           ///< 乘法
    DIVIDE,             ///< 除法
    EQUALS,             ///< 等号
    PERCENT,            ///< 百分比
    SQUARE_ROOT,        ///< 平方根
    SQUARE,             ///< 平方
    RECIPROCAL          ///< 倒数
};

/**
 * @brief 单个按键配置
 */
struct KeyConfig {
    uint8_t position;           ///< 物理按键位置 (1-22)
    KeyType type;               ///< 按键类型
    String symbol;              ///< 显示符号
    String label;               ///< 按键标签
    Operator operation;         ///< 对应的运算操作
    String functionName;        ///< 自定义函数名（仅当type为FUNCTION时使用）
    uint16_t keyCode;          ///< 键码（用于BLE等）
    bool isEnabled;            ///< 是否启用
};

/**
 * @brief 键盘层配置
 */
struct LayerConfig {
    KeyLayer layer;                     ///< 层级类型
    String name;                        ///< 层级名称
    String description;                 ///< 层级描述
    std::vector<KeyConfig> keys;        ///< 按键配置列表
    bool isActive;                      ///< 是否为当前活动层
};

/**
 * @brief 完整的键盘配置
 */
struct KeyboardLayoutConfig {
    String name;                        ///< 布局名称
    String version;                     ///< 版本号
    uint32_t checksum;                  ///< 配置校验和
    std::vector<LayerConfig> layers;    ///< 层级配置列表
    KeyLayer defaultLayer;              ///< 默认层级
    uint8_t tabKeyPosition;            ///< Tab键位置
    uint32_t layerSwitchTimeout;       ///< 层级切换超时时间(ms)
};

/**
 * @brief Tab键行为配置
 */
struct TabBehaviorConfig {
    uint16_t shortPressThreshold;       ///< 短按阈值(ms)
    uint16_t longPressThreshold;        ///< 长按阈值(ms)
    bool enableAutoReturn;              ///< 是否自动返回主层
    uint32_t autoReturnTimeout;         ///< 自动返回超时时间(ms)
    
    // 长按功能回调接口
    std::function<void()> onLongPress;  ///< 长按回调函数
    std::function<void()> onDoublePress; ///< 双击回调函数
};

/**
 * @brief 键盘配置管理器
 */
class KeyboardConfigManager {
public:
    /**
     * @brief 构造函数
     */
    KeyboardConfigManager();
    
    /**
     * @brief 析构函数
     */
    ~KeyboardConfigManager();
    
    /**
     * @brief 初始化配置管理器
     * @return true 成功，false 失败
     */
    bool begin();
    
    /**
     * @brief 加载配置
     * @param forceDefault 是否强制使用默认配置
     * @return true 成功，false 失败
     */
    bool loadConfig(bool forceDefault = false);
    
    /**
     * @brief 保存配置到Flash
     * @return true 成功，false 失败
     */
    bool saveConfig();
    
    /**
     * @brief 重置为默认配置
     * @return true 成功，false 失败
     */
    bool resetToDefault();
    
    /**
     * @brief 获取当前活动层级
     * @return 当前层级
     */
    KeyLayer getCurrentLayer() const { return _currentLayer; }
    
    /**
     * @brief 切换到指定层级
     * @param layer 目标层级
     * @return true 成功，false 失败
     */
    bool switchToLayer(KeyLayer layer);
    
    /**
     * @brief 处理Tab键按下事件
     * @param isLongPress 是否为长按
     * @return true 处理成功，false 处理失败
     */
    bool handleTabKey(bool isLongPress = false);
    
    /**
     * @brief 获取指定位置的按键配置
     * @param position 按键位置
     * @param layer 指定层级（默认为当前层级）
     * @return 按键配置指针，未找到返回nullptr
     */
    const KeyConfig* getKeyConfig(uint8_t position, KeyLayer layer = KeyLayer::PRIMARY) const;
    
    /**
     * @brief 修改按键配置
     * @param position 按键位置
     * @param config 新的按键配置
     * @param layer 指定层级
     * @return true 成功，false 失败
     */
    bool setKeyConfig(uint8_t position, const KeyConfig& config, KeyLayer layer = KeyLayer::PRIMARY);
    
    /**
     * @brief 获取完整的键盘布局配置
     * @return 键盘布局配置
     */
    const KeyboardLayoutConfig& getLayoutConfig() const { return _layoutConfig; }
    
    /**
     * @brief 设置Tab键行为配置
     * @param config Tab行为配置
     */
    void setTabBehavior(const TabBehaviorConfig& config) { _tabBehavior = config; }
    
    /**
     * @brief 获取Tab键行为配置
     * @return Tab行为配置
     */
    const TabBehaviorConfig& getTabBehavior() const { return _tabBehavior; }
    
    /**
     * @brief 验证配置完整性
     * @return true 配置有效，false 配置无效
     */
    bool validateConfig() const;
    
    /**
     * @brief 打印当前配置信息
     */
    void printConfig() const;
    
    /**
     * @brief 获取配置版本
     * @return 版本字符串
     */
    String getConfigVersion() const { return _layoutConfig.version; }

private:
    Preferences _preferences;                   ///< ESP32 Preferences存储
    KeyboardLayoutConfig _layoutConfig;         ///< 键盘布局配置
    TabBehaviorConfig _tabBehavior;            ///< Tab键行为配置
    KeyLayer _currentLayer;                     ///< 当前活动层级
    uint32_t _lastLayerSwitchTime;             ///< 上次层级切换时间
    
    static const char* PREF_NAMESPACE;          ///< Preferences命名空间
    static const char* PREF_CONFIG_KEY;         ///< 配置键名
    static const char* PREF_VERSION_KEY;        ///< 版本键名
    static const char* PREF_CHECKSUM_KEY;       ///< 校验和键名
    
    /**
     * @brief 创建默认配置
     * @return 默认键盘布局配置
     */
    KeyboardLayoutConfig createDefaultConfig();
    
    /**
     * @brief 创建主层配置
     * @return 主层配置
     */
    LayerConfig createPrimaryLayer();
    
    /**
     * @brief 创建第二层配置
     * @return 第二层配置
     */
    LayerConfig createSecondaryLayer();
    
    /**
     * @brief 计算配置校验和
     * @return 校验和值
     */
    uint32_t calculateChecksum() const;
    
    /**
     * @brief 序列化配置为字节数组
     * @param buffer 输出缓冲区
     * @param maxSize 缓冲区最大大小
     * @return 实际使用的字节数，失败返回0
     */
    size_t serializeConfig(uint8_t* buffer, size_t maxSize) const;
    
    /**
     * @brief 从字节数组反序列化配置
     * @param buffer 输入缓冲区
     * @param size 缓冲区大小
     * @return true 成功，false 失败
     */
    bool deserializeConfig(const uint8_t* buffer, size_t size);
    
    /**
     * @brief 查找层级配置
     * @param layer 层级类型
     * @return 层级配置指针，未找到返回nullptr
     */
    LayerConfig* findLayerConfig(KeyLayer layer);
    
    /**
     * @brief 查找层级配置（常量版本）
     * @param layer 层级类型
     * @return 层级配置指针，未找到返回nullptr
     */
    const LayerConfig* findLayerConfig(KeyLayer layer) const;
    
    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param format 格式字符串
     * @param ... 参数
     */
    void logMessage(const char* level, const char* format, ...) const;
};

// 全局实例声明
extern KeyboardConfigManager keyboardConfig;

// 便捷的日志宏
#define KEYBOARD_LOG_E(format, ...) keyboardConfig.logMessage("E", format, ##__VA_ARGS__)
#define KEYBOARD_LOG_W(format, ...) keyboardConfig.logMessage("W", format, ##__VA_ARGS__)
#define KEYBOARD_LOG_I(format, ...) keyboardConfig.logMessage("I", format, ##__VA_ARGS__)
#define KEYBOARD_LOG_D(format, ...) keyboardConfig.logMessage("D", format, ##__VA_ARGS__)
#define KEYBOARD_LOG_V(format, ...) keyboardConfig.logMessage("V", format, ##__VA_ARGS__)

#endif // KEYBOARD_CONFIG_H