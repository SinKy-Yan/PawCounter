#include "LVGLCalculatorUI.h"
#include "Logger.h"
#include "all_fonts.h"

#define TAG_LVGL_UI "LVGLCalcUI"

LVGLCalculatorUI::LVGLCalculatorUI(LVGLDisplay* display) 
    : _display(display), _screen(nullptr), _history_label1(nullptr), 
      _history_label2(nullptr), _expr_label(nullptr), _result_label(nullptr) {
    
    // 初始化历史记录
    _history[0] = "";  // 最新
    _history[1] = "";  // 较旧
    _current_expr = "";
    _current_result = "0";
}

LVGLCalculatorUI::~LVGLCalculatorUI() {
    // LVGL对象会自动清理
}

bool LVGLCalculatorUI::begin() {
    if (!_display) {
        LOG_E(TAG_LVGL_UI, "显示指针为空");
        return false;
    }
    
    // 初始化字体管理器
    FontManager& fontMgr = FontManager::getInstance();
    if (!fontMgr.initialize()) {
        LOG_E(TAG_LVGL_UI, "字体管理器初始化失败");
        return false;
    }
    
    // 初始化UI
    initializeUI();
    
    LOG_I(TAG_LVGL_UI, "LVGL计算器UI初始化完成");
    return true;
}

void LVGLCalculatorUI::initializeUI() {
    // 创建主屏幕
    _screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, LV_PART_MAIN);
    
    // 创建UI组件
    createHistoryLabels();
    createExpressionLabel();
    createResultLabel();
    
    // 加载屏幕
    lv_scr_load(_screen);
    
    // 初始刷新
    refresh();
}

void LVGLCalculatorUI::createHistoryLabels() {
    // 历史记录第2行（较旧，部分隐藏）
    _history_label2 = lv_label_create(_screen);
    lv_obj_set_pos(_history_label2, PADDING, -20);  // 部分隐藏
    setLabelStyle(_history_label2, COLOR_HIST, 3);
    lv_label_set_text(_history_label2, "");
    
    // 历史记录第1行（最新）
    _history_label1 = lv_label_create(_screen);
    lv_obj_set_pos(_history_label1, PADDING, 6);
    setLabelStyle(_history_label1, COLOR_HIST, 3);
    lv_label_set_text(_history_label1, "");
}

void LVGLCalculatorUI::createExpressionLabel() {
    // 当前表达式
    _expr_label = lv_label_create(_screen);
    lv_obj_set_pos(_expr_label, PADDING, 32);
    setLabelStyle(_expr_label, COLOR_FG, 3);
    lv_label_set_text(_expr_label, "");
}

void LVGLCalculatorUI::createResultLabel() {
    // 计算结果（大字体）
    _result_label = lv_label_create(_screen);
    lv_obj_set_pos(_result_label, PADDING, 60);
    setLabelStyle(_result_label, COLOR_FG, 8);
    lv_label_set_text(_result_label, "0");
}

void LVGLCalculatorUI::setLabelStyle(lv_obj_t* label, uint32_t color, uint8_t font_size) {
    lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, LV_PART_MAIN);
    
    // 使用WenQuanYi_Bitmap_Song_39px完整字符集字体
    lv_obj_set_style_text_font(label, &WenQuanYi_Bitmap_Song_39px, LV_PART_MAIN);
    
    // 原自定义字体代码（暂时禁用）
    /*
    FontManager& fontMgr = FontManager::getInstance();
    switch (font_size) {
        case 3:
            // 小字体用于历史记录
            fontMgr.applyFont(label, FontManager::USAGE_HISTORY);
            break;
        case 8:
            // 大字体用于数字显示
            fontMgr.applyFont(label, FontManager::USAGE_NUMBERS);
            break;
        default:
            // 默认使用通用字体
            fontMgr.applyFont(label, FontManager::USAGE_GENERAL);
            break;
    }
    */
}

void LVGLCalculatorUI::pushHistory(const String& line) {
    // 滚动历史记录：旧的向上推
    _history[1] = _history[0];  // 最新 → 较旧
    _history[0] = line;         // 新的成为最新
    
    updateHistoryDisplay();
}

void LVGLCalculatorUI::setExpression(const String& expr) {
    _current_expr = expr;
    if (_expr_label) {
        lv_label_set_text(_expr_label, expr.c_str());
    }
}

void LVGLCalculatorUI::setResult(const String& result) {
    _current_result = result;
    if (_result_label) {
        lv_label_set_text(_result_label, result.c_str());
    }
}

void LVGLCalculatorUI::updateHistoryDirect(const String& latest, const String& older) {
    _history[0] = latest;
    _history[1] = older;
    updateHistoryDisplay();
}

void LVGLCalculatorUI::updateExpressionDirect(const String& expr) {
    _current_expr = expr;
    if (_expr_label) {
        lv_label_set_text(_expr_label, expr.c_str());
    }
}

void LVGLCalculatorUI::updateResultDirect(const String& result) {
    _current_result = result;
    if (_result_label) {
        lv_label_set_text(_result_label, result.c_str());
    }
}

void LVGLCalculatorUI::updateHistoryDisplay() {
    if (_history_label1) {
        lv_label_set_text(_history_label1, _history[0].c_str());
    }
    if (_history_label2) {
        lv_label_set_text(_history_label2, _history[1].c_str());
    }
}

void LVGLCalculatorUI::clearAll() {
    _history[0] = "";
    _history[1] = "";
    _current_expr = "";
    _current_result = "0";
    
    updateHistoryDisplay();
    setExpression("");
    setResult("0");
}

void LVGLCalculatorUI::refresh() {
    // 刷新所有显示内容
    updateHistoryDisplay();
    setExpression(_current_expr);
    setResult(_current_result);
    
    // 强制LVGL刷新
    if (_screen) {
        lv_obj_invalidate(_screen);
    }
}

void LVGLCalculatorUI::update() {
    // 强制刷新所有显示内容
    refresh();
}