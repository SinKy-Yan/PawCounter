#include "calc_display.h"
#include "Logger.h"

#define TAG_CALC_DISPLAY "CalcDisp"

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
    
    LOG_I(TAG_CALC_DISPLAY, "CalcDisplay已初始化");
}

CalcDisplay::~CalcDisplay() {
    LOG_I(TAG_CALC_DISPLAY, "CalcDisplay已销毁");
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
    
    // ★ 真正的防闪烁：一次性批量写入
    tft->startWrite();
    
    // 设置文本属性（第二个参数=背景色，可省fillRect）
    tft->setTextColor(line.color, COLOR_BG);
    tft->setTextSize(line.textSize);
    tft->setCursor(PAD_X, line.y);
    tft->print(line.text);
    
    tft->endWrite();
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
    
    // 如果使用Canvas，需要flush到屏幕
    // 检查tft是否为Arduino_Canvas类型
    extern Arduino_Canvas *canvas;
    if (canvas && tft == canvas) {
        canvas->flush();
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

// 简化的tick更新
void CalcDisplay::tick() {
    // --- 推送 Canvas 缓冲到屏幕 ---
    extern Arduino_Canvas *canvas;
    if (canvas && tft == canvas) {
        canvas->flush();
    }
}

// 简化的动画方法（仅记录日志）
void CalcDisplay::animateInputChange(const String& oldTxt, const String& newTxt) {
    LOG_D(TAG_CALC_DISPLAY, "输入变更: %s -> %s", oldTxt.c_str(), newTxt.c_str());
    // 直接刷新显示，不执行动画
    refresh();
}

void CalcDisplay::animateMoveInputToExpr(const String& inputTxt, const String& finalExpr) {
    LOG_D(TAG_CALC_DISPLAY, "移至表达式: %s -> %s", inputTxt.c_str(), finalExpr.c_str());
    // 直接刷新显示，不执行动画
    refresh();
}

// 动画辅助方法
void CalcDisplay::clearLineArea(uint8_t lineIndex, bool inWriteBatch) {
    if (lineIndex >= 4) return;
    
    LineConfig &line = lines[lineIndex];
    
    // 计算行区域
    int16_t x = PAD_X;
    int16_t y = line.y;
    uint16_t w = screenWidth - PAD_X * 2;
    uint16_t h = line.charHeight;
    
    // 根据是否在批量写入中决定是否包装startWrite/endWrite
    if (!inWriteBatch) {
        tft->startWrite();
    }
    
    // 清除区域
    tft->fillRect(x, y, w, h, COLOR_BG);
    
    if (!inWriteBatch) {
        tft->endWrite();
    }
}

uint16_t CalcDisplay::getCharWidth(uint8_t textSize) {
    // Arduino_GFX标准字符宽度：6像素 * textSize
    return 6 * textSize;
}

// 简化的动画管理方法
void CalcDisplay::interruptCurrentAnimation() {
    LOG_D(TAG_CALC_DISPLAY, "请求中断动画(简化版)");
}

bool CalcDisplay::hasActiveAnimation() const {
    return false;  // 简化版本总是返回false
}

uint8_t CalcDisplay::getActiveAnimationCount() const {
    return 0;  // 简化版本总是返回0
}