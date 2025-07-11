#include "FontManager.h"
#include "ChillBitmapFonts.h"
#include "Logger.h"
#include <algorithm>

#define TAG_FONT_MGR "FontManager"

FontManager* FontManager::_instance = nullptr;

FontManager& FontManager::getInstance() {
    if (!_instance) {
        _instance = new FontManager();
    }
    return *_instance;
}

bool FontManager::initialize(const FontConfig& config) {
    if (_initialized) {
        LOG_W(TAG_FONT_MGR, "FontManager已经初始化");
        return true;
    }
    
    _config = config;
    _global_scale = 1.0f;
    _initialized = false;
    
    // 注册ChillBitmap字体
    if (!registerChillBitmapFonts()) {
        LOG_E(TAG_FONT_MGR, "注册ChillBitmap字体失败");
        return false;
    }
    
    // 更新用途映射
    updateUsageMapping();
    
    // 内存优化
    if (_config.enable_memory_optimization) {
        optimizeMemory();
    }
    
    _initialized = true;
    LOG_I(TAG_FONT_MGR, "FontManager初始化完成");
    printFontStatus();
    
    return true;
}

void FontManager::setConfig(const FontConfig& config) {
    _config = config;
    updateUsageMapping();
    
    if (_config.enable_memory_optimization) {
        optimizeMemory();
    }
}

bool FontManager::registerFont(FontType type, const lv_font_t* font, const char* name, FontUsage usage) {
    if (!font || !name) {
        LOG_E(TAG_FONT_MGR, "字体指针或名称为空");
        return false;
    }
    
    FontInfo info(font, name, 0, usage);
    
    // 从字体对象获取大小信息
    if (font->line_height > 0) {
        info.size = font->line_height;
    }
    
    // 计算内存使用
    calculateMemoryUsage(info);
    info.is_loaded = true;
    
    _fonts[type] = info;
    
    LOG_I(TAG_FONT_MGR, "注册字体: %s (类型: %d, 大小: %d, 用途: %d, 内存: %d bytes)", 
          name, type, info.size, usage, info.memory_usage);
    
    return true;
}

bool FontManager::registerChillBitmapFonts() {
    bool success = true;
    
    // 注册7px字体作为小字体
    if (!registerFont(FONT_SMALL, &chill_bitmap_7px, "ChillBitmap 7px", USAGE_HISTORY)) {
        LOG_E(TAG_FONT_MGR, "注册7px字体失败");
        success = false;
    }
    
    // 注册16px字体作为大字体
    if (!registerFont(FONT_LARGE, &chill_bitmap_16px, "ChillBitmap 16px", USAGE_NUMBERS)) {
        LOG_E(TAG_FONT_MGR, "注册16px字体失败");
        success = false;
    }
    
    // 注册16px字体也作为中等字体（通用）
    if (!registerFont(FONT_MEDIUM, &chill_bitmap_16px, "ChillBitmap 16px", USAGE_GENERAL)) {
        LOG_E(TAG_FONT_MGR, "注册中等字体失败");
        success = false;
    }
    
    // 使用默认字体作为回退
    if (LV_FONT_DEFAULT) {
        registerFont(FONT_EXTRA_LARGE, LV_FONT_DEFAULT, "LVGL Default", USAGE_SYMBOLS);
    }
    
    return success;
}

const lv_font_t* FontManager::getFont(FontType type) {
    auto it = _fonts.find(type);
    if (it != _fonts.end() && it->second.is_loaded) {
        return it->second.font;
    }
    
    // 使用回退字体
    return getFallbackFont(type);
}

const lv_font_t* FontManager::getFont(FontUsage usage, FontType preferred_type) {
    // 首先尝试用途映射
    auto usage_it = _usage_mapping.find(usage);
    if (usage_it != _usage_mapping.end()) {
        const lv_font_t* font = getFont(usage_it->second);
        if (font) {
            return font;
        }
    }
    
    // 使用首选类型
    const lv_font_t* font = getFont(preferred_type);
    if (font) {
        return font;
    }
    
    // 最后使用配置的主字体
    return getFont(_config.primary_font);
}

const lv_font_t* FontManager::getFontBySize(uint16_t size) {
    const lv_font_t* best_font = nullptr;
    int min_diff = INT_MAX;
    
    for (const auto& pair : _fonts) {
        if (pair.second.is_loaded) {
            int diff = abs((int)pair.second.size - (int)size);
            if (diff < min_diff) {
                min_diff = diff;
                best_font = pair.second.font;
            }
        }
    }
    
    return best_font ? best_font : getFallbackFont(FONT_MEDIUM);
}

void FontManager::applyFont(lv_obj_t* obj, FontType type) {
    if (!obj) {
        return;
    }
    
    const lv_font_t* font = getFont(type);
    if (font) {
        lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
    }
}

void FontManager::applyFont(lv_obj_t* obj, FontUsage usage, FontType preferred_type) {
    if (!obj) {
        return;
    }
    
    const lv_font_t* font = getFont(usage, preferred_type);
    if (font) {
        lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
    }
}

void FontManager::switchFontTheme(const char* theme_name) {
    LOG_I(TAG_FONT_MGR, "切换字体主题: %s", theme_name);
    
    // 触发字体变更回调
    if (_change_callback) {
        _change_callback(_config.primary_font, _config.primary_font);
    }
}

void FontManager::setGlobalFontScale(float scale) {
    if (scale > 0.1f && scale <= 3.0f) {
        _global_scale = scale;
        LOG_I(TAG_FONT_MGR, "设置全局字体缩放: %.2f", scale);
    }
}

void FontManager::optimizeMemory() {
    if (!_config.enable_memory_optimization) {
        return;
    }
    
    uint32_t total_memory = getTotalMemoryUsage();
    uint32_t limit = _config.memory_limit_kb * 1024;
    
    if (total_memory > limit) {
        LOG_W(TAG_FONT_MGR, "字体内存使用超限: %d/%d bytes", total_memory, limit);
        unloadUnusedFonts();
    }
}

uint32_t FontManager::getTotalMemoryUsage() const {
    uint32_t total = 0;
    for (const auto& pair : _fonts) {
        if (pair.second.is_loaded) {
            total += pair.second.memory_usage;
        }
    }
    return total;
}

uint32_t FontManager::getAvailableMemory() const {
    uint32_t total = getTotalMemoryUsage();
    uint32_t limit = _config.memory_limit_kb * 1024;
    return total < limit ? (limit - total) : 0;
}

void FontManager::unloadUnusedFonts() {
    // 简化实现：标记为未加载但保持字体指针
    // 在实际应用中，可能需要更复杂的引用计数机制
    LOG_I(TAG_FONT_MGR, "执行字体内存优化");
}

std::vector<FontManager::FontInfo> FontManager::getLoadedFonts() const {
    std::vector<FontInfo> loaded;
    for (const auto& pair : _fonts) {
        if (pair.second.is_loaded) {
            loaded.push_back(pair.second);
        }
    }
    return loaded;
}

FontManager::FontInfo FontManager::getFontInfo(FontType type) const {
    auto it = _fonts.find(type);
    return (it != _fonts.end()) ? it->second : FontInfo();
}

bool FontManager::isFontLoaded(FontType type) const {
    auto it = _fonts.find(type);
    return (it != _fonts.end()) && it->second.is_loaded;
}

void FontManager::printFontStatus() const {
    LOG_I(TAG_FONT_MGR, "=== 字体状态 ===");
    for (const auto& pair : _fonts) {
        const FontInfo& info = pair.second;
        LOG_I(TAG_FONT_MGR, "类型: %d, 名称: %s, 大小: %d, 内存: %d, 加载: %s", 
              pair.first, info.name, info.size, info.memory_usage,
              info.is_loaded ? "是" : "否");
    }
    LOG_I(TAG_FONT_MGR, "总内存使用: %d bytes", getTotalMemoryUsage());
}

void FontManager::printMemoryUsage() const {
    uint32_t total = getTotalMemoryUsage();
    uint32_t limit = _config.memory_limit_kb * 1024;
    uint32_t available = getAvailableMemory();
    
    LOG_I(TAG_FONT_MGR, "内存使用: %d/%d bytes (%.1f%%)", 
          total, limit, (float)total / limit * 100.0f);
    LOG_I(TAG_FONT_MGR, "可用内存: %d bytes", available);
}

void FontManager::setFontChangeCallback(FontChangeCallback callback) {
    _change_callback = callback;
}

// 私有方法实现
void FontManager::calculateMemoryUsage(FontInfo& info) {
    // 估算字体内存使用（简化实现）
    // 实际应该通过字体数据结构计算
    if (info.font) {
        // 基于字体高度的简单估算
        info.memory_usage = info.size * info.size * 64; // 简单估算
    }
}

bool FontManager::isMemoryLimitExceeded() const {
    uint32_t total = getTotalMemoryUsage();
    uint32_t limit = _config.memory_limit_kb * 1024;
    return total > limit;
}

const lv_font_t* FontManager::getFallbackFont(FontType type) {
    // 首先尝试配置的回退字体
    auto fallback_it = _fonts.find(_config.fallback_font);
    if (fallback_it != _fonts.end() && fallback_it->second.is_loaded) {
        return fallback_it->second.font;
    }
    
    // 使用LVGL默认字体
    return LV_FONT_DEFAULT;
}

void FontManager::updateUsageMapping() {
    // 设置默认的用途映射
    _usage_mapping[USAGE_GENERAL] = FONT_MEDIUM;
    _usage_mapping[USAGE_NUMBERS] = FONT_LARGE;
    _usage_mapping[USAGE_SYMBOLS] = FONT_MEDIUM;
    _usage_mapping[USAGE_HISTORY] = FONT_SMALL;
}

// ScopedFont实现
ScopedFont::ScopedFont(lv_obj_t* obj, FontManager::FontType type) : _obj(obj), _original_font(nullptr) {
    if (_obj) {
        // 保存原字体
        _original_font = lv_obj_get_style_text_font(_obj, LV_PART_MAIN);
        // 应用新字体
        FontManager::getInstance().applyFont(_obj, type);
    }
}

ScopedFont::ScopedFont(lv_obj_t* obj, FontManager::FontUsage usage, FontManager::FontType preferred_type) 
    : _obj(obj), _original_font(nullptr) {
    if (_obj) {
        _original_font = lv_obj_get_style_text_font(_obj, LV_PART_MAIN);
        FontManager::getInstance().applyFont(_obj, usage, preferred_type);
    }
}

ScopedFont::~ScopedFont() {
    if (_obj && _original_font) {
        lv_obj_set_style_text_font(_obj, _original_font, LV_PART_MAIN);
    }
}

// FontThemeManager实现
std::map<std::string, FontThemeManager::Theme> FontThemeManager::_themes;

void FontThemeManager::registerTheme(const char* name, const Theme& theme) {
    if (name) {
        _themes[name] = theme;
        LOG_I(TAG_FONT_MGR, "注册字体主题: %s", name);
    }
}

bool FontThemeManager::applyTheme(const char* name) {
    if (!name) return false;
    
    auto it = _themes.find(name);
    if (it == _themes.end()) {
        LOG_E(TAG_FONT_MGR, "未找到字体主题: %s", name);
        return false;
    }
    
    const Theme& theme = it->second;
    FontManager& mgr = FontManager::getInstance();
    
    // 应用主题中的字体映射
    for (const auto& font_pair : theme.fonts) {
        // 这里需要更复杂的逻辑来应用主题字体
        LOG_I(TAG_FONT_MGR, "应用主题字体: 类型 %d", font_pair.first);
    }
    
    mgr.switchFontTheme(name);
    return true;
}

std::vector<const char*> FontThemeManager::getAvailableThemes() {
    std::vector<const char*> themes;
    for (const auto& pair : _themes) {
        themes.push_back(pair.first.c_str());
    }
    return themes;
}