#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "LVGLDisplay.h"
#include "ChillBitmapFonts.h"

/**
 * @brief 字体测试器
 * @details 测试生成的LVGL字体文件
 */
class FontTester {
public:
    FontTester(LVGLDisplay* display);
    ~FontTester();
    
    bool begin();
    void showFontTest();
    void showNextFont();
    void showPreviousFont();
    
private:
    LVGLDisplay* _display;
    lv_obj_t* _screen;
    lv_obj_t* _title_label;
    lv_obj_t* _ascii_label;
    lv_obj_t* _chinese_label;
    lv_obj_t* _info_label;
    
    int _current_font_index;
    
    // 生成的字体对象
    const lv_font_t* _chill_7px;
    const lv_font_t* _chill_16px;
    
    void createTestUI();
    void loadFonts();
    void updateFontDisplay();
    void setTestFont(const lv_font_t* font, const char* font_name);
    
    // 测试文本
    static const char* ASCII_TEXT;
    static const char* CHINESE_TEXT;
};