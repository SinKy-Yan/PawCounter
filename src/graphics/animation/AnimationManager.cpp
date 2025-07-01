#include "AnimationManager.h"
#include "../../Logger.h"
#include <algorithm>

#define TAG_ANIM_MGR "AnimMgr"

AnimationManager::AnimationManager(uint8_t maxConcurrent, uint8_t targetFPS)
    : _state(IDLE), _maxConcurrentAnimations(maxConcurrent), 
      _lastTickTime(0), _targetFPS(targetFPS), _frameStartTime(0), 
      _lastFrameTime(0), _frameCount(0), _averageFPS(0.0f) {
    
    // 计算帧间隔
    _frameInterval = 1000 / _targetFPS;
    
    // 预分配向量空间
    _activeAnimations.reserve(maxConcurrent);
    _pendingAnimations.reserve(maxConcurrent * 2);
    
    LOG_I(TAG_ANIM_MGR, "AnimationManager initialized: maxConcurrent=%d, targetFPS=%d", 
          maxConcurrent, targetFPS);
}

AnimationManager::~AnimationManager() {
    // 清理所有动画
    interruptAllAnimations();
    
    // 清理等待队列
    for (Animation* anim : _pendingAnimations) {
        delete anim;
    }
    _pendingAnimations.clear();
    
    LOG_I(TAG_ANIM_MGR, "AnimationManager destroyed");
}

bool AnimationManager::addAnimation(Animation* animation, bool autoStart) {
    if (!animation) {
        LOG_E(TAG_ANIM_MGR, "Cannot add null animation");
        return false;
    }

    // 检查是否可以直接添加到活跃列表
    if (_activeAnimations.size() < _maxConcurrentAnimations) {
        _activeAnimations.push_back(animation);
        
        if (autoStart) {
            animation->start();
        }
        
        // 更新管理器状态
        _state = ACTIVE;
        if (_activeAnimations.size() >= _maxConcurrentAnimations) {
            _state = BUSY;
        }
        
        LOG_D(TAG_ANIM_MGR, "Animation added to active list (priority=%d, active=%d/%d)", 
              animation->getPriority(), _activeAnimations.size(), _maxConcurrentAnimations);
        return true;
    }

    // 检查优先级是否足够高，可以抢占低优先级动画
    Animation::AnimationPriority newPriority = animation->getPriority();
    Animation* lowestPriorityAnim = nullptr;
    Animation::AnimationPriority lowestPriority = Animation::PRIORITY_CRITICAL;

    for (Animation* anim : _activeAnimations) {
        if (anim->getPriority() < lowestPriority) {
            lowestPriority = anim->getPriority();
            lowestPriorityAnim = anim;
        }
    }

    // 如果新动画优先级更高，可以抢占
    if (newPriority > lowestPriority) {
        // 中断低优先级动画
        lowestPriorityAnim->interrupt();
        
        // 从活跃列表中移除
        auto it = std::find(_activeAnimations.begin(), _activeAnimations.end(), lowestPriorityAnim);
        if (it != _activeAnimations.end()) {
            _activeAnimations.erase(it);
            delete lowestPriorityAnim;  // 释放内存
        }
        
        // 添加新动画
        _activeAnimations.push_back(animation);
        if (autoStart) {
            animation->start();
        }
        
        LOG_I(TAG_ANIM_MGR, "Animation preempted (new priority=%d > old priority=%d)", 
              newPriority, lowestPriority);
        return true;
    }

    // 添加到等待队列
    _pendingAnimations.push_back(animation);
    
    // 按优先级排序等待队列
    std::sort(_pendingAnimations.begin(), _pendingAnimations.end(), compareAnimationPriority);
    
    LOG_D(TAG_ANIM_MGR, "Animation added to waiting queue (priority=%d, pending=%d)", 
          newPriority, _pendingAnimations.size());
    return true;
}

void AnimationManager::interruptAnimations(Animation::AnimationPriority minPriority) {
    auto it = _activeAnimations.begin();
    while (it != _activeAnimations.end()) {
        Animation* anim = *it;
        if (anim->getPriority() < minPriority) {
            anim->interrupt();
            delete anim;
            it = _activeAnimations.erase(it);
            LOG_D(TAG_ANIM_MGR, "Animation interrupted (priority=%d < %d)", 
                  anim->getPriority(), minPriority);
        } else {
            ++it;
        }
    }
    
    // 更新状态
    if (_activeAnimations.empty()) {
        _state = IDLE;
    } else if (_activeAnimations.size() < _maxConcurrentAnimations) {
        _state = ACTIVE;
    }
}

void AnimationManager::interruptAllAnimations() {
    // 中断所有活跃动画
    for (Animation* anim : _activeAnimations) {
        anim->interrupt();
        delete anim;
    }
    _activeAnimations.clear();
    
    _state = IDLE;
    LOG_I(TAG_ANIM_MGR, "All animations interrupted");
}

uint8_t AnimationManager::tick() {
    unsigned long currentTime = millis();
    
    // 帧率控制 - 非阻塞实现
    if (currentTime - _lastTickTime < _frameInterval) {
        return _activeAnimations.size();  // 还没到下一帧时间
    }
    
    _frameStartTime = currentTime;
    _lastTickTime = currentTime;

    // 如果没有活跃动画，直接返回
    if (_activeAnimations.empty()) {
        _state = IDLE;
        processWaitingQueue();  // 处理等待队列
        return 0;
    }

    // 更新所有活跃动画
    auto it = _activeAnimations.begin();
    while (it != _activeAnimations.end()) {
        Animation* anim = *it;
        
        // 调用动画的tick方法
        bool continueAnimation = anim->tick();
        
        if (!continueAnimation) {
            // 动画结束，清理资源
            delete anim;
            it = _activeAnimations.erase(it);
            LOG_D(TAG_ANIM_MGR, "Animation completed and removed");
        } else {
            ++it;
        }
    }

    // 处理等待队列
    processWaitingQueue();

    // 更新管理器状态
    if (_activeAnimations.empty()) {
        _state = IDLE;
    } else if (_activeAnimations.size() < _maxConcurrentAnimations) {
        _state = ACTIVE;
    } else {
        _state = BUSY;
    }

    // 更新性能统计
    updatePerformanceStats();

    return _activeAnimations.size();
}

void AnimationManager::setTargetFPS(uint8_t fps) {
    if (fps > 0 && fps <= 60) {
        _targetFPS = fps;
        _frameInterval = 1000 / fps;
        LOG_I(TAG_ANIM_MGR, "Target FPS changed to %d (interval=%lu ms)", fps, _frameInterval);
    }
}

void AnimationManager::setMaxConcurrentAnimations(uint8_t maxConcurrent) {
    if (maxConcurrent > 0) {
        _maxConcurrentAnimations = maxConcurrent;
        
        // 如果当前活跃动画超过新限制，中断低优先级的
        while (_activeAnimations.size() > maxConcurrent) {
            Animation* lowestPriorityAnim = nullptr;
            Animation::AnimationPriority lowestPriority = Animation::PRIORITY_CRITICAL;
            
            for (Animation* anim : _activeAnimations) {
                if (anim->getPriority() <= lowestPriority) {
                    lowestPriority = anim->getPriority();
                    lowestPriorityAnim = anim;
                }
            }
            
            if (lowestPriorityAnim) {
                lowestPriorityAnim->interrupt();
                auto it = std::find(_activeAnimations.begin(), _activeAnimations.end(), lowestPriorityAnim);
                if (it != _activeAnimations.end()) {
                    _activeAnimations.erase(it);
                    delete lowestPriorityAnim;
                }
            }
        }
        
        LOG_I(TAG_ANIM_MGR, "Max concurrent animations changed to %d", maxConcurrent);
    }
}

bool AnimationManager::hasAnimationType(const char* animationType) const {
    // 这里需要根据实际动画类型实现
    // P1阶段暂时简单返回是否有活跃动画
    return !_activeAnimations.empty();
}

void AnimationManager::processWaitingQueue() {
    // 处理等待队列中的动画
    while (!_pendingAnimations.empty() && _activeAnimations.size() < _maxConcurrentAnimations) {
        Animation* nextAnim = _pendingAnimations.front();
        _pendingAnimations.erase(_pendingAnimations.begin());
        
        _activeAnimations.push_back(nextAnim);
        nextAnim->start();
        
        LOG_D(TAG_ANIM_MGR, "Animation moved from pending to active (priority=%d)", 
              nextAnim->getPriority());
    }
}

void AnimationManager::cleanupCompletedAnimations() {
    // 清理已完成的动画（在tick中已经处理）
}

bool AnimationManager::compareAnimationPriority(Animation* a, Animation* b) {
    return a->getPriority() > b->getPriority();  // 高优先级在前面
}

void AnimationManager::updatePerformanceStats() {
    unsigned long frameEndTime = millis();
    _lastFrameTime = frameEndTime - _frameStartTime;
    _frameCount++;
    
    // 每100帧计算一次平均FPS
    if (_frameCount % 100 == 0) {
        static unsigned long lastStatsTime = 0;
        unsigned long currentTime = frameEndTime;
        
        if (lastStatsTime > 0) {
            unsigned long elapsed = currentTime - lastStatsTime;
            _averageFPS = 100000.0f / elapsed;  // 100帧的时间间隔
        }
        
        lastStatsTime = currentTime;
        
        LOG_V(TAG_ANIM_MGR, "Performance stats: avgFPS=%.1f, lastFrameTime=%lu ms, active=%d", 
              _averageFPS, _lastFrameTime, _activeAnimations.size());
    }
}