#include "FontIntegrationExample.h"
#include "Logger.h"

#define TAG_FONT_DEMO "FontDemo"

FontIntegrationExample::FontIntegrationExample(LVGLDisplay* display) 
    : _display(display), _demo_screen(nullptr),
      _title_label(nullptr), _number_label(nullptr), 
      _history_label(nullptr), _symbol_label(nullptr), _memory_label(nullptr) {
}

FontIntegrationExample::~FontIntegrationExample() {
    // LVGL对象自动清理
}

bool FontIntegrationExample::begin() {
    if (!_display) {
        LOG_E(TAG_FONT_DEMO, "显示指针为空");
        return false;
    }
    
    // 确保FontManager已初始化
    FontManager& fontMgr = FontManager::getInstance();
    if (!fontMgr.initialize()) {
        LOG_E(TAG_FONT_DEMO, "字体管理器初始化失败");
        return false;
    }
    
    createDemoLabels();
    
    LOG_I(TAG_FONT_DEMO, "字体集成演示初始化完成");
    return true;
}

void FontIntegrationExample::demonstrate() {
    LOG_I(TAG_FONT_DEMO, "开始字体集成演示");
    
    basicFontUsage();
    delay(2000);
    
    dynamicFontSwitching();
    delay(2000);
    
    memoryOptimization();
    delay(2000);
    
    fontThemeDemo();
    delay(2000);
    
    scopedFontDemo();
    
    LOG_I(TAG_FONT_DEMO, "字体集成演示完成");
}

void FontIntegrationExample::basicFontUsage() {
    LOG_I(TAG_FONT_DEMO, "演示基础字体使用");
    
    FontManager& fontMgr = FontManager::getInstance();
    
    // 使用不同类型的字体
    if (_title_label) {
        lv_label_set_text(_title_label, "Calculator Display");
        fontMgr.applyFont(_title_label, FontManager::FONT_LARGE);
    }
    
    if (_number_label) {
        lv_label_set_text(_number_label, "123456.789");
        fontMgr.applyFont(_number_label, FontManager::USAGE_NUMBERS);
    }
    
    if (_history_label) {
        lv_label_set_text(_history_label, "Previous: 100+200=300");
        fontMgr.applyFont(_history_label, FontManager::USAGE_HISTORY);
    }
    
    if (_symbol_label) {
        lv_label_set_text(_symbol_label, "+-*/=()");
        fontMgr.applyFont(_symbol_label, FontManager::USAGE_SYMBOLS);
    }
}

void FontIntegrationExample::dynamicFontSwitching() {
    LOG_I(TAG_FONT_DEMO, "演示动态字体切换");
    
    FontManager& fontMgr = FontManager::getInstance();
    
    // 演示字体大小切换
    FontManager::FontType sizes[] = {
        FontManager::FONT_SMALL,
        FontManager::FONT_MEDIUM,
        FontManager::FONT_LARGE
    };
    
    for (int i = 0; i < 3; i++) {
        if (_number_label) {
            fontMgr.applyFont(_number_label, sizes[i]);
            
            // 显示当前字体信息
            FontManager::FontInfo info = fontMgr.getFontInfo(sizes[i]);
            String text = "Size: " + String(info.size) + "px";
            lv_label_set_text(_number_label, text.c_str());
        }
        delay(1000);
    }
}

void FontIntegrationExample::memoryOptimization() {
    LOG_I(TAG_FONT_DEMO, "演示内存优化");
    
    FontManager& fontMgr = FontManager::getInstance();
    
    // 显示内存使用情况
    showMemoryStatus();
    
    // 执行内存优化
    fontMgr.optimizeMemory();
    
    // 再次显示内存状态
    showMemoryStatus();
}

void FontIntegrationExample::fontThemeDemo() {
    LOG_I(TAG_FONT_DEMO, "演示字体主题");
    
    // 注册示例主题
    FontThemeManager::Theme calculatorTheme;
    calculatorTheme.name = "Calculator";
    
    FontThemeManager::registerTheme("Calculator", calculatorTheme);
    FontThemeManager::applyTheme("Calculator");
    
    if (_title_label) {
        lv_label_set_text(_title_label, "Calculator Theme Applied");
    }
}

void FontIntegrationExample::scopedFontDemo() {
    LOG_I(TAG_FONT_DEMO, "演示作用域字体管理");
    
    FontManager& fontMgr = FontManager::getInstance();
    
    if (_number_label) {
        // 设置初始字体
        fontMgr.applyFont(_number_label, FontManager::FONT_MEDIUM);
        lv_label_set_text(_number_label, "Original Font");
        delay(1000);
        
        {
            // 在作用域内使用不同字体
            ScopedFont scoped(_number_label, FontManager::FONT_LARGE);
            lv_label_set_text(_number_label, "Scoped Large Font");
            delay(1000);
        } // 作用域结束，自动恢复原字体
        
        lv_label_set_text(_number_label, "Restored Font");
    }
}

void FontIntegrationExample::createDemoLabels() {
    _demo_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_demo_screen, lv_color_hex(0x000000), LV_PART_MAIN);
    
    // 标题标签
    _title_label = lv_label_create(_demo_screen);
    lv_obj_set_pos(_title_label, 10, 10);
    lv_obj_set_style_text_color(_title_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
    
    // 数字标签
    _number_label = lv_label_create(_demo_screen);
    lv_obj_set_pos(_number_label, 10, 40);
    lv_obj_set_style_text_color(_number_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    
    // 历史标签
    _history_label = lv_label_create(_demo_screen);
    lv_obj_set_pos(_history_label, 10, 70);
    lv_obj_set_style_text_color(_history_label, lv_color_hex(0x808080), LV_PART_MAIN);
    
    // 符号标签
    _symbol_label = lv_label_create(_demo_screen);
    lv_obj_set_pos(_symbol_label, 10, 100);
    lv_obj_set_style_text_color(_symbol_label, lv_color_hex(0xFFFF00), LV_PART_MAIN);
    
    // 内存状态标签
    _memory_label = lv_label_create(_demo_screen);
    lv_obj_set_pos(_memory_label, 10, 130);
    lv_obj_set_style_text_color(_memory_label, lv_color_hex(0xFF8080), LV_PART_MAIN);
    
    lv_scr_load(_demo_screen);
}

void FontIntegrationExample::showMemoryStatus() {
    FontManager& fontMgr = FontManager::getInstance();
    
    uint32_t total = fontMgr.getTotalMemoryUsage();
    uint32_t available = fontMgr.getAvailableMemory();
    
    if (_memory_label) {
        String status = "Memory: " + String(total) + "/" + String(total + available) + " bytes";
        lv_label_set_text(_memory_label, status.c_str());
    }
    
    fontMgr.printMemoryUsage();
}

// FontBestPractices命名空间实现
namespace FontBestPractices {
    
    void applyBasicFonts(lv_obj_t* number_display, lv_obj_t* history_text) {
        FontManager& fontMgr = FontManager::getInstance();
        
        // 数字显示使用大字体
        if (number_display) {
            fontMgr.applyFont(number_display, FontManager::USAGE_NUMBERS);
        }
        
        // 历史记录使用小字体
        if (history_text) {
            fontMgr.applyFont(history_text, FontManager::USAGE_HISTORY);
        }
    }
    
    void smartFontSelection(lv_obj_t* obj, const String& content) {
        if (!obj) return;
        
        FontManager& fontMgr = FontManager::getInstance();
        
        // 根据内容选择字体
        if (content.length() > 20) {
            // 长文本使用小字体
            fontMgr.applyFont(obj, FontManager::FONT_SMALL);
        } else if (content.indexOf("=") >= 0) {
            // 包含等号的数学表达式使用中等字体
            fontMgr.applyFont(obj, FontManager::USAGE_NUMBERS, FontManager::FONT_MEDIUM);
        } else {
            // 默认使用通用字体
            fontMgr.applyFont(obj, FontManager::USAGE_GENERAL);
        }
    }
    
    void setupMemoryOptimizedFonts() {
        FontManager::FontConfig config;
        config.enable_memory_optimization = true;
        config.memory_limit_kb = 16; // 限制16KB内存使用
        config.primary_font = FontManager::FONT_MEDIUM;
        config.fallback_font = FontManager::FONT_SMALL;
        
        FontManager::getInstance().setConfig(config);
        
        LOG_I(TAG_FONT_DEMO, "配置内存优化字体管理");
    }
    
    void setupFontThemes() {
        // 创建计算器主题
        FontThemeManager::Theme calcTheme;
        calcTheme.name = "Calculator";
        
        // 创建紧凑主题（节省内存）
        FontThemeManager::Theme compactTheme;
        compactTheme.name = "Compact";
        
        FontThemeManager::registerTheme("Calculator", calcTheme);
        FontThemeManager::registerTheme("Compact", compactTheme);
        
        LOG_I(TAG_FONT_DEMO, "注册字体主题完成");
    }
    
    bool safeFontApplication(lv_obj_t* obj, FontManager::FontType type) {
        if (!obj) {
            LOG_E(TAG_FONT_DEMO, "对象指针为空");
            return false;
        }
        
        FontManager& fontMgr = FontManager::getInstance();
        
        // 检查字体是否可用
        if (!fontMgr.isFontLoaded(type)) {
            LOG_W(TAG_FONT_DEMO, "字体未加载，使用回退字体");
            fontMgr.applyFont(obj, FontManager::FONT_MEDIUM); // 使用默认字体
            return false;
        }
        
        // 应用字体
        fontMgr.applyFont(obj, type);
        return true;
    }
}