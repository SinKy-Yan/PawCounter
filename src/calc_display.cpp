#include "calc_display.h"

CalcDisplay::CalcDisplay(Arduino_GFX *d, uint16_t w, uint16_t h)
    : tft(d), screenWidth(w), screenHeight(h) {
    
    // 初始化历史记录
    history[0] = "";  // 最新
    history[1] = "";  // 较旧
    
    // 初始化行配置
    initializeLines();
    
    // 绘制初始界面
    drawFrame();
    refresh();
}

void CalcDisplay::initializeLines() {
    // L0: 历史第2条（最旧）- 部分隐藏营造滚动效果（硬件已扩展5像素）
    lines[0] = {history[1], 3, COLOR_HIST, -6, 24};
    
    // L1: 历史第1条（较旧）
    lines[1] = {history[0], 3, COLOR_HIST, 20, 24};
    
    // L2: 当前输入表达式
    lines[2] = {"", 3, COLOR_FG, 46, 24};
    
    // L3: 计算结果（超大字体）
    lines[3] = {"0", 8, COLOR_FG, 74, 64};
}

void CalcDisplay::drawFrame() {
    // 清屏为黑色
    tft->fillScreen(COLOR_BG);
    
    // 移除白色边框，保持纯黑色背景
    // 简洁无边框设计
}

void CalcDisplay::drawLine(uint8_t lineIndex) {
    if (lineIndex >= 4) return;
    
    LineConfig &line = lines[lineIndex];
    
    // 设置文本属性并绘制（不做局部擦除）
    tft->setTextColor(line.color);
    tft->setTextSize(line.textSize);
    tft->setCursor(PAD_X, line.y);
    tft->print(line.text);
}

void CalcDisplay::pushHistory(const String &line) {
    // 滚动历史记录：旧的向上推
    history[1] = history[0];  // 最新 → 较旧
    history[0] = line;        // 新的成为最新
    
    // 更新行配置
    lines[0].text = history[1];  // L0显示较旧的
    lines[1].text = history[0];  // L1显示最新的
    
    // 全屏刷新
    refresh();
}

void CalcDisplay::setExpr(const String &expr) {
    lines[2].text = expr;
    // 全屏刷新
    refresh();
}

void CalcDisplay::setResult(const String &res) {
    lines[3].text = res;
    // 全屏刷新
    refresh();
}

void CalcDisplay::refresh() {
    // 全屏刷新：先清屏，再重绘所有内容
    tft->fillScreen(COLOR_BG);                                    // 清屏为黑色
    
    // 绘制所有行（无边框）
    for (uint8_t i = 0; i < 4; i++) {
        drawLine(i);
    }
}

// 直接数据更新方法（不立即刷新）
void CalcDisplay::updateHistoryDirect(const String &latest, const String &older) {
    history[0] = latest;
    history[1] = older;
    lines[0].text = history[1];  // L0显示较旧的
    lines[1].text = history[0];  // L1显示最新的
}

void CalcDisplay::updateExprDirect(const String &expr) {
    lines[2].text = expr;
}

void CalcDisplay::updateResultDirect(const String &res) {
    lines[3].text = res;
}