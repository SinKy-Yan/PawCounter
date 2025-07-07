#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>
#include "Logger.h"

#define TAG_SLEEP "SLEEP"

/**
 * 休眠管理器 - 管理系统自动休眠功能
 * 
 * 检测用户活动并在指定时间后自动进入休眠状态
 * 通过回调机制通知其他模块系统休眠/唤醒
 */
class SleepManager {
public:
    // 系统状态枚举
    enum class State {
        ACTIVE,     // 活跃状态
        SLEEPING    // 休眠状态
    };
    
    // 回调ID类型
    using CallbackID = uint8_t;
    
    // 标准回调类型：void function(void* context)
    using CallbackFunc = void (*)(void*);
    
    /**
     * 获取单例实例
     */
    static SleepManager& instance() {
        static SleepManager instance;
        return instance;
    }
    
    /**
     * 初始化休眠管理器
     * @param timeoutMs 无活动后进入休眠的时间(毫秒)，0表示禁用休眠
     */
    void begin(uint32_t timeoutMs = 10000);
    
    /**
     * 更新休眠状态 - 在主循环中调用
     */
    void update();
    
    /**
     * 喂狗 - 记录用户活动，重置休眠计时器
     */
    void feed();
    
    /**
     * 设置休眠超时时间
     * @param timeoutMs 超时时间(毫秒)，0表示禁用休眠
     */
    void setTimeout(uint32_t timeoutMs);
    
    /**
     * 获取当前休眠超时时间
     * @return 超时时间(毫秒)
     */
    uint32_t getTimeout() const { return _timeoutMs; }
    
    /**
     * 获取当前系统状态
     * @return 当前状态(ACTIVE/SLEEPING)
     */
    State getState() const { return _state; }
    
    /**
     * 直接设置系统状态(用于强制休眠/唤醒)
     * @param state 目标状态
     */
    void setState(State state);
    
    /**
     * 添加休眠/唤醒回调函数
     * @param onSleep 进入休眠时调用的函数
     * @param onWake 唤醒时调用的函数
     * @param context 回调上下文(可选)
     * @return 回调ID，用于后续移除；失败返回0xFF
     */
    CallbackID addCallback(CallbackFunc onSleep, CallbackFunc onWake, void* context = nullptr);
    
    /**
     * 移除回调
     * @param id 要移除的回调ID
     * @return 是否成功移除
     */
    bool removeCallback(CallbackID id);

private:
    // 私有构造函数(单例)
    SleepManager() : 
        _initialized(false), 
        _timeoutMs(10000), 
        _lastActivity(0), 
        _state(State::ACTIVE), 
        _callbackCount(0) {
        // 初始化回调数组
        for (uint8_t i = 0; i < MAX_CALLBACKS; i++) {
            _callbacks[i].active = false;
        }
    }
    
    // 回调结构定义
    struct SleepCallback {
        CallbackFunc onSleepFunc;
        CallbackFunc onWakeFunc;
        void* context;
        bool active;
    };
    
    static const uint8_t MAX_CALLBACKS = 8;  // 最大支持的回调数量
    
    bool _initialized;           // 是否已初始化
    uint32_t _timeoutMs;         // 休眠超时时间(毫秒)
    uint32_t _lastActivity;      // 最后活动时间
    State _state;                // 当前状态
    
    SleepCallback _callbacks[MAX_CALLBACKS];  // 回调数组
    uint8_t _callbackCount;                   // 当前活动回调数量
    
    // 执行所有休眠回调
    void _notifySleep();
    
    // 执行所有唤醒回调
    void _notifyWake();
};

#endif // SLEEP_MANAGER_H 