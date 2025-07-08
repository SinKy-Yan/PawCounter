#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "LVGLDisplay.h"

/**
 * @brief LVGL计算器UI类
 * @details 使用LVGL重新实现原来的计算器UI功能
 * 包含4行布局：历史记录、当前表达式、计算结果
 */
class LVGLCalculatorUI {
public:
    LVGLCalculatorUI(LVGLDisplay* display);
    ~LVGLCalculatorUI();
    
    bool begin();
    void update();
    
    // UI更新方法
    void pushHistory(const String& line);
    void setExpression(const String& expr);
    void setResult(const String& result);
    void clearAll();
    
    // 直接更新方法
    void updateHistoryDirect(const String& latest, const String& older);
    void updateExpressionDirect(const String& expr);
    void updateResultDirect(const String& result);
    
    // 刷新显示
    void refresh();
    
private:
    LVGLDisplay* _display;
    
    // LVGL对象
    lv_obj_t* _screen;
    lv_obj_t* _history_label1;   // 历史记录第1行
    lv_obj_t* _history_label2;   // 历史记录第2行
    lv_obj_t* _expr_label;       // 表达式标签
    lv_obj_t* _result_label;     // 结果标签
    
    // 数据存储
    String _history[2];          // 历史记录缓存
    String _current_expr;        // 当前表达式
    String _current_result;      // 当前结果
    
    // 颜色和样式常量
    static const uint32_t COLOR_BG = 0x000000;      // 黑色背景
    static const uint32_t COLOR_FG = 0xFFFFFF;      // 白色前景
    static const uint32_t COLOR_HIST = 0x808080;    // 灰色历史
    
    // 布局常量
    static const int16_t PADDING = 15;
    static const int16_t LINE_HEIGHT = 26;
    
    // 初始化方法
    void initializeUI();
    void createHistoryLabels();
    void createExpressionLabel();
    void createResultLabel();
    
    // 样式设置
    void setLabelStyle(lv_obj_t* label, uint32_t color, uint8_t font_size);
    void updateHistoryDisplay();
};