#include "CharSlideAnim.h"
#include "../../calc_display.h"

CharSlideAnim::CharSlideAnim(CalcDisplay* display, const String& prevText, const String& newText,
                             bool insertMode, uint8_t lineIndex, unsigned long duration)
    : Animation(display, duration, Animation::PRIORITY_NORMAL, 12),  // 降低帧率减少闪烁
      _prevText(prevText), _newText(newText), _isInsertMode(insertMode), _lineIndex(lineIndex),
      _charWidth(0), _startX(0), _endX(0), _currentX(0), _lastX(0) {
}

void CharSlideAnim::start() {
    // 计算动画参数
    calculateAnimationParams();
    
    // 清空目标行，避免旧内容残影
    _display->clearLineArea(_lineIndex, true);
    
    // 调用基类开始动画
    Animation::start();
}

void CharSlideAnim::calculateAnimationParams() {
    // 获取行配置
    const auto& line = _display->lines[_lineIndex];
    _charWidth = _display->getCharWidth(line.textSize);
    
    if (_isInsertMode) {
        // A1: 字符滑入模式
        // 新字符从屏幕右侧滑入到正确位置
        _startX = _display->screenWidth;
        _endX = _display->PAD_X + _charWidth * _prevText.length();
    } else {
        // A2: 字符滑出模式
        // 被删除的字符从当前位置滑出到屏幕右侧
        _startX = _display->PAD_X + _charWidth * _newText.length();
        _endX = _display->screenWidth;
    }
    
    _currentX = _startX;
    _lastX = _startX;
}

void CharSlideAnim::renderFrame(float progress) {
    // 使用缓出函数使动画更自然
    float easedProgress = easeOut(progress);
    
    // 计算当前X位置
    _currentX = _startX + (int16_t)((float)(_endX - _startX) * easedProgress);
    
    // ★ 真正的防闪烁：一次性批量写入
    _display->tft->startWrite();
    
    // 1) 清行 (直接 fillRect，切勿再次 startWrite)
    const auto &line = _display->lines[_lineIndex];
    _display->tft->fillRect(_display->PAD_X, line.y,
                            _display->screenWidth - _display->PAD_X * 2,
                            line.charHeight,
                            _display->COLOR_BG);
    
    if (_isInsertMode) {
        // A1: 滑入模式
        // 2) 绘制静态文本 (内部只setCursor/print)
        if (_prevText.length() > 0) {
            drawStaticText(_prevText, _lineIndex);
        }
        
        // 3) 绘制动态字符
        if (_newText.length() > _prevText.length()) {
            char newChar = _newText.charAt(_prevText.length());
            drawMovingChar(newChar, _currentX, _lineIndex);
        }
    } else {
        // A2: 滑出模式
        // 2) 绘制保留的静态文本
        if (_newText.length() > 0) {
            drawStaticText(_newText, _lineIndex);
        }
        
        // 3) 绘制正在滑出的字符
        if (_prevText.length() > _newText.length()) {
            char deletedChar = _prevText.charAt(_newText.length());
            drawMovingChar(deletedChar, _currentX, _lineIndex);
        }
    }
    
    _display->tft->endWrite();
    
    // 动画结束时更新最终状态
    if (progress >= 1.0f) {
        _display->lines[_lineIndex].text = _newText;
    }
}

void CharSlideAnim::clearLine(uint8_t lineIndex) {
    _display->clearLineArea(lineIndex);
}

void CharSlideAnim::drawStaticText(const String& text, uint8_t lineIndex) {
    if (text.length() == 0) return;
    
    const auto& line = _display->lines[lineIndex];
    
    // 设置文本属性
    _display->tft->setTextColor(line.color);
    _display->tft->setTextSize(line.textSize);
    _display->tft->setCursor(_display->PAD_X, line.y);
    
    // 绘制文本
    _display->tft->print(text);
}

void CharSlideAnim::drawMovingChar(char character, int16_t x, uint8_t lineIndex) {
    const auto& line = _display->lines[lineIndex];
    
    // 设置文本属性
    _display->tft->setTextColor(line.color);
    _display->tft->setTextSize(line.textSize);
    _display->tft->setCursor(x, line.y);
    
    // 绘制单个字符
    _display->tft->print(character);
}

