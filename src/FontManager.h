#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <map>
#include <vector>
#include <functional>

/**
 * @brief 字体管理器类
 * @details 统一管理LVGL字体，支持动态加载、内存优化和字体切换
 * 
 * 主要功能：
 * - 字体注册和管理
 * - 动态字体切换
 * - 内存使用优化
 * - 字体回退机制
 * - 多语言支持
 */
class FontManager {
public:
    /**
     * @brief 字体类型枚举
     */
    enum FontType {
        FONT_SMALL = 0,      // 小字体 (7px)
        FONT_MEDIUM,         // 中等字体 (12px)
        FONT_LARGE,          // 大字体 (16px)
        FONT_EXTRA_LARGE,    // 超大字体 (24px)
        FONT_TYPE_COUNT
    };

    /**
     * @brief 字体用途枚举
     */
    enum FontUsage {
        USAGE_GENERAL = 0,   // 通用文本
        USAGE_NUMBERS,       // 数字显示
        USAGE_SYMBOLS,       // 符号和操作符
        USAGE_HISTORY,       // 历史记录
        USAGE_COUNT
    };

    /**
     * @brief 字体信息结构
     */
    struct FontInfo {
        const lv_font_t* font;
        const char* name;
        uint16_t size;
        uint32_t memory_usage;
        bool is_loaded;
        FontUsage usage;
        
        FontInfo() : font(nullptr), name(""), size(0), memory_usage(0), is_loaded(false), usage(USAGE_GENERAL) {}
        FontInfo(const lv_font_t* f, const char* n, uint16_t s, FontUsage u) 
            : font(f), name(n), size(s), memory_usage(0), is_loaded(false), usage(u) {}
    };

    /**
     * @brief 字体配置结构
     */
    struct FontConfig {
        FontType primary_font;
        FontType fallback_font;
        bool enable_memory_optimization;
        uint32_t memory_limit_kb;
        
        FontConfig() : primary_font(FONT_MEDIUM), fallback_font(FONT_SMALL), 
                      enable_memory_optimization(true), memory_limit_kb(128) {}
    };

    static FontManager& getInstance();
    
    // 初始化和配置
    bool initialize(const FontConfig& config = FontConfig());
    void setConfig(const FontConfig& config);
    const FontConfig& getConfig() const { return _config; }
    
    // 字体注册
    bool registerFont(FontType type, const lv_font_t* font, const char* name, FontUsage usage = USAGE_GENERAL);
    bool registerChillBitmapFonts();
    
    // 字体获取
    const lv_font_t* getFont(FontType type);
    const lv_font_t* getFont(FontUsage usage, FontType preferred_type = FONT_MEDIUM);
    const lv_font_t* getFontBySize(uint16_t size);
    
    // 字体应用
    void applyFont(lv_obj_t* obj, FontType type);
    void applyFont(lv_obj_t* obj, FontUsage usage, FontType preferred_type = FONT_MEDIUM);
    
    // 动态字体切换
    void switchFontTheme(const char* theme_name);
    void setGlobalFontScale(float scale);
    
    // 内存管理
    void optimizeMemory();
    uint32_t getTotalMemoryUsage() const;
    uint32_t getAvailableMemory() const;
    void unloadUnusedFonts();
    
    // 字体信息
    std::vector<FontInfo> getLoadedFonts() const;
    FontInfo getFontInfo(FontType type) const;
    bool isFontLoaded(FontType type) const;
    
    // 调试和监控
    void printFontStatus() const;
    void printMemoryUsage() const;
    
    // 回调函数类型
    using FontChangeCallback = std::function<void(FontType old_type, FontType new_type)>;
    void setFontChangeCallback(FontChangeCallback callback);

private:
    FontManager() = default;
    ~FontManager() = default;
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    
    // 内部状态
    FontConfig _config;
    std::map<FontType, FontInfo> _fonts;
    std::map<FontUsage, FontType> _usage_mapping;
    FontChangeCallback _change_callback;
    float _global_scale;
    bool _initialized;
    
    // 内部方法
    void calculateMemoryUsage(FontInfo& info);
    bool isMemoryLimitExceeded() const;
    const lv_font_t* getFallbackFont(FontType type);
    void updateUsageMapping();
    
    // 静态实例
    static FontManager* _instance;
};

/**
 * @brief 字体管理器辅助类 - 自动字体应用
 * @details RAII方式管理字体应用，确保对象销毁时恢复原字体
 */
class ScopedFont {
public:
    ScopedFont(lv_obj_t* obj, FontManager::FontType type);
    ScopedFont(lv_obj_t* obj, FontManager::FontUsage usage, FontManager::FontType preferred_type = FontManager::FONT_MEDIUM);
    ~ScopedFont();

private:
    lv_obj_t* _obj;
    const lv_font_t* _original_font;
};

/**
 * @brief 字体主题管理器
 * @details 管理预定义的字体主题配置
 */
class FontThemeManager {
public:
    struct Theme {
        const char* name;
        std::map<FontManager::FontType, const lv_font_t*> fonts;
        std::map<FontManager::FontUsage, FontManager::FontType> usage_mapping;
    };
    
    static void registerTheme(const char* name, const Theme& theme);
    static bool applyTheme(const char* name);
    static std::vector<const char*> getAvailableThemes();
    
private:
    static std::map<std::string, Theme> _themes;
};

// 便捷宏定义
#define FONT_MGR FontManager::getInstance()
#define APPLY_FONT(obj, type) FONT_MGR.applyFont(obj, type)
#define GET_FONT(type) FONT_MGR.getFont(type)
#define SCOPED_FONT(obj, type) ScopedFont _scoped_font(obj, type)