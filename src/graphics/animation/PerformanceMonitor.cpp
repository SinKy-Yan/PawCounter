#include "PerformanceMonitor.h"
#include "../../Logger.h"
#include "../../config.h"
#include <algorithm>
#include <esp_system.h>

#ifdef ENABLE_BATTERY_MANAGER
#include "../../BatteryManager.h"
#endif

#define TAG_PERF_MON "PerfMon"

PerformanceMonitor::PerformanceMonitor(float targetFPS, unsigned long updateInterval)
    : _lastUpdateTime(0), _updateInterval(updateInterval), _frameCount(0), 
      _fpsWindowStart(0), _currentFPS(0.0f), _averageFPS(0.0f), 
      _frameStartTime(0), _currentFrameTime(0), _maxFrameTime(0), _totalFrameTime(0),
      _busyTime(0), _idleTime(0), _cpuUsage(0.0f), _currentLevel(PERFORMANCE_HIGH),
      _lastLevelCheck(0), _levelDowngradeCount(0), _levelUpgradeCount(0),
      _targetFPS(targetFPS), _minAcceptableFPS(targetFPS * 0.8f),
      _maxAcceptableFrameTime(1000 / targetFPS * 1.5f), _minBatteryLevel(PERF_LOW_BATTERY_THRESHOLD),
      _minFreeHeap(MIN_FREE_HEAP_KB) {
    
    reset();
    LOG_I(TAG_PERF_MON, "PerformanceMonitor initialized: targetFPS=%.1f, updateInterval=%lu ms", 
          targetFPS, updateInterval);
}

void PerformanceMonitor::beginFrame() {
    _frameStartTime = millis();
}

void PerformanceMonitor::endFrame() {
    if (_frameStartTime > 0) {
        _currentFrameTime = millis() - _frameStartTime;
        _totalFrameTime += _currentFrameTime;
        
        if (_currentFrameTime > _maxFrameTime) {
            _maxFrameTime = _currentFrameTime;
        }
        
        _frameCount++;
        _frameStartTime = 0;  // 重置
    }
}

void PerformanceMonitor::update(uint8_t activeAnimations) {
    unsigned long currentTime = millis();
    
    // 检查是否到更新时间
    if (currentTime - _lastUpdateTime < _updateInterval) {
        return;
    }
    
    _lastUpdateTime = currentTime;
    
    // 更新FPS统计
    updateFPSStats();
    
    // 更新CPU使用率
    updateCPUUsage();
    
    // 更新性能等级
    updatePerformanceLevel(activeAnimations);
    
    // 定期打印性能报告（每10秒）
    static unsigned long lastReportTime = 0;
    if (currentTime - lastReportTime > 10000) {
        printReport();
        lastReportTime = currentTime;
    }
}

PerformanceMonitor::PerformanceMetrics PerformanceMonitor::getMetrics() const {
    PerformanceMetrics metrics;
    metrics.currentFPS = _currentFPS;
    metrics.averageFPS = _averageFPS;
    metrics.frameTime = _currentFrameTime;
    metrics.maxFrameTime = _maxFrameTime;
    metrics.cpuUsage = _cpuUsage;
    metrics.batteryLevel = getBatteryLevel();
    metrics.freeHeap = getFreeHeapKB();
    metrics.activeAnimations = 0;  // 注意：需要调用者通过update()传入实际值
    metrics.level = _currentLevel;
    return metrics;
}

PerformanceMonitor::PerformanceMetrics PerformanceMonitor::getMetrics(uint8_t activeAnimations) const {
    PerformanceMetrics metrics = getMetrics();
    metrics.activeAnimations = activeAnimations;  // 设置实际的活跃动画数
    return metrics;
}

bool PerformanceMonitor::shouldReduceAnimations() const {
    return _currentLevel >= PERFORMANCE_MEDIUM;
}

bool PerformanceMonitor::shouldReduceFrameRate() const {
    return _currentLevel >= PERFORMANCE_LOW;
}

bool PerformanceMonitor::shouldDisableAnimations() const {
    return _currentLevel >= PERFORMANCE_CRITICAL;
}

uint8_t PerformanceMonitor::getRecommendedFPS() const {
    switch (_currentLevel) {
        case PERFORMANCE_HIGH:
            return _targetFPS;
        case PERFORMANCE_MEDIUM:
            return _targetFPS * 0.8f;
        case PERFORMANCE_LOW:
            return _targetFPS * 0.5f;
        case PERFORMANCE_CRITICAL:
            return _targetFPS * 0.3f;
        default:
            return _targetFPS;
    }
}

uint8_t PerformanceMonitor::getRecommendedMaxAnimations() const {
    switch (_currentLevel) {
        case PERFORMANCE_HIGH:
            return 3;
        case PERFORMANCE_MEDIUM:
            return 2;
        case PERFORMANCE_LOW:
            return 1;
        case PERFORMANCE_CRITICAL:
            return 0;
        default:
            return 3;
    }
}

void PerformanceMonitor::setTargetFPS(float fps) {
    if (fps > 0 && fps <= 60) {
        _targetFPS = fps;
        _minAcceptableFPS = fps * 0.8f;
        _maxAcceptableFrameTime = 1000 / fps * 1.5f;
        LOG_I(TAG_PERF_MON, "Target FPS changed to %.1f", fps);
    }
}

void PerformanceMonitor::reset() {
    _frameCount = 0;
    _fpsWindowStart = millis();
    _currentFPS = 0.0f;
    _averageFPS = 0.0f;
    _currentFrameTime = 0;
    _maxFrameTime = 0;
    _totalFrameTime = 0;
    _busyTime = 0;
    _idleTime = 0;
    _cpuUsage = 0.0f;
    _currentLevel = PERFORMANCE_HIGH;
    _levelDowngradeCount = 0;
    _levelUpgradeCount = 0;
    
    LOG_I(TAG_PERF_MON, "Performance monitor reset");
}

void PerformanceMonitor::printReport() const {
    LOG_I(TAG_PERF_MON, "=== Performance Report ===");
    LOG_I(TAG_PERF_MON, "FPS: current=%.1f, average=%.1f, target=%.1f", 
          _currentFPS, _averageFPS, _targetFPS);
    LOG_I(TAG_PERF_MON, "Frame Time: current=%lu ms, max=%lu ms, limit=%lu ms", 
          _currentFrameTime, _maxFrameTime, _maxAcceptableFrameTime);
    LOG_I(TAG_PERF_MON, "CPU Usage: %.1f%%", _cpuUsage);
    LOG_I(TAG_PERF_MON, "Battery: %d%%, Free Heap: %dKB", 
          getBatteryLevel(), getFreeHeapKB());
    LOG_I(TAG_PERF_MON, "Performance Level: %d", _currentLevel);
    LOG_I(TAG_PERF_MON, "Recommendations: FPS=%d, MaxAnims=%d", 
          getRecommendedFPS(), getRecommendedMaxAnimations());
}

void PerformanceMonitor::updateFPSStats() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - _fpsWindowStart;
    
    if (elapsed > 0) {
        _currentFPS = (float)_frameCount * 1000.0f / elapsed;
        
        // 更新平均FPS（使用指数移动平均）
        if (_averageFPS == 0.0f) {
            _averageFPS = _currentFPS;
        } else {
            _averageFPS = _averageFPS * 0.9f + _currentFPS * 0.1f;
        }
        
        // 重置计数窗口
        if (elapsed > 5000) {  // 每5秒重置一次
            _frameCount = 0;
            _fpsWindowStart = currentTime;
        }
    }
}

void PerformanceMonitor::updateCPUUsage() {
    // 简单的CPU使用率估算
    // 基于帧时间和帧间隔的比例
    if (_frameCount > 0) {
        unsigned long averageFrameTime = _totalFrameTime / _frameCount;
        unsigned long frameInterval = 1000 / _targetFPS;
        
        _cpuUsage = (float)averageFrameTime / frameInterval * 100.0f;
        _cpuUsage = constrain(_cpuUsage, 0.0f, 100.0f);
    }
}

void PerformanceMonitor::updatePerformanceLevel(uint8_t activeAnimations) {
    unsigned long currentTime = millis();
    
    // 每2秒检查一次性能等级
    if (currentTime - _lastLevelCheck < 2000) {
        return;
    }
    
    _lastLevelCheck = currentTime;
    
    // 计算降级/升级因子
    bool shouldDowngrade = false;
    bool shouldUpgrade = false;
    
    // 检查各项指标
    if (_currentFPS < _minAcceptableFPS) {
        shouldDowngrade = true;
    }
    
    if (_currentFrameTime > _maxAcceptableFrameTime) {
        shouldDowngrade = true;
    }
    
    if (_cpuUsage > 80.0f) {
        shouldDowngrade = true;
    }
    
    if (getBatteryLevel() < _minBatteryLevel) {
        shouldDowngrade = true;
    }
    
    if (getFreeHeapKB() < _minFreeHeap) {
        shouldDowngrade = true;
    }
    
    // 检查是否可以升级
    if (_currentFPS > _targetFPS * 0.95f && 
        _currentFrameTime < _maxAcceptableFrameTime * 0.8f &&
        _cpuUsage < 60.0f) {
        shouldUpgrade = true;
    }
    
    // 更新计数器
    if (shouldDowngrade) {
        _levelDowngradeCount++;
        _levelUpgradeCount = 0;
    } else if (shouldUpgrade) {
        _levelUpgradeCount++;
        _levelDowngradeCount = 0;
    } else {
        _levelDowngradeCount = 0;
        _levelUpgradeCount = 0;
    }
    
    // 需要连续3次才改变等级（防止频繁变化）
    if (_levelDowngradeCount >= 3 && _currentLevel < PERFORMANCE_CRITICAL) {
        _currentLevel = (PerformanceLevel)(_currentLevel + 1);
        _levelDowngradeCount = 0;
        LOG_W(TAG_PERF_MON, "Performance level downgraded to %d", _currentLevel);
    } else if (_levelUpgradeCount >= 3 && _currentLevel > PERFORMANCE_HIGH) {
        _currentLevel = (PerformanceLevel)(_currentLevel - 1);
        _levelUpgradeCount = 0;
        LOG_I(TAG_PERF_MON, "Performance level upgraded to %d", _currentLevel);
    }
}

uint8_t PerformanceMonitor::getBatteryLevel() const {
#ifdef ENABLE_BATTERY_MANAGER
    // 如果启用了电池管理，获取真实电量
    if (BatteryManager::getInstance()) {
        return BatteryManager::getInstance()->getBatteryPercentage();
    }
#endif
    // 否则返回假设的电量值
    return 100;  // 假设电量充足
}

uint16_t PerformanceMonitor::getFreeHeapKB() const {
    return ESP.getFreeHeap() / 1024;
}