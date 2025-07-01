#include "MoveToExprAnim.h"
#include "../../calc_display.h"

MoveToExprAnim::MoveToExprAnim(CalcDisplay* display, const String& inputText, const String& finalExpr,
                               unsigned long duration)
    : Animation(display, duration, Animation::PRIORITY_NORMAL, 15),
      _inputText(inputText), _finalExpr(finalExpr), _operatorSuffix(""),
      _currentY(0), _currentSize(0), _currentColor(0) {
}

void MoveToExprAnim::start() {
    // 提取运算符后缀
    extractOperatorSuffix();
    
    // 计算动画参数
    calculateAnimationParams();
    
    // 初始化当前状态
    _currentY = _params.startY;
    _currentSize = _params.startSize;
    _currentColor = _params.startColor;
    
    // 调用基类开始动画
    Animation::start();
}

void MoveToExprAnim::calculateAnimationParams() {
    // 起始参数：结果行（L3）
    _params.startY = _display->lines[3].y;
    _params.startSize = _display->lines[3].textSize;
    _params.startColor = _display->lines[3].color;
    
    // 目标参数：表达式行（L2）
    _params.endY = _display->lines[2].y;
    _params.endSize = _display->lines[2].textSize;
    _params.endColor = _display->lines[2].color;
}

void MoveToExprAnim::extractOperatorSuffix() {
    // 从最终表达式中提取运算符部分
    // 假设格式为：数字 + 运算符
    if (_finalExpr.length() > _inputText.length()) {
        _operatorSuffix = _finalExpr.substring(_inputText.length());
    }
}

void MoveToExprAnim::renderFrame(float progress) {
    // 使用缓入缓出函数使动画更自然
    float easedProgress = easeOut(progress);
    
    // 计算当前参数
    _currentY = interpolate(_params.startY, _params.endY, easedProgress);
    _currentSize = interpolate(_params.startSize, _params.endSize, easedProgress);
    _currentColor = _params.startColor; // 颜色暂不插值
    
    // ★ 真正的防闪烁：一次性批量写入
    _display->tft->startWrite();
    
    // 1) 清除相关区域 (直接 fillRect，在批量写入中)
    const auto& line2 = _display->lines[2];
    const auto& line3 = _display->lines[3];
    
    // 清除表达式行
    _display->tft->fillRect(_display->PAD_X, line2.y,
                            _display->screenWidth - _display->PAD_X * 2,
                            line2.charHeight, _display->COLOR_BG);
    
    // 清除结果行
    _display->tft->fillRect(_display->PAD_X, line3.y,
                            _display->screenWidth - _display->PAD_X * 2,
                            line3.charHeight, _display->COLOR_BG);
    
    // 2) 绘制移动中的数字
    _display->tft->setTextColor(_currentColor);
    _display->tft->setTextSize(_currentSize);
    _display->tft->setCursor(_display->PAD_X, _currentY);
    _display->tft->print(_inputText);
    
    // 3) 如果有运算符后缀，显示在表达式行
    if (_operatorSuffix.length() > 0 && progress > 0.5f) {
        _display->tft->setTextColor(_display->lines[2].color);
        _display->tft->setTextSize(_display->lines[2].textSize);
        
        // 计算运算符位置
        uint16_t operatorX = _display->PAD_X + 
                           _display->getCharWidth(_display->lines[2].textSize) * _inputText.length();
        _display->tft->setCursor(operatorX, _display->lines[2].y);
        _display->tft->print(_operatorSuffix);
    }
    
    // 4) 显示新的结果（通常是"0"）
    if (progress > 0.3f) {
        _display->tft->setTextColor(_display->lines[3].color);
        _display->tft->setTextSize(_display->lines[3].textSize);
        _display->tft->setCursor(_display->PAD_X, _display->lines[3].y);
        _display->tft->print("0");
    }
    
    _display->tft->endWrite();
    
    // 动画结束时更新最终状态
    if (progress >= 1.0f) {
        _display->lines[2].text = _finalExpr;
        _display->lines[3].text = "0";
    }
}