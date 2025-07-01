#pragma once
#include <Arduino.h>

/**
 * @brief 性能监控模块
 * @details P1阶段实现：FPS统计、CPU使用率估算、电池电量监控、自动降级决策
 */
class PerformanceMonitor {
public:
    /**
     * @brief 性能等级
     */
    enum PerformanceLevel {
        PERFORMANCE_HIGH = 0,       // 高性能：满帧率、全特效
        PERFORMANCE_MEDIUM = 1,     // 中等性能：降低帧率
        PERFORMANCE_LOW = 2,        // 低性能：最小特效
        PERFORMANCE_CRITICAL = 3    // 危险状态：禁用动画
    };

    /**
     * @brief 性能指标结构
     */
    struct PerformanceMetrics {
        float currentFPS;           // 当前帧率
        float averageFPS;           // 平均帧率
        unsigned long frameTime;    // 当前帧耗时(ms)
        unsigned long maxFrameTime; // 最大帧耗时(ms)
        float cpuUsage;             // CPU使用率估算(%)
        uint8_t batteryLevel;       // 电池电量(%)
        uint16_t freeHeap;          // 可用堆内存(KB)
        uint8_t activeAnimations;   // 活跃动画数量
        PerformanceLevel level;     // 当前性能等级
    };

private:
    // 性能监控参数
    unsigned long _lastUpdateTime;          // 上次更新时间
    unsigned long _updateInterval;          // 更新间隔(ms)
    
    // FPS统计
    uint16_t _frameCount;                   // 帧计数器
    unsigned long _fpsWindowStart;          // FPS统计窗口开始时间
    float _currentFPS;                      // 当前FPS
    float _averageFPS;                      // 平均FPS
    
    // 帧时间统计
    unsigned long _frameStartTime;          // 当前帧开始时间
    unsigned long _currentFrameTime;        // 当前帧耗时
    unsigned long _maxFrameTime;            // 最大帧耗时
    unsigned long _totalFrameTime;          // 总帧时间（用于平均计算）
    
    // CPU使用率估算
    unsigned long _busyTime;                // 忙碌时间
    unsigned long _idleTime;                // 空闲时间
    float _cpuUsage;                        // CPU使用率
    
    // 性能等级
    PerformanceLevel _currentLevel;         // 当前性能等级
    unsigned long _lastLevelCheck;          // 上次等级检查时间
    uint8_t _levelDowngradeCount;          // 降级计数器
    uint8_t _levelUpgradeCount;            // 升级计数器
    
    // 配置参数
    float _targetFPS;                       // 目标FPS
    float _minAcceptableFPS;               // 最低可接受FPS
    unsigned long _maxAcceptableFrameTime; // 最大可接受帧时间
    uint8_t _minBatteryLevel;              // 最低电池电量阈值
    uint16_t _minFreeHeap;                 // 最低可用内存阈值

public:
    /**
     * @brief 构造函数
     * @param targetFPS 目标帧率
     * @param updateInterval 监控更新间隔(ms)
     */
    PerformanceMonitor(float targetFPS = 20.0f, unsigned long updateInterval = 1000);

    /**
     * @brief 开始帧性能监控
     */
    void beginFrame();

    /**
     * @brief 结束帧性能监控
     */
    void endFrame();

    /**
     * @brief 更新性能监控数据
     * @param activeAnimations 当前活跃动画数量
     */
    void update(uint8_t activeAnimations = 0);

    /**
     * @brief 获取当前性能指标
     */
    PerformanceMetrics getMetrics() const;
    
    /**
     * @brief 获取当前性能指标（包含活跃动画数）
     * @param activeAnimations 当前活跃动画数量
     */
    PerformanceMetrics getMetrics(uint8_t activeAnimations) const;

    /**
     * @brief 获取当前性能等级
     */
    PerformanceLevel getPerformanceLevel() const { return _currentLevel; }

    /**
     * @brief 是否应该降低动画质量
     */
    bool shouldReduceAnimations() const;

    /**
     * @brief 是否应该降低帧率
     */
    bool shouldReduceFrameRate() const;

    /**
     * @brief 是否应该禁用动画
     */
    bool shouldDisableAnimations() const;

    /**
     * @brief 获取建议的目标帧率
     */
    uint8_t getRecommendedFPS() const;

    /**
     * @brief 获取建议的最大并发动画数
     */
    uint8_t getRecommendedMaxAnimations() const;

    /**
     * @brief 设置目标FPS
     */
    void setTargetFPS(float fps);

    /**
     * @brief 重置性能统计
     */
    void reset();

    /**
     * @brief 打印性能报告到串口
     */
    void printReport() const;

private:
    /**
     * @brief 更新FPS统计
     */
    void updateFPSStats();

    /**
     * @brief 更新CPU使用率估算
     */
    void updateCPUUsage();

    /**
     * @brief 评估并更新性能等级
     */
    void updatePerformanceLevel(uint8_t activeAnimations);

    /**
     * @brief 获取电池电量
     */
    uint8_t getBatteryLevel() const;

    /**
     * @brief 获取可用堆内存(KB)
     */
    uint16_t getFreeHeapKB() const;
};