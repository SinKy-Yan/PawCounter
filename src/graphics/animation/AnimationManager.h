#pragma once
#include <Arduino.h>
#include <vector>
#include "Animation.h"

/**
 * @brief 动画管理器
 * @details P1阶段实现：非阻塞动画管理、优先级队列、并发控制
 */
class AnimationManager {
public:
    /**
     * @brief 动画管理器状态
     */
    enum ManagerState {
        IDLE,           // 空闲状态
        ACTIVE,         // 有活跃动画
        BUSY            // 忙碌状态（达到最大并发数）
    };

private:
    std::vector<Animation*> _activeAnimations;      // 活跃动画列表
    std::vector<Animation*> _pendingAnimations;     // 等待队列
    
    ManagerState _state;                            // 管理器状态
    uint8_t _maxConcurrentAnimations;              // 最大并发动画数
    unsigned long _lastTickTime;                    // 上次tick时间
    uint8_t _targetFPS;                            // 目标帧率
    unsigned long _frameInterval;                   // 帧间隔(ms)
    
    // 性能统计
    unsigned long _frameStartTime;                  // 帧开始时间
    unsigned long _lastFrameTime;                   // 上一帧耗时
    uint16_t _frameCount;                          // 帧计数
    float _averageFPS;                             // 平均帧率

public:
    /**
     * @brief 构造函数
     * @param maxConcurrent 最大并发动画数
     * @param targetFPS 目标帧率
     */
    AnimationManager(uint8_t maxConcurrent = 3, uint8_t targetFPS = 20);
    
    /**
     * @brief 析构函数
     */
    ~AnimationManager();

    /**
     * @brief 添加动画到管理器
     * @param animation 动画对象指针
     * @param autoStart 是否自动开始
     * @return true=成功添加, false=失败（队列满或优先级不足）
     */
    bool addAnimation(Animation* animation, bool autoStart = true);

    /**
     * @brief 中断指定优先级以下的动画
     * @param minPriority 最小保留优先级
     */
    void interruptAnimations(Animation::AnimationPriority minPriority);

    /**
     * @brief 中断所有动画
     */
    void interruptAllAnimations();

    /**
     * @brief 非阻塞tick更新
     * @return 当前活跃动画数量
     */
    uint8_t tick();

    /**
     * @brief 获取管理器状态
     */
    ManagerState getState() const { return _state; }

    /**
     * @brief 获取活跃动画数量
     */
    uint8_t getActiveAnimationCount() const { return _activeAnimations.size(); }

    /**
     * @brief 获取等待队列长度
     */
    uint8_t getPendingAnimationCount() const { return _pendingAnimations.size(); }

    /**
     * @brief 获取平均帧率
     */
    float getAverageFPS() const { return _averageFPS; }

    /**
     * @brief 获取上一帧耗时(ms)
     */
    unsigned long getLastFrameTime() const { return _lastFrameTime; }

    /**
     * @brief 设置目标帧率
     * @param fps 目标帧率
     */
    void setTargetFPS(uint8_t fps);

    /**
     * @brief 设置最大并发动画数
     * @param maxConcurrent 最大并发数
     */
    void setMaxConcurrentAnimations(uint8_t maxConcurrent);

    /**
     * @brief 检查是否有指定类型的动画在播放
     * @param animationType 动画类型名（用于调试）
     */
    bool hasAnimationType(const char* animationType) const;

private:
    /**
     * @brief 处理等待队列
     */
    void processWaitingQueue();

    /**
     * @brief 清理已完成的动画
     */
    void cleanupCompletedAnimations();

    /**
     * @brief 按优先级排序动画
     * @param a 动画A
     * @param b 动画B
     * @return true=A优先级高于B
     */
    static bool compareAnimationPriority(Animation* a, Animation* b);

    /**
     * @brief 更新性能统计
     */
    void updatePerformanceStats();
};