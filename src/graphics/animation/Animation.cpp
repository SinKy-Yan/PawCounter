#include "Animation.h"
#include "../../calc_display.h"

Animation::Animation(CalcDisplay* display, unsigned long duration, 
                     AnimationPriority priority, uint8_t targetFPS)
    : _display(display), _state(IDLE), _priority(priority), 
      _startTime(0), _duration(duration), _targetFPS(targetFPS) {
}

void Animation::start() {
    _state = PLAYING;
    _startTime = millis();
}

bool Animation::tick() {
    if (_state != PLAYING) {
        return false;
    }

    float progress = getProgress();
    
    if (progress >= 1.0f) {
        // 动画完成
        progress = 1.0f;
        _state = COMPLETED;
        renderFrame(progress);
        return false;
    }

    // 渲染当前帧
    renderFrame(progress);
    
    // P0阶段：简单延时控制帧率（阻塞实现）
    delay(1000 / _targetFPS);
    
    return true;
}

void Animation::interrupt() {
    if (_state == PLAYING) {
        _state = INTERRUPTED;
        // 立即跳到终态
        renderFrame(1.0f);
    }
}

float Animation::getProgress() const {
    if (_state != PLAYING || _duration == 0) {
        return (_state == COMPLETED) ? 1.0f : 0.0f;
    }

    unsigned long elapsed = millis() - _startTime;
    return min(1.0f, (float)elapsed / (float)_duration);
}

// 缓动函数实现
float Animation::easeLinear(float t) {
    return t;
}

float Animation::easeIn(float t) {
    return t * t;
}

float Animation::easeOut(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}