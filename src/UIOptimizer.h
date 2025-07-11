#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include <functional>

/**
 * @brief UI性能优化器
 * @details 专为嵌入式环境优化LVGL渲染性能
 */
class UIOptimizer {
public:
    /**
     * @brief 性能配置结构
     */
    struct PerformanceConfig {
        uint32_t refresh_rate_limit;     ///< 刷新率限制 (Hz)
        uint32_t buffer_optimization;    ///< 缓冲区优化等级 (0-3)
        bool enable_partial_refresh;     ///< 启用部分刷新
        bool enable_vsync;               ///< 启用垂直同步
        uint32_t idle_timeout;           ///< 空闲超时 (ms)
        bool enable_auto_sleep;          ///< 启用自动休眠
        uint8_t animation_speed;         ///< 动画速度 (1-10)
        bool enable_shadow_cache;        ///< 启用阴影缓存
    };
    
    /**
     * @brief 渲染统计信息
     */
    struct RenderStats {
        uint32_t frame_count;            ///< 帧计数
        uint32_t avg_frame_time;         ///< 平均帧时间 (us)
        uint32_t max_frame_time;         ///< 最大帧时间 (us)
        uint32_t memory_usage;           ///< 内存使用量 (bytes)
        uint32_t invalid_area_count;     ///< 无效区域计数
        float cpu_usage;                 ///< CPU使用率 (%)
        uint32_t last_update_time;       ///< 上次更新时间
    };
    
    UIOptimizer();
    ~UIOptimizer();
    
    // 初始化和配置
    bool initialize();
    void setPerformanceProfile(const PerformanceConfig& config);
    const PerformanceConfig& getPerformanceConfig() const { return _config; }
    
    // 性能监控
    void startFrameTimer();
    void endFrameTimer();
    const RenderStats& getRenderStats() const { return _stats; }
    void resetStats();
    
    // 渲染优化
    void optimizeForLowMemory();
    void optimizeForSpeed();
    void optimizeForQuality();
    void enableBatchUpdates(bool enable);
    
    // 内存管理
    void garbageCollect();
    size_t getMemoryUsage();
    void setMemoryLimit(size_t limit);
    
    // 节能模式
    void enterPowerSaveMode();
    void exitPowerSaveMode();
    bool isPowerSaveMode() const { return _power_save_mode; }
    
    // 区域刷新优化
    void markDirtyRegion(const lv_area_t& area);
    void optimizeInvalidAreas();
    void flushDirtyRegions();
    
    // 动画优化
    void pauseAnimations();
    void resumeAnimations();
    void setAnimationSpeedScale(float scale);
    
    // 调试和分析
    void enableDebugInfo(bool enable);
    void printPerformanceReport();
    String getPerformanceReport();
    
    // 静态实例访问
    static UIOptimizer* getInstance() { return _instance; }
    
private:
    PerformanceConfig _config;
    RenderStats _stats;
    
    // 性能监控
    uint32_t _frame_start_time;
    uint32_t _last_stats_update;
    std::vector<uint32_t> _frame_times;
    size_t _frame_time_index;
    
    // 内存管理
    size_t _memory_limit;
    size_t _current_memory_usage;
    uint32_t _last_gc_time;
    
    // 节能模式
    bool _power_save_mode;
    uint32_t _last_activity_time;
    uint32_t _idle_start_time;
    
    // 刷新优化
    std::vector<lv_area_t> _dirty_regions;
    bool _batch_updates_enabled;
    uint32_t _last_flush_time;
    
    // 动画控制
    bool _animations_paused;
    float _animation_speed_scale;
    
    // 调试信息
    bool _debug_enabled;
    uint32_t _debug_frame_count;
    
    // 静态实例
    static UIOptimizer* _instance;
    
    // 内部方法
    void updateStats();
    void checkMemoryUsage();
    void checkIdleTimeout();
    void optimizeInvalidAreasInternal();
    void mergeDirtyRegions();
    bool areAreasOverlapping(const lv_area_t& a, const lv_area_t& b);
    lv_area_t mergeAreas(const lv_area_t& a, const lv_area_t& b);
    
    // LVGL回调优化
    static void flushCallback(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p);
    static bool rounderCallback(lv_disp_drv_t* disp_drv, lv_area_t* area);
    static void waitCallback(lv_disp_drv_t* disp_drv);
    
    // 性能预设配置
    static const PerformanceConfig PERFORMANCE_LOW_MEMORY;
    static const PerformanceConfig PERFORMANCE_SPEED;
    static const PerformanceConfig PERFORMANCE_QUALITY;
    static const PerformanceConfig PERFORMANCE_BALANCED;
};

/**
 * @brief UI缓存管理器
 * @details 管理UI组件的缓存以提高渲染性能
 */
class UICacheManager {
public:
    /**
     * @brief 缓存项结构
     */
    struct CacheItem {
        lv_obj_t* object;               ///< 缓存的对象
        lv_img_dsc_t* image_cache;      ///< 图像缓存
        uint32_t last_access_time;      ///< 最后访问时间
        uint32_t access_count;          ///< 访问计数
        size_t memory_size;             ///< 内存大小
        bool is_dirty;                  ///< 是否需要更新
    };
    
    UICacheManager();
    ~UICacheManager();
    
    bool initialize(size_t max_cache_size);
    
    // 缓存管理
    bool cacheObject(lv_obj_t* obj, const String& key);
    lv_obj_t* getCachedObject(const String& key);
    void invalidateCache(const String& key);
    void clearCache();
    
    // 图像缓存
    bool cacheImage(const lv_img_dsc_t* img, const String& key);
    const lv_img_dsc_t* getCachedImage(const String& key);
    void preloadImages(const std::vector<String>& image_keys);
    
    // 内存管理
    void cleanup();
    size_t getCacheMemoryUsage();
    void setMaxCacheSize(size_t size);
    
private:
    std::vector<CacheItem> _cache_items;
    size_t _max_cache_size;
    size_t _current_cache_size;
    uint32_t _cache_hits;
    uint32_t _cache_misses;
    
    void evictLeastRecentlyUsed();
    CacheItem* findCacheItem(const String& key);
    void updateAccessInfo(CacheItem* item);
};

/**
 * @brief 帧率控制器
 * @details 控制UI刷新帧率以优化性能和功耗
 */
class FrameRateController {
public:
    enum class FrameRateMode {
        FIXED_30FPS,        ///< 固定30FPS
        FIXED_60FPS,        ///< 固定60FPS
        ADAPTIVE,           ///< 自适应帧率
        POWER_SAVE,         ///< 节能模式（低帧率）
        ON_DEMAND          ///< 按需刷新
    };
    
    FrameRateController();
    ~FrameRateController();
    
    bool initialize();
    void setFrameRateMode(FrameRateMode mode);
    void setTargetFPS(uint32_t fps);
    
    // 帧率控制
    bool shouldRender();
    void frameComplete();
    void requestFrame();
    
    // 统计信息
    uint32_t getCurrentFPS();
    uint32_t getTargetFPS() const { return _target_fps; }
    float getFrameTime() const { return _avg_frame_time; }
    
private:
    FrameRateMode _mode;
    uint32_t _target_fps;
    uint32_t _frame_interval;
    uint32_t _last_frame_time;
    uint32_t _frame_count;
    float _avg_frame_time;
    bool _frame_requested;
    
    void updateFrameInterval();
    void calculateAverageFrameTime();
};

// 全局优化器实例
extern UIOptimizer* g_uiOptimizer;
extern UICacheManager* g_uiCache;
extern FrameRateController* g_frameController;

// 便捷宏定义
#define UI_OPTIMIZER() (UIOptimizer::getInstance())
#define START_FRAME_TIMER() if(g_uiOptimizer) g_uiOptimizer->startFrameTimer()
#define END_FRAME_TIMER() if(g_uiOptimizer) g_uiOptimizer->endFrameTimer()
#define MARK_DIRTY_REGION(area) if(g_uiOptimizer) g_uiOptimizer->markDirtyRegion(area)