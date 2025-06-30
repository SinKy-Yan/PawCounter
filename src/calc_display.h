#pragma once
#include <Arduino_GFX_Library.h>

/**
 * @brief 简化版计算器显示UI
 * @details 基于建议的UI设计：
 * - 240×135分辨率，黑底白框
 * - 4行布局：L0历史第2条，L1历史第1条，L2当前表达式，L3计算结果
 * - 局部刷新避免闪烁
 * - 滚动历史效果（L0部分隐藏）
 */
class CalcDisplay {
public:
    CalcDisplay(Arduino_GFX *d, uint16_t w, uint16_t h);

    void pushHistory(const String &line);      // 添加历史记录并滚动
    void setExpr(const String &expr);          // 设置当前表达式
    void setResult(const String &res);         // 设置计算结果
    void refresh();                            // 刷新所有行
    
    // 直接数据更新方法（用于适配器批量更新）
    void updateHistoryDirect(const String &latest, const String &older);
    void updateExprDirect(const String &expr);
    void updateResultDirect(const String &res);

private:
    // UI常量
    static const uint16_t COLOR_BG = 0x0000;      // 黑色背景
    static const uint16_t COLOR_FG = 0xFFFF;      // 白色前景
    static const uint16_t COLOR_HIST = 0x4208;    // 灰色历史
    static const uint8_t PAD_X = 5;               // 左内边距
    
    // 行配置
    struct LineConfig {
        String text;
        uint8_t textSize;
        uint16_t color;
        int16_t y;
        uint8_t charHeight;
    };

    Arduino_GFX *tft;
    uint16_t screenWidth, screenHeight;
    LineConfig lines[4];
    String history[2];  // history[0]最新，history[1]较旧

    void drawFrame();                             // 绘制边框
    void drawLine(uint8_t lineIndex);             // 局部刷新指定行
    void initializeLines();                       // 初始化行配置
};