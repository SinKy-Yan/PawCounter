/**
 * @file DualCoreManager.h
 * @brief ESP32-S3双核管理器
 * @details 实现核心0和核心1的任务分离，优化性能和响应速度
 * 
 * 核心分工：
 * - 核心0: 实时硬件控制（键盘、显示、LED、计算器逻辑）
 * - 核心1: 后台服务（串口、日志、调试、网络通信）
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef DUAL_CORE_MANAGER_H
#define DUAL_CORE_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "Logger.h"

/**
 * @brief 核心间消息类型
 */
enum MessageType {
    MSG_LOG_OUTPUT,         ///< 日志输出消息
    MSG_SERIAL_COMMAND,     ///< 串口命令消息
    MSG_STATUS_UPDATE,      ///< 状态更新消息
    MSG_DISPLAY_UPDATE,     ///< 显示更新消息
    MSG_SYSTEM_INFO,        ///< 系统信息消息
    MSG_BATTERY_STATUS,     ///< 电池状态消息
    MSG_KEY_EVENT,          ///< 按键事件消息
    MSG_DEBUG_INFO          ///< 调试信息消息
};

/**
 * @brief 核心间消息结构
 */
struct CoreMessage {
    MessageType type;                    ///< 消息类型
    uint32_t timestamp;                  ///< 时间戳
    union {
        struct {
            LogLevel level;              ///< 日志级别
            char tag[16];               ///< 日志标签
            char message[256];          ///< 日志内容
        } log;
        
        struct {
            char command[64];           ///< 串口命令
            bool processed;             ///< 是否已处理
        } serial;
        
        struct {
            uint32_t uptime;            ///< 运行时间
            uint8_t backlight;          ///< 背光亮度
            char calculator_display[32]; ///< 计算器显示内容
            bool calculator_ready;      ///< 计算器就绪状态
        } status;
        
        struct {
            float voltage;              ///< 电池电压
            uint8_t percentage;         ///< 电池电量
            bool charging;              ///< 是否充电
            bool standby;               ///< 是否待机
        } battery;
        
        struct {
            uint8_t key;                ///< 按键编号
            uint8_t event;              ///< 事件类型
            uint32_t press_time;        ///< 按下时间
        } key_event;
        
        uint8_t raw_data[256];          ///< 原始数据
    } data;
};

/**
 * @brief 双核管理器类
 */
class DualCoreManager {
public:
    /**
     * @brief 获取单例实例
     */
    static DualCoreManager& getInstance();
    
    /**
     * @brief 初始化双核管理器
     * @return true 成功，false 失败
     */
    bool begin();
    
    /**
     * @brief 发送消息到核心1
     * @param message 消息结构
     * @param timeout_ms 超时时间（毫秒）
     * @return true 成功，false 失败
     */
    bool sendToCore1(const CoreMessage& message, uint32_t timeout_ms = 100);
    
    /**
     * @brief 发送消息到核心0
     * @param message 消息结构
     * @param timeout_ms 超时时间（毫秒）
     * @return true 成功，false 失败
     */
    bool sendToCore0(const CoreMessage& message, uint32_t timeout_ms = 100);
    
    /**
     * @brief 创建日志消息
     * @param level 日志级别
     * @param tag 日志标签
     * @param message 日志内容
     * @return CoreMessage 消息结构
     */
    CoreMessage createLogMessage(LogLevel level, const char* tag, const char* message);
    
    /**
     * @brief 创建状态更新消息
     * @param uptime 运行时间
     * @param backlight 背光亮度
     * @param calculator_display 计算器显示
     * @param calculator_ready 计算器就绪状态
     * @return CoreMessage 消息结构
     */
    CoreMessage createStatusMessage(uint32_t uptime, uint8_t backlight, 
                                   const char* calculator_display, bool calculator_ready);
    
    /**
     * @brief 创建电池状态消息
     * @param voltage 电池电压
     * @param percentage 电池电量
     * @param charging 是否充电
     * @param standby 是否待机
     * @return CoreMessage 消息结构
     */
    CoreMessage createBatteryMessage(float voltage, uint8_t percentage, 
                                    bool charging, bool standby);
    
    /**
     * @brief 创建按键事件消息
     * @param key 按键编号
     * @param event 事件类型
     * @param press_time 按下时间
     * @return CoreMessage 消息结构
     */
    CoreMessage createKeyEventMessage(uint8_t key, uint8_t event, uint32_t press_time);
    
    /**
     * @brief 获取当前运行的核心ID
     * @return 0或1
     */
    uint8_t getCurrentCore() const;
    
    /**
     * @brief 获取队列中待处理消息数量
     * @param to_core1 true查询发往核心1的队列，false查询发往核心0的队列
     * @return 消息数量
     */
    uint32_t getQueueCount(bool to_core1) const;
    
    /**
     * @brief 获取系统统计信息
     */
    void printStats();

private:
    DualCoreManager() = default;
    ~DualCoreManager() = default;
    DualCoreManager(const DualCoreManager&) = delete;
    DualCoreManager& operator=(const DualCoreManager&) = delete;
    
    /**
     * @brief 核心0任务入口
     */
    static void core0TaskEntry(void* parameter);
    
    /**
     * @brief 核心1任务入口
     */
    static void core1TaskEntry(void* parameter);
    
    /**
     * @brief 核心0主循环
     */
    void core0Loop();
    
    /**
     * @brief 核心1主循环
     */
    void core1Loop();
    
    // FreeRTOS对象
    QueueHandle_t _queueToCore1;        ///< 发往核心1的消息队列
    QueueHandle_t _queueToCore0;        ///< 发往核心0的消息队列
    TaskHandle_t _core0Task;            ///< 核心0任务句柄
    TaskHandle_t _core1Task;            ///< 核心1任务句柄
    SemaphoreHandle_t _serialMutex;     ///< 串口互斥锁
    SemaphoreHandle_t _displayMutex;    ///< 显示互斥锁
    
    // 状态变量
    bool _initialized;                  ///< 是否已初始化
    uint32_t _core0Messages;           ///< 核心0处理的消息数
    uint32_t _core1Messages;           ///< 核心1处理的消息数
    uint32_t _lastStatsTime;           ///< 上次统计时间
    
    // 任务配置
    static const uint32_t QUEUE_SIZE = 50;              ///< 队列大小
    static const uint32_t CORE0_STACK_SIZE = 8192;      ///< 核心0栈大小
    static const uint32_t CORE1_STACK_SIZE = 16384;     ///< 核心1栈大小
    static const uint8_t CORE0_PRIORITY = 2;            ///< 核心0优先级
    static const uint8_t CORE1_PRIORITY = 1;            ///< 核心1优先级
};

// 便捷宏定义
#define DUAL_CORE DualCoreManager::getInstance()

// 跨核心日志宏
#define CORE_LOG_E(tag, format, ...) do { \
    if (DUAL_CORE.getCurrentCore() == 0) { \
        char msg[256]; \
        snprintf(msg, sizeof(msg), format, ##__VA_ARGS__); \
        auto logMsg = DUAL_CORE.createLogMessage(LOG_LEVEL_ERROR, tag, msg); \
        DUAL_CORE.sendToCore1(logMsg); \
    } else { \
        LOG_E(tag, format, ##__VA_ARGS__); \
    } \
} while(0)

#define CORE_LOG_W(tag, format, ...) do { \
    if (DUAL_CORE.getCurrentCore() == 0) { \
        char msg[256]; \
        snprintf(msg, sizeof(msg), format, ##__VA_ARGS__); \
        auto logMsg = DUAL_CORE.createLogMessage(LOG_LEVEL_WARN, tag, msg); \
        DUAL_CORE.sendToCore1(logMsg); \
    } else { \
        LOG_W(tag, format, ##__VA_ARGS__); \
    } \
} while(0)

#define CORE_LOG_I(tag, format, ...) do { \
    if (DUAL_CORE.getCurrentCore() == 0) { \
        char msg[256]; \
        snprintf(msg, sizeof(msg), format, ##__VA_ARGS__); \
        auto logMsg = DUAL_CORE.createLogMessage(LOG_LEVEL_INFO, tag, msg); \
        DUAL_CORE.sendToCore1(logMsg); \
    } else { \
        LOG_I(tag, format, ##__VA_ARGS__); \
    } \
} while(0)

#define CORE_LOG_D(tag, format, ...) do { \
    if (DUAL_CORE.getCurrentCore() == 0) { \
        char msg[256]; \
        snprintf(msg, sizeof(msg), format, ##__VA_ARGS__); \
        auto logMsg = DUAL_CORE.createLogMessage(LOG_LEVEL_DEBUG, tag, msg); \
        DUAL_CORE.sendToCore1(logMsg); \
    } else { \
        LOG_D(tag, format, ##__VA_ARGS__); \
    } \
} while(0)

#define CORE_LOG_V(tag, format, ...) do { \
    if (DUAL_CORE.getCurrentCore() == 0) { \
        char msg[256]; \
        snprintf(msg, sizeof(msg), format, ##__VA_ARGS__); \
        auto logMsg = DUAL_CORE.createLogMessage(LOG_LEVEL_VERBOSE, tag, msg); \
        DUAL_CORE.sendToCore1(logMsg); \
    } else { \
        LOG_V(tag, format, ##__VA_ARGS__); \
    } \
} while(0)

#endif // DUAL_CORE_MANAGER_H