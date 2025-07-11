#include "UIOptimizer.h"
#include "Logger.h"
#include <algorithm>

#define TAG_UI_OPT "UIOptimizer"

// 静态实例指针
UIOptimizer* UIOptimizer::_instance = nullptr;
UIOptimizer* g_uiOptimizer = nullptr;
UICacheManager* g_uiCache = nullptr;
FrameRateController* g_frameController = nullptr;

// 性能预设配置
const UIOptimizer::PerformanceConfig UIOptimizer::PERFORMANCE_LOW_MEMORY = {
    .refresh_rate_limit = 30,        // 30 FPS
    .buffer_optimization = 3,        // 最高优化
    .enable_partial_refresh = true,
    .enable_vsync = false,
    .idle_timeout = 2000,           // 2秒
    .enable_auto_sleep = true,
    .animation_speed = 3,           // 较慢动画
    .enable_shadow_cache = false
};

const UIOptimizer::PerformanceConfig UIOptimizer::PERFORMANCE_SPEED = {
    .refresh_rate_limit = 60,        // 60 FPS
    .buffer_optimization = 1,        // 轻度优化
    .enable_partial_refresh = true,
    .enable_vsync = true,
    .idle_timeout = 5000,           // 5秒
    .enable_auto_sleep = false,
    .animation_speed = 8,           // 较快动画
    .enable_shadow_cache = true
};

const UIOptimizer::PerformanceConfig UIOptimizer::PERFORMANCE_QUALITY = {
    .refresh_rate_limit = 60,        // 60 FPS
    .buffer_optimization = 0,        // 无优化
    .enable_partial_refresh = false,
    .enable_vsync = true,
    .idle_timeout = 10000,          // 10秒
    .enable_auto_sleep = false,
    .animation_speed = 10,          // 最快动画
    .enable_shadow_cache = true
};

const UIOptimizer::PerformanceConfig UIOptimizer::PERFORMANCE_BALANCED = {
    .refresh_rate_limit = 45,        // 45 FPS
    .buffer_optimization = 2,        // 中度优化
    .enable_partial_refresh = true,
    .enable_vsync = true,
    .idle_timeout = 3000,           // 3秒
    .enable_auto_sleep = true,
    .animation_speed = 6,           // 中等动画
    .enable_shadow_cache = true
};

UIOptimizer::UIOptimizer() 
    : _frame_start_time(0), _last_stats_update(0), _frame_time_index(0),
      _memory_limit(64 * 1024), _current_memory_usage(0), _last_gc_time(0),
      _power_save_mode(false), _last_activity_time(0), _idle_start_time(0),
      _batch_updates_enabled(true), _last_flush_time(0),
      _animations_paused(false), _animation_speed_scale(1.0f),
      _debug_enabled(false), _debug_frame_count(0) {
    
    _instance = this;
    g_uiOptimizer = this;
    
    // 初始化配置为平衡模式
    _config = PERFORMANCE_BALANCED;
    
    // 初始化统计信息
    memset(&_stats, 0, sizeof(_stats));
    
    // 预分配帧时间缓冲区
    _frame_times.resize(60); // 保存最近60帧的时间
}

UIOptimizer::~UIOptimizer() {
    _instance = nullptr;
    g_uiOptimizer = nullptr;
}

bool UIOptimizer::initialize() {
    LOG_I(TAG_UI_OPT, "初始化UI性能优化器...");
    
    // 重置统计信息
    resetStats();
    
    // 启动内存监控
    _last_activity_time = millis();
    
    LOG_I(TAG_UI_OPT, "UI优化器初始化完成");
    LOG_I(TAG_UI_OPT, "目标帧率: %d FPS", _config.refresh_rate_limit);
    LOG_I(TAG_UI_OPT, "内存限制: %d KB", _memory_limit / 1024);
    
    return true;
}

void UIOptimizer::setPerformanceProfile(const PerformanceConfig& config) {
    _config = config;
    
    LOG_I(TAG_UI_OPT, "切换性能配置:");
    LOG_I(TAG_UI_OPT, "  帧率限制: %d FPS", config.refresh_rate_limit);
    LOG_I(TAG_UI_OPT, "  缓冲区优化: %d", config.buffer_optimization);
    LOG_I(TAG_UI_OPT, "  部分刷新: %s", config.enable_partial_refresh ? "开启" : "关闭");
    LOG_I(TAG_UI_OPT, "  自动休眠: %s", config.enable_auto_sleep ? "开启" : "关闭");
}

void UIOptimizer::startFrameTimer() {
    _frame_start_time = micros();
}

void UIOptimizer::endFrameTimer() {
    if (_frame_start_time == 0) return;
    
    uint32_t frame_time = micros() - _frame_start_time;
    
    // 更新帧时间统计
    _frame_times[_frame_time_index] = frame_time;
    _frame_time_index = (_frame_time_index + 1) % _frame_times.size();
    
    // 更新统计信息
    _stats.frame_count++;
    if (frame_time > _stats.max_frame_time) {
        _stats.max_frame_time = frame_time;
    }
    
    // 每秒更新一次平均值
    uint32_t now = millis();
    if (now - _last_stats_update >= 1000) {
        updateStats();
        _last_stats_update = now;
    }
    
    _frame_start_time = 0;
}

void UIOptimizer::updateStats() {
    // 计算平均帧时间
    uint32_t total_time = 0;
    size_t valid_frames = 0;
    
    for (uint32_t time : _frame_times) {
        if (time > 0) {
            total_time += time;
            valid_frames++;
        }
    }
    
    if (valid_frames > 0) {
        _stats.avg_frame_time = total_time / valid_frames;
    }
    
    // 计算CPU使用率（简化估算）
    if (_stats.avg_frame_time > 0) {
        uint32_t target_frame_time = 1000000 / _config.refresh_rate_limit; // 微秒
        _stats.cpu_usage = (float)_stats.avg_frame_time / target_frame_time * 100.0f;
        if (_stats.cpu_usage > 100.0f) _stats.cpu_usage = 100.0f;
    }
    
    // 更新内存使用量
    _stats.memory_usage = getMemoryUsage();
    _stats.last_update_time = millis();
    
    // 检查内存使用情况
    checkMemoryUsage();
    
    // 检查空闲超时
    if (_config.enable_auto_sleep) {
        checkIdleTimeout();
    }
}

void UIOptimizer::optimizeForLowMemory() {
    setPerformanceProfile(PERFORMANCE_LOW_MEMORY);
    
    // 强制垃圾回收
    garbageCollect();
    
    // 清理缓存
    if (g_uiCache) {
        g_uiCache->cleanup();
    }
    
    LOG_I(TAG_UI_OPT, "已优化为低内存模式");
}

void UIOptimizer::optimizeForSpeed() {
    setPerformanceProfile(PERFORMANCE_SPEED);
    
    // 预加载常用资源
    if (g_uiCache) {
        // 这里可以预加载常用图像等资源
    }
    
    LOG_I(TAG_UI_OPT, "已优化为高速模式");
}

void UIOptimizer::optimizeForQuality() {
    setPerformanceProfile(PERFORMANCE_QUALITY);
    LOG_I(TAG_UI_OPT, "已优化为高质量模式");
}

void UIOptimizer::enableBatchUpdates(bool enable) {
    _batch_updates_enabled = enable;
    LOG_I(TAG_UI_OPT, "批量更新: %s", enable ? "开启" : "关闭");
}

void UIOptimizer::garbageCollect() {
    uint32_t start_time = millis();
    size_t memory_before = getMemoryUsage();
    
    // 执行LVGL垃圾回收 (LVGL 8.x中已移除)
    // lv_gc_collect(); // 在LVGL 8.x中已移除此API
    
    // 清理过期的脏区域
    uint32_t now = millis();
    _dirty_regions.erase(
        std::remove_if(_dirty_regions.begin(), _dirty_regions.end(),
            [now](const lv_area_t& area) {
                // 移除超过100ms的脏区域（简化逻辑）
                return false; // 实际实现中需要时间戳
            }),
        _dirty_regions.end()
    );
    
    size_t memory_after = getMemoryUsage();
    uint32_t gc_time = millis() - start_time;
    
    LOG_I(TAG_UI_OPT, "垃圾回收完成: 耗时%dms, 释放%d字节", 
          gc_time, memory_before - memory_after);
    
    _last_gc_time = millis();
}

size_t UIOptimizer::getMemoryUsage() {
    // 简化的内存使用量估算
    // 实际实现中应该统计LVGL内存池使用情况
    return _current_memory_usage;
}

void UIOptimizer::setMemoryLimit(size_t limit) {
    _memory_limit = limit;
    LOG_I(TAG_UI_OPT, "设置内存限制: %d KB", limit / 1024);
}

void UIOptimizer::enterPowerSaveMode() {
    if (_power_save_mode) return;
    
    _power_save_mode = true;
    
    // 降低刷新率
    PerformanceConfig power_config = _config;
    power_config.refresh_rate_limit = 15; // 15 FPS
    power_config.animation_speed = 2;      // 很慢的动画
    setPerformanceProfile(power_config);
    
    // 暂停动画
    pauseAnimations();
    
    LOG_I(TAG_UI_OPT, "进入节能模式");
}

void UIOptimizer::exitPowerSaveMode() {
    if (!_power_save_mode) return;
    
    _power_save_mode = false;
    
    // 恢复正常配置
    setPerformanceProfile(PERFORMANCE_BALANCED);
    
    // 恢复动画
    resumeAnimations();
    
    _last_activity_time = millis();
    
    LOG_I(TAG_UI_OPT, "退出节能模式");
}

void UIOptimizer::markDirtyRegion(const lv_area_t& area) {
    if (!_config.enable_partial_refresh) return;
    
    _dirty_regions.push_back(area);
    _stats.invalid_area_count++;
    
    // 限制脏区域数量
    if (_dirty_regions.size() > 50) {
        optimizeInvalidAreasInternal();
    }
}

void UIOptimizer::optimizeInvalidAreas() {
    optimizeInvalidAreasInternal();
}

void UIOptimizer::optimizeInvalidAreasInternal() {
    if (_dirty_regions.empty()) return;
    
    size_t original_count = _dirty_regions.size();
    
    // 合并重叠区域
    mergeDirtyRegions();
    
    // 移除过小的区域
    _dirty_regions.erase(
        std::remove_if(_dirty_regions.begin(), _dirty_regions.end(),
            [](const lv_area_t& area) {
                int32_t width = area.x2 - area.x1 + 1;
                int32_t height = area.y2 - area.y1 + 1;
                return width * height < 100; // 移除小于100像素的区域
            }),
        _dirty_regions.end()
    );
    
    if (_debug_enabled) {
        LOG_D(TAG_UI_OPT, "优化脏区域: %d -> %d", original_count, _dirty_regions.size());
    }
}

void UIOptimizer::mergeDirtyRegions() {
    if (_dirty_regions.size() < 2) return;
    
    bool merged = true;
    while (merged && _dirty_regions.size() > 1) {
        merged = false;
        
        for (size_t i = 0; i < _dirty_regions.size() - 1; i++) {
            for (size_t j = i + 1; j < _dirty_regions.size(); j++) {
                if (areAreasOverlapping(_dirty_regions[i], _dirty_regions[j])) {
                    _dirty_regions[i] = mergeAreas(_dirty_regions[i], _dirty_regions[j]);
                    _dirty_regions.erase(_dirty_regions.begin() + j);
                    merged = true;
                    break;
                }
            }
            if (merged) break;
        }
    }
}

bool UIOptimizer::areAreasOverlapping(const lv_area_t& a, const lv_area_t& b) {
    return !(a.x2 < b.x1 || b.x2 < a.x1 || a.y2 < b.y1 || b.y2 < a.y1);
}

lv_area_t UIOptimizer::mergeAreas(const lv_area_t& a, const lv_area_t& b) {
    lv_area_t merged;
    merged.x1 = LV_MIN(a.x1, b.x1);
    merged.y1 = LV_MIN(a.y1, b.y1);
    merged.x2 = LV_MAX(a.x2, b.x2);
    merged.y2 = LV_MAX(a.y2, b.y2);
    return merged;
}

void UIOptimizer::flushDirtyRegions() {
    if (_dirty_regions.empty()) return;
    
    uint32_t now = millis();
    
    // 如果启用批量更新，检查是否到了刷新时间
    if (_batch_updates_enabled) {
        uint32_t flush_interval = 1000 / _config.refresh_rate_limit;
        if (now - _last_flush_time < flush_interval) {
            return; // 还没到刷新时间
        }
    }
    
    // 执行刷新
    for (const auto& area : _dirty_regions) {
        lv_obj_invalidate_area(lv_scr_act(), &area);
    }
    
    _dirty_regions.clear();
    _last_flush_time = now;
}

void UIOptimizer::pauseAnimations() {
    if (_animations_paused) return;
    
    _animations_paused = true;
    // 这里应该暂停所有LVGL动画
    // lv_anim_pause_all(); // 如果LVGL版本支持
    
    LOG_D(TAG_UI_OPT, "暂停动画");
}

void UIOptimizer::resumeAnimations() {
    if (!_animations_paused) return;
    
    _animations_paused = false;
    // 这里应该恢复所有LVGL动画
    // lv_anim_resume_all(); // 如果LVGL版本支持
    
    LOG_D(TAG_UI_OPT, "恢复动画");
}

void UIOptimizer::setAnimationSpeedScale(float scale) {
    _animation_speed_scale = scale;
    LOG_D(TAG_UI_OPT, "设置动画速度比例: %.2f", scale);
}

void UIOptimizer::checkMemoryUsage() {
    if (_stats.memory_usage > _memory_limit * 0.9f) { // 90%阈值
        LOG_W(TAG_UI_OPT, "内存使用量过高: %d/%d bytes", 
              _stats.memory_usage, _memory_limit);
        
        // 自动垃圾回收
        if (millis() - _last_gc_time > 1000) { // 至少间隔1秒
            garbageCollect();
        }
        
        // 如果内存仍然不足，切换到低内存模式
        if (_stats.memory_usage > _memory_limit * 0.95f) {
            optimizeForLowMemory();
        }
    }
}

void UIOptimizer::checkIdleTimeout() {
    uint32_t now = millis();
    
    if (now - _last_activity_time > _config.idle_timeout) {
        if (!_power_save_mode) {
            _idle_start_time = now;
            enterPowerSaveMode();
        }
    }
}

void UIOptimizer::resetStats() {
    memset(&_stats, 0, sizeof(_stats));
    _stats.last_update_time = millis();
    
    // 重置帧时间缓冲区
    std::fill(_frame_times.begin(), _frame_times.end(), 0);
    _frame_time_index = 0;
    
    LOG_D(TAG_UI_OPT, "重置性能统计");
}

void UIOptimizer::enableDebugInfo(bool enable) {
    _debug_enabled = enable;
    LOG_I(TAG_UI_OPT, "调试信息: %s", enable ? "开启" : "关闭");
}

void UIOptimizer::printPerformanceReport() {
    LOG_I(TAG_UI_OPT, "=== 性能报告 ===");
    LOG_I(TAG_UI_OPT, "总帧数: %d", _stats.frame_count);
    LOG_I(TAG_UI_OPT, "平均帧时间: %d μs", _stats.avg_frame_time);
    LOG_I(TAG_UI_OPT, "最大帧时间: %d μs", _stats.max_frame_time);
    LOG_I(TAG_UI_OPT, "CPU使用率: %.1f%%", _stats.cpu_usage);
    LOG_I(TAG_UI_OPT, "内存使用: %d bytes", _stats.memory_usage);
    LOG_I(TAG_UI_OPT, "无效区域数: %d", _stats.invalid_area_count);
    LOG_I(TAG_UI_OPT, "节能模式: %s", _power_save_mode ? "开启" : "关闭");
    
    if (_stats.avg_frame_time > 0) {
        uint32_t actual_fps = 1000000 / _stats.avg_frame_time;
        LOG_I(TAG_UI_OPT, "实际帧率: %d FPS (目标: %d FPS)", 
              actual_fps, _config.refresh_rate_limit);
    }
}

String UIOptimizer::getPerformanceReport() {
    String report = "性能报告:\n";
    report += "帧数: " + String(_stats.frame_count) + "\n";
    report += "平均帧时间: " + String(_stats.avg_frame_time) + " μs\n";
    report += "CPU使用率: " + String(_stats.cpu_usage, 1) + "%\n";
    report += "内存使用: " + String(_stats.memory_usage) + " bytes\n";
    
    if (_stats.avg_frame_time > 0) {
        uint32_t actual_fps = 1000000 / _stats.avg_frame_time;
        report += "实际帧率: " + String(actual_fps) + " FPS\n";
    }
    
    return report;
}

// ===================== UICacheManager 实现 =====================

UICacheManager::UICacheManager() 
    : _max_cache_size(32 * 1024), _current_cache_size(0), 
      _cache_hits(0), _cache_misses(0) {
    g_uiCache = this;
}

UICacheManager::~UICacheManager() {
    clearCache();
    g_uiCache = nullptr;
}

bool UICacheManager::initialize(size_t max_cache_size) {
    _max_cache_size = max_cache_size;
    LOG_I(TAG_UI_OPT, "初始化UI缓存管理器，最大缓存: %d KB", max_cache_size / 1024);
    return true;
}

void UICacheManager::clearCache() {
    for (auto& item : _cache_items) {
        if (item.image_cache) {
            // 释放图像缓存内存
            free((void*)item.image_cache);
        }
    }
    
    _cache_items.clear();
    _current_cache_size = 0;
    
    LOG_D(TAG_UI_OPT, "清空UI缓存");
}

void UICacheManager::cleanup() {
    uint32_t now = millis();
    size_t removed_count = 0;
    
    // 移除超过5分钟未访问的项目
    _cache_items.erase(
        std::remove_if(_cache_items.begin(), _cache_items.end(),
            [now, &removed_count](const CacheItem& item) {
                if (now - item.last_access_time > 300000) { // 5分钟
                    removed_count++;
                    return true;
                }
                return false;
            }),
        _cache_items.end()
    );
    
    if (removed_count > 0) {
        LOG_D(TAG_UI_OPT, "清理了 %d 个过期缓存项", removed_count);
    }
}

size_t UICacheManager::getCacheMemoryUsage() {
    return _current_cache_size;
}

void UICacheManager::setMaxCacheSize(size_t size) {
    _max_cache_size = size;
    
    // 如果当前缓存超过新限制，进行清理
    while (_current_cache_size > _max_cache_size && !_cache_items.empty()) {
        evictLeastRecentlyUsed();
    }
}

void UICacheManager::evictLeastRecentlyUsed() {
    if (_cache_items.empty()) return;
    
    // 找到最久未访问的项目
    auto oldest = std::min_element(_cache_items.begin(), _cache_items.end(),
        [](const CacheItem& a, const CacheItem& b) {
            return a.last_access_time < b.last_access_time;
        });
    
    if (oldest != _cache_items.end()) {
        _current_cache_size -= oldest->memory_size;
        if (oldest->image_cache) {
            free((void*)oldest->image_cache);
        }
        _cache_items.erase(oldest);
    }
}

// ===================== FrameRateController 实现 =====================

FrameRateController::FrameRateController() 
    : _mode(FrameRateMode::ADAPTIVE), _target_fps(30), _frame_interval(33),
      _last_frame_time(0), _frame_count(0), _avg_frame_time(33.0f),
      _frame_requested(false) {
    g_frameController = this;
}

FrameRateController::~FrameRateController() {
    g_frameController = nullptr;
}

bool FrameRateController::initialize() {
    updateFrameInterval();
    _last_frame_time = millis();
    
    LOG_I(TAG_UI_OPT, "初始化帧率控制器，目标: %d FPS", _target_fps);
    return true;
}

void FrameRateController::setFrameRateMode(FrameRateMode mode) {
    _mode = mode;
    
    switch (mode) {
        case FrameRateMode::FIXED_30FPS:
            setTargetFPS(30);
            break;
        case FrameRateMode::FIXED_60FPS:
            setTargetFPS(60);
            break;
        case FrameRateMode::POWER_SAVE:
            setTargetFPS(15);
            break;
        case FrameRateMode::ADAPTIVE:
        case FrameRateMode::ON_DEMAND:
            // 保持当前FPS设置
            break;
    }
    
    LOG_I(TAG_UI_OPT, "设置帧率模式: %d", static_cast<int>(mode));
}

void FrameRateController::setTargetFPS(uint32_t fps) {
    _target_fps = fps;
    updateFrameInterval();
    LOG_I(TAG_UI_OPT, "设置目标帧率: %d FPS", fps);
}

bool FrameRateController::shouldRender() {
    uint32_t now = millis();
    
    switch (_mode) {
        case FrameRateMode::ON_DEMAND:
            if (_frame_requested) {
                _frame_requested = false;
                _last_frame_time = now;
                return true;
            }
            return false;
            
        case FrameRateMode::ADAPTIVE:
            // 简单的自适应逻辑：如果平均帧时间过长，降低帧率
            if (_avg_frame_time > _frame_interval * 1.5f) {
                uint32_t adaptive_interval = _frame_interval * 1.5f;
                if (now - _last_frame_time >= adaptive_interval) {
                    _last_frame_time = now;
                    return true;
                }
                return false;
            }
            // 否则按正常间隔渲染
            [[fallthrough]];
            
        default:
            if (now - _last_frame_time >= _frame_interval) {
                _last_frame_time = now;
                return true;
            }
            return false;
    }
}

void FrameRateController::frameComplete() {
    _frame_count++;
    calculateAverageFrameTime();
}

void FrameRateController::requestFrame() {
    _frame_requested = true;
}

uint32_t FrameRateController::getCurrentFPS() {
    if (_avg_frame_time > 0) {
        return 1000.0f / _avg_frame_time;
    }
    return 0;
}

void FrameRateController::updateFrameInterval() {
    _frame_interval = 1000 / _target_fps; // 毫秒
}

void FrameRateController::calculateAverageFrameTime() {
    // 简单的指数移动平均
    static uint32_t last_frame_complete_time = 0;
    uint32_t now = millis();
    
    if (last_frame_complete_time > 0) {
        float current_frame_time = now - last_frame_complete_time;
        _avg_frame_time = _avg_frame_time * 0.9f + current_frame_time * 0.1f;
    }
    
    last_frame_complete_time = now;
}