/**
 * @file Logger.h
 * @brief 计算器项目日志系统
 * @details 基于ESP32-IDF日志库的统一日志管理系统
 * 
 * 提供功能：
 * - 多级别日志输出（ERROR, WARN, INFO, DEBUG, VERBOSE）
 * - 模块化标签管理
 * - 可配置的输出格式
 * - 串口和可选的文件输出
 * - 运行时日志级别调整
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "esp_log.h"
#include "config.h"

/**
 * @brief 日志级别枚举
 */
typedef enum {
    LOG_LEVEL_NONE = 0,    ///< 不输出日志
    LOG_LEVEL_ERROR,       ///< 错误级别
    LOG_LEVEL_WARN,        ///< 警告级别
    LOG_LEVEL_INFO,        ///< 信息级别
    LOG_LEVEL_DEBUG,       ///< 调试级别
    LOG_LEVEL_VERBOSE      ///< 详细级别
} log_level_t;

/**
 * @brief 日志输出目标枚举
 */
typedef enum {
    LOG_OUTPUT_SERIAL = 1,  ///< 串口输出
    LOG_OUTPUT_FILE = 2,    ///< 文件输出（预留）
    LOG_OUTPUT_BOTH = 3     ///< 同时输出到串口和文件
} log_output_t;

/**
 * @brief 日志系统配置结构体
 */
struct LoggerConfig {
    log_level_t level;              ///< 全局日志级别
    log_output_t output;            ///< 输出目标
    bool showTimestamp;             ///< 是否显示时间戳
    bool showLevel;                 ///< 是否显示级别标签
    bool showTag;                   ///< 是否显示模块标签
    bool colorOutput;               ///< 是否使用彩色输出
    uint32_t baudRate;              ///< 串口波特率
    size_t bufferSize;              ///< 日志缓冲区大小
};

/**
 * @brief 日志系统管理类
 */
class Logger {
public:
    /**
     * @brief 获取日志系统单例
     * @return Logger& 日志系统实例引用
     */
    static Logger& getInstance();

    /**
     * @brief 初始化日志系统
     * @param config 日志配置
     * @return true 初始化成功，false 初始化失败
     */
    bool begin(const LoggerConfig& config = getDefaultConfig());

    /**
     * @brief 设置全局日志级别
     * @param level 日志级别
     */
    void setLevel(log_level_t level);

    /**
     * @brief 设置特定标签的日志级别
     * @param tag 模块标签
     * @param level 日志级别
     */
    void setTagLevel(const char* tag, log_level_t level);

    /**
     * @brief 获取默认配置
     * @return LoggerConfig 默认配置
     */
    static LoggerConfig getDefaultConfig();

    /**
     * @brief 记录错误日志
     * @param tag 模块标签
     * @param format 格式化字符串
     * @param ... 可变参数
     */
    void error(const char* tag, const char* format, ...);

    /**
     * @brief 记录警告日志
     * @param tag 模块标签
     * @param format 格式化字符串
     * @param ... 可变参数
     */
    void warn(const char* tag, const char* format, ...);

    /**
     * @brief 记录信息日志
     * @param tag 模块标签
     * @param format 格式化字符串
     * @param ... 可变参数
     */
    void info(const char* tag, const char* format, ...);

    /**
     * @brief 记录调试日志
     * @param tag 模块标签
     * @param format 格式化字符串
     * @param ... 可变参数
     */
    void debug(const char* tag, const char* format, ...);

    /**
     * @brief 记录详细日志
     * @param tag 模块标签
     * @param format 格式化字符串
     * @param ... 可变参数
     */
    void verbose(const char* tag, const char* format, ...);

    /**
     * @brief 刷新日志缓冲区
     */
    void flush();

    /**
     * @brief 设置自定义日志输出函数
     * @param enable true使用自定义格式，false使用系统默认格式
     */
    void setCustomFormat(bool enable = true);

    /**
     * @brief 获取当前配置
     * @return const LoggerConfig& 当前配置引用
     */
    const LoggerConfig& getConfig() const { return _config; }

    /**
     * @brief 启用/禁用彩色输出
     * @param enable true启用，false禁用
     */
    void setColorOutput(bool enable);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LoggerConfig _config;           ///< 日志配置
    bool _initialized;              ///< 初始化状态
    bool _customFormat;             ///< 是否使用自定义格式
    static Logger* _instance;       ///< 单例实例指针（用于静态回调）

    /**
     * @brief 转换日志级别到ESP-IDF格式
     * @param level 自定义日志级别
     * @return esp_log_level_t ESP-IDF日志级别
     */
    esp_log_level_t toEspLogLevel(log_level_t level);

    /**
     * @brief 获取日志级别字符串
     * @param level 日志级别
     * @return const char* 级别字符串
     */
    const char* getLevelString(log_level_t level);

    /**
     * @brief 获取日志级别颜色代码
     * @param level 日志级别  
     * @return const char* ANSI颜色代码
     */
    const char* getLevelColor(log_level_t level);

    /**
     * @brief 自定义日志输出函数
     * @param format 格式化字符串
     * @param args 参数列表
     * @return int 输出字符数
     */
    static int customVprintf(const char* format, va_list args);

    /**
     * @brief 格式化并输出日志
     * @param level 日志级别
     * @param tag 标签
     * @param format 格式化字符串
     * @param args 参数列表
     */
    void formatAndPrint(log_level_t level, const char* tag, const char* format, va_list args);

    /**
     * @brief 获取日志级别缩写
     * @param level 日志级别
     * @return const char* 级别缩写（如 E, W, I, D, V）
     */
    const char* getLevelChar(log_level_t level);
};

// 模块标签定义
#define TAG_MAIN        "MAIN"
#define TAG_KEYPAD      "KEYPAD"
#define TAG_DISPLAY     "DISPLAY"
#define TAG_BACKLIGHT   "BACKLIGHT"
#define TAG_BATTERY     "BATTERY"
#define TAG_CALCULATOR  "CALC"
#define TAG_SYSTEM      "SYSTEM"
#define TAG_INIT        "INIT"
#define TAG_MODE        "MODE"
#define TAG_LVGL        "LVGL"

// 便捷宏定义
#define LOG_E(tag, format, ...) Logger::getInstance().error(tag, format, ##__VA_ARGS__)
#define LOG_W(tag, format, ...) Logger::getInstance().warn(tag, format, ##__VA_ARGS__)
#define LOG_I(tag, format, ...) Logger::getInstance().info(tag, format, ##__VA_ARGS__)
#define LOG_D(tag, format, ...) Logger::getInstance().debug(tag, format, ##__VA_ARGS__)
#define LOG_V(tag, format, ...) Logger::getInstance().verbose(tag, format, ##__VA_ARGS__)

// 模块专用日志宏
#define KEYPAD_LOG_E(format, ...) LOG_E(TAG_KEYPAD, format, ##__VA_ARGS__)
#define KEYPAD_LOG_W(format, ...) LOG_W(TAG_KEYPAD, format, ##__VA_ARGS__)
#define KEYPAD_LOG_I(format, ...) LOG_I(TAG_KEYPAD, format, ##__VA_ARGS__)
#define KEYPAD_LOG_D(format, ...) LOG_D(TAG_KEYPAD, format, ##__VA_ARGS__)
#define KEYPAD_LOG_V(format, ...) LOG_V(TAG_KEYPAD, format, ##__VA_ARGS__)

#define DISPLAY_LOG_E(format, ...) LOG_E(TAG_DISPLAY, format, ##__VA_ARGS__)
#define DISPLAY_LOG_W(format, ...) LOG_W(TAG_DISPLAY, format, ##__VA_ARGS__)
#define DISPLAY_LOG_I(format, ...) LOG_I(TAG_DISPLAY, format, ##__VA_ARGS__)
#define DISPLAY_LOG_D(format, ...) LOG_D(TAG_DISPLAY, format, ##__VA_ARGS__)
#define DISPLAY_LOG_V(format, ...) LOG_V(TAG_DISPLAY, format, ##__VA_ARGS__)

#define BATTERY_LOG_E(format, ...) LOG_E(TAG_BATTERY, format, ##__VA_ARGS__)
#define BATTERY_LOG_W(format, ...) LOG_W(TAG_BATTERY, format, ##__VA_ARGS__)
#define BATTERY_LOG_I(format, ...) LOG_I(TAG_BATTERY, format, ##__VA_ARGS__)
#define BATTERY_LOG_D(format, ...) LOG_D(TAG_BATTERY, format, ##__VA_ARGS__)
#define BATTERY_LOG_V(format, ...) LOG_V(TAG_BATTERY, format, ##__VA_ARGS__)

#define CALC_LOG_E(format, ...) LOG_E(TAG_CALCULATOR, format, ##__VA_ARGS__)
#define CALC_LOG_W(format, ...) LOG_W(TAG_CALCULATOR, format, ##__VA_ARGS__)
#define CALC_LOG_I(format, ...) LOG_I(TAG_CALCULATOR, format, ##__VA_ARGS__)
#define CALC_LOG_D(format, ...) LOG_D(TAG_CALCULATOR, format, ##__VA_ARGS__)
#define CALC_LOG_V(format, ...) LOG_V(TAG_CALCULATOR, format, ##__VA_ARGS__)

#define SYSTEM_LOG_E(format, ...) LOG_E(TAG_SYSTEM, format, ##__VA_ARGS__)
#define SYSTEM_LOG_W(format, ...) LOG_W(TAG_SYSTEM, format, ##__VA_ARGS__)
#define SYSTEM_LOG_I(format, ...) LOG_I(TAG_SYSTEM, format, ##__VA_ARGS__)
#define SYSTEM_LOG_D(format, ...) LOG_D(TAG_SYSTEM, format, ##__VA_ARGS__)
#define SYSTEM_LOG_V(format, ...) LOG_V(TAG_SYSTEM, format, ##__VA_ARGS__)

#define MODE_LOG_E(format, ...) LOG_E(TAG_MODE, format, ##__VA_ARGS__)
#define MODE_LOG_W(format, ...) LOG_W(TAG_MODE, format, ##__VA_ARGS__)
#define MODE_LOG_I(format, ...) LOG_I(TAG_MODE, format, ##__VA_ARGS__)
#define MODE_LOG_D(format, ...) LOG_D(TAG_MODE, format, ##__VA_ARGS__)
#define MODE_LOG_V(format, ...) LOG_V(TAG_MODE, format, ##__VA_ARGS__)

#define LVGL_LOG_E(format, ...) LOG_E(TAG_LVGL, format, ##__VA_ARGS__)
#define LVGL_LOG_W(format, ...) LOG_W(TAG_LVGL, format, ##__VA_ARGS__)
#define LVGL_LOG_I(format, ...) LOG_I(TAG_LVGL, format, ##__VA_ARGS__)
#define LVGL_LOG_D(format, ...) LOG_D(TAG_LVGL, format, ##__VA_ARGS__)
#define LVGL_LOG_V(format, ...) LOG_V(TAG_LVGL, format, ##__VA_ARGS__)

#endif // LOGGER_H