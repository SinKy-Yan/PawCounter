#include "SleepManager.h"

void SleepManager::begin(uint32_t timeoutMs) {
    if (!_initialized) {
        _timeoutMs = timeoutMs;
        _lastActivity = millis();
        _state = State::ACTIVE;
        _initialized = true;
        
        LOG_I(TAG_SLEEP, "休眠管理器初始化完成，超时时间：%u ms", _timeoutMs);
    } else {
        _timeoutMs = timeoutMs;
        LOG_I(TAG_SLEEP, "休眠管理器超时时间已更新：%u ms", _timeoutMs);
    }
}

void SleepManager::update() {
    if (!_initialized) return;
    
    // 休眠禁用检查
    if (_timeoutMs == 0) return;
    
    uint32_t currentTime = millis();
    uint32_t inactivityTime = currentTime - _lastActivity;
    
    // 状态检查与切换
    if (_state == State::ACTIVE) {
        if (inactivityTime > _timeoutMs) {
            // 切换到休眠状态
            _state = State::SLEEPING;
            LOG_I(TAG_SLEEP, "系统进入休眠状态，不活动时间：%u ms", inactivityTime);
            _notifySleep();
        }
    } else if (_state == State::SLEEPING) {
        // 已在休眠状态，无需额外操作
    }
}

void SleepManager::feed() {
    _lastActivity = millis();
    
    // 如果处于休眠状态，唤醒
    if (_state == State::SLEEPING) {
        _state = State::ACTIVE;
        LOG_I(TAG_SLEEP, "系统唤醒");
        _notifyWake();
    }
}

void SleepManager::setTimeout(uint32_t timeoutMs) {
    if (_timeoutMs != timeoutMs) {
        _timeoutMs = timeoutMs;
        
        if (_timeoutMs == 0) {
            LOG_I(TAG_SLEEP, "休眠功能已禁用");
            // 如果当前处于休眠状态，则唤醒
            if (_state == State::SLEEPING) {
                _state = State::ACTIVE;
                _notifyWake();
            }
        } else {
            LOG_I(TAG_SLEEP, "休眠超时时间已设置为：%u ms", _timeoutMs);
        }
        
        // 重置活动计时器
        _lastActivity = millis();
    }
}

void SleepManager::setState(State state) {
    if (_state != state) {
        State prevState = _state;
        _state = state;
        
        // 处理状态变化
        if (prevState == State::ACTIVE && state == State::SLEEPING) {
            LOG_I(TAG_SLEEP, "系统手动切换到休眠状态");
            _notifySleep();
        } else if (prevState == State::SLEEPING && state == State::ACTIVE) {
            LOG_I(TAG_SLEEP, "系统手动切换到活动状态");
            _notifyWake();
        }
        
        // 重置活动计时器
        _lastActivity = millis();
    }
}

SleepManager::CallbackID SleepManager::addCallback(
    CallbackFunc onSleep, 
    CallbackFunc onWake, 
    void* context
) {
    if (_callbackCount >= MAX_CALLBACKS) {
        LOG_W(TAG_SLEEP, "无法添加回调：已达最大数量 (%d)", MAX_CALLBACKS);
        return 0xFF;  // 失败
    }
    
    // 寻找空闲槽位
    for (uint8_t i = 0; i < MAX_CALLBACKS; i++) {
        if (!_callbacks[i].active) {
            _callbacks[i].onSleepFunc = onSleep;
            _callbacks[i].onWakeFunc = onWake;
            _callbacks[i].context = context;
            _callbacks[i].active = true;
            _callbackCount++;
            
            LOG_D(TAG_SLEEP, "添加回调成功，ID：%d，总数：%d", i, _callbackCount);
            return i;  // 返回ID
        }
    }
    
    // 不应该到达这里
    LOG_W(TAG_SLEEP, "添加回调失败：未知错误");
    return 0xFF;
}

bool SleepManager::removeCallback(CallbackID id) {
    if (id >= MAX_CALLBACKS || !_callbacks[id].active) {
        LOG_W(TAG_SLEEP, "移除回调失败：无效ID %d", id);
        return false;
    }
    
    _callbacks[id].active = false;
    _callbacks[id].onSleepFunc = nullptr;
    _callbacks[id].onWakeFunc = nullptr;
    _callbacks[id].context = nullptr;
    _callbackCount--;
    
    LOG_D(TAG_SLEEP, "成功移除回调 ID：%d，剩余：%d", id, _callbackCount);
    return true;
}

void SleepManager::_notifySleep() {
    for (uint8_t i = 0; i < MAX_CALLBACKS; i++) {
        if (_callbacks[i].active && _callbacks[i].onSleepFunc) {
            _callbacks[i].onSleepFunc(_callbacks[i].context);
        }
    }
}

void SleepManager::_notifyWake() {
    for (uint8_t i = 0; i < MAX_CALLBACKS; i++) {
        if (_callbacks[i].active && _callbacks[i].onWakeFunc) {
            _callbacks[i].onWakeFunc(_callbacks[i].context);
        }
    }
} 