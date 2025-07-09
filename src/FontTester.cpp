#include "FontTester.h"
#include "Logger.h"
#include <lvgl.h>

#if LV_USE_TINY_TTF
#include <extra/libs/tiny_ttf/lv_tiny_ttf.h>
#endif

#define TAG_FONT_TEST "FontTester"

// 测试文本
const char* FontTester::ASCII_TEXT = "Hello World! 123";
const char* FontTester::CHINESE_TEXT = "你好世界！测试";

FontTester::FontTester(LVGLDisplay* display) 
    : _display(display), _screen(nullptr), _title_label(nullptr),
      _ascii_label(nullptr), _chinese_label(nullptr), _info_label(nullptr),
      _current_font_index(0), _chill_7px(nullptr), _chill_16px(nullptr) {
}

FontTester::~FontTester() {
#if LV_USE_TINY_TTF
    if (_chill_7px) {
        lv_tiny_ttf_destroy(_chill_7px);
    }
    if (_chill_16px) {
        lv_tiny_ttf_destroy(_chill_16px);
    }
#endif
}

bool FontTester::begin() {
    if (!_display) {
        return false;
    }
    
    loadTTFFonts();
    createTestUI();
    
    return true;
}

void FontTester::loadTTFFonts() {
    LOG_I(TAG_FONT_TEST, "加载TTF字体...");
    
#if LV_USE_TINY_TTF && LV_TINY_TTF_FILE_SUPPORT
    _chill_7px = lv_tiny_ttf_create_file("src/font/ChillBitmap_7px.ttf", 7);
    _chill_16px = lv_tiny_ttf_create_file("src/font/ChillBitmap_16px.ttf", 16);
#else
    _chill_7px = nullptr;
    _chill_16px = nullptr;
    LOG_W(TAG_FONT_TEST, "Tiny TTF支持未启用");
#endif
    
    LOG_I(TAG_FONT_TEST, "7px字体: %s", _chill_7px ? "成功" : "失败");
    LOG_I(TAG_FONT_TEST, "16px字体: %s", _chill_16px ? "成功" : "失败");
}

void FontTester::createTestUI() {
    _screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x000000), LV_PART_MAIN);
    
    // 标题
    _title_label = lv_label_create(_screen);
    lv_obj_set_pos(_title_label, 10, 5);
    lv_obj_set_style_text_color(_title_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
    lv_label_set_text(_title_label, "TTF Font Test");
    
    // ASCII测试
    _ascii_label = lv_label_create(_screen);
    lv_obj_set_pos(_ascii_label, 10, 30);
    lv_obj_set_style_text_color(_ascii_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_label_set_text(_ascii_label, ASCII_TEXT);
    
    // 中文测试
    _chinese_label = lv_label_create(_screen);
    lv_obj_set_pos(_chinese_label, 10, 60);
    lv_obj_set_style_text_color(_chinese_label, lv_color_hex(0xFFFF00), LV_PART_MAIN);
    lv_label_set_text(_chinese_label, CHINESE_TEXT);
    
    // 信息
    _info_label = lv_label_create(_screen);
    lv_obj_set_pos(_info_label, 10, 90);
    lv_obj_set_style_text_color(_info_label, lv_color_hex(0x808080), LV_PART_MAIN);
    lv_label_set_text(_info_label, "Font: Default");
    
    lv_scr_load(_screen);
}

void FontTester::showFontTest() {
    if (_screen) {
        lv_scr_load(_screen);
    }
}

void FontTester::showNextFont() {
    _current_font_index = (_current_font_index + 1) % 3;
    updateFontDisplay();
}

void FontTester::showPreviousFont() {
    _current_font_index = (_current_font_index - 1 + 3) % 3;
    updateFontDisplay();
}

void FontTester::updateFontDisplay() {
    const lv_font_t* font = LV_FONT_DEFAULT;
    const char* name = "Default";
    
    switch (_current_font_index) {
        case 0:
            font = LV_FONT_DEFAULT;
            name = "Default";
            break;
        case 1:
            font = _chill_7px ? _chill_7px : LV_FONT_DEFAULT;
            name = _chill_7px ? "ChillBitmap 7px" : "ChillBitmap 7px (Failed)";
            break;
        case 2:
            font = _chill_16px ? _chill_16px : LV_FONT_DEFAULT;
            name = _chill_16px ? "ChillBitmap 16px" : "ChillBitmap 16px (Failed)";
            break;
    }
    
    if (_ascii_label) {
        lv_obj_set_style_text_font(_ascii_label, font, LV_PART_MAIN);
    }
    if (_chinese_label) {
        lv_obj_set_style_text_font(_chinese_label, font, LV_PART_MAIN);
    }
    if (_info_label) {
        lv_label_set_text(_info_label, name);
    }
    
    LOG_I(TAG_FONT_TEST, "切换字体: %s", name);
}

void FontTester::setTestFont(const lv_font_t* font, const char* font_name) {
    // 简化实现，使用updateFontDisplay()
}