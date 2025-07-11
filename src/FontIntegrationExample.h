#pragma once

#include <Arduino.h>
#include "FontManager.h"
#include "LVGLDisplay.h"

/**
 * @brief 字体集成使用示例
 * @details 展示如何在项目中使用FontManager进行字体管理
 */
class FontIntegrationExample {
public:
    FontIntegrationExample(LVGLDisplay* display);
    ~FontIntegrationExample();
    
    bool begin();
    void demonstrate();
    
private:
    LVGLDisplay* _display;
    lv_obj_t* _demo_screen;
    
    // 演示方法
    void basicFontUsage();
    void dynamicFontSwitching();
    void memoryOptimization();
    void fontThemeDemo();
    void scopedFontDemo();
    
    // UI创建方法
    void createDemoLabels();
    void showMemoryStatus();
    
    // 演示用的标签
    lv_obj_t* _title_label;
    lv_obj_t* _number_label;
    lv_obj_t* _history_label;
    lv_obj_t* _symbol_label;
    lv_obj_t* _memory_label;
};

/**
 * @brief 字体使用最佳实践示例
 */
namespace FontBestPractices {
    
    // 1. 基础字体应用
    void applyBasicFonts(lv_obj_t* number_display, lv_obj_t* history_text);
    
    // 2. 智能字体选择
    void smartFontSelection(lv_obj_t* obj, const String& content);
    
    // 3. 内存优化策略
    void setupMemoryOptimizedFonts();
    
    // 4. 主题化字体管理
    void setupFontThemes();
    
    // 5. 错误处理和回退机制
    bool safeFontApplication(lv_obj_t* obj, FontManager::FontType type);
}