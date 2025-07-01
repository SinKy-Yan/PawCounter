#pragma once
#include <Arduino.h>

// 前向声明
class CalcDisplay;

/**
 * @brief 动画基类
 * @details P0阶段采用阻塞实现，为后续非阻塞重构预留接口
 */
class Animation {
public:
    /**
     * @brief 动画状态枚举
     */
    enum AnimationState {
        IDLE,           // 空闲状态
        PLAYING,        // 播放中
        COMPLETED,      // 已完成
        INTERRUPTED     // 被中断
    };

    /**
     * @brief 动画优先级枚举
     */
    enum AnimationPriority {
        PRIORITY_LOW = 0,        // 低优先级
        PRIORITY_NORMAL = 1,     // 普通优先级
        PRIORITY_HIGH = 2,       // 高优先级
        PRIORITY_CRITICAL = 3    // 关键优先级（如CLEAR动画）
    };

protected:
    CalcDisplay* _display;              // 显示对象引用
    AnimationState _state;              // 动画状态
    AnimationPriority _priority;        // 动画优先级
    unsigned long _startTime;           // 开始时间
    unsigned long _duration;            // 动画时长(ms)
    uint8_t _targetFPS;                 // 目标帧率

public:
    /**
     * @brief 构造函数
     * @param display 显示对象指针
     * @param duration 动画持续时间(ms)
     * @param priority 动画优先级
     * @param targetFPS 目标帧率
     */
    Animation(CalcDisplay* display, unsigned long duration = 200, 
              AnimationPriority priority = PRIORITY_NORMAL, uint8_t targetFPS = 15);

    virtual ~Animation() = default;

    /**
     * @brief 开始动画
     */
    virtual void start();

    /**
     * @brief 更新动画帧
     * @return true=继续播放, false=动画结束
     */
    virtual bool tick();

    /**
     * @brief 中断动画
     */
    virtual void interrupt();

    /**
     * @brief 获取动画状态
     */
    AnimationState getState() const { return _state; }

    /**
     * @brief 获取动画优先级
     */
    AnimationPriority getPriority() const { return _priority; }

    /**
     * @brief 获取动画进度 (0.0-1.0)
     */
    float getProgress() const;

protected:
    /**
     * @brief 纯虚函数：渲染动画帧
     * @param progress 动画进度 (0.0-1.0)
     */
    virtual void renderFrame(float progress) = 0;

    /**
     * @brief 缓动函数：线性插值
     */
    float easeLinear(float t);

    /**
     * @brief 缓动函数：缓入
     */
    float easeIn(float t);

    /**
     * @brief 缓动函数：缓出
     */
    float easeOut(float t);
};