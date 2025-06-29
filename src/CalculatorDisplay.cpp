/**
 * @file CalculatorDisplay.cpp
 * @brief 显示管理系统实现
 * 
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "CalculatorDisplay.h"

// ============================================================================
// LCDDisplay 实现
// ============================================================================

LCDDisplay::LCDDisplay(Arduino_GFX* gfx) 
    : _gfx(gfx)
    , _calculatorCore(nullptr)
    , _displayWidth(DISPLAY_WIDTH)
    , _displayHeight(DISPLAY_HEIGHT) {
    
    _theme = getDefaultTheme();
    _numberFormat = getDefaultNumberFormat();
    _unitDisplay.enabled = false;
    
    DISPLAY_LOG_I("LCD display manager created");
}

bool LCDDisplay::begin() {
    if (!_gfx) {
        DISPLAY_LOG_E("GFX object is null");
        return false;
    }
    
    // 显示已在initDisplay()中初始化，这里只设置基本属性
    _gfx->fillScreen(_theme.backgroundColor);
    _gfx->setTextColor(_theme.textColor);
    
    DISPLAY_LOG_I("LCD display initialized");
    return true;
}

void LCDDisplay::clear() {
    if (_gfx) {
        _gfx->fillScreen(_theme.backgroundColor);
    }
}

void LCDDisplay::updateDisplay(const String& number, 
                              const String& expression,
                              CalculatorState state) {
    
    if (!_gfx) return;
    
    // 清屏
    _gfx->fillScreen(_theme.backgroundColor);
    
    // 动态布局计算
    calculateDynamicLayout(number, expression, state);
    
    DISPLAY_LOG_V("LCD display updated");
}

void LCDDisplay::showError(CalculatorError error, const String& message) {
    if (!_gfx) return;
    
    // 显示错误消息
    uint16_t errorX = 10;
    uint16_t errorY = _displayHeight/2;
    
    _gfx->setTextColor(_theme.errorColor);
    _gfx->setTextSize(2);
    _gfx->setCursor(errorX, errorY);
    _gfx->print("ERROR: ");
    _gfx->print(message);
    
    DISPLAY_LOG_W("Error displayed: %s", message.c_str());
}

void LCDDisplay::showStatus(const String& message) {
    if (!_gfx) return;
    
    // 在状态栏显示消息
    _gfx->fillRect(0, _displayHeight - 30, _displayWidth, 30, _theme.backgroundColor);
    _gfx->setTextColor(_theme.statusColor);
    _gfx->setTextSize(1);
    _gfx->setCursor(5, _displayHeight - 25);
    _gfx->print(message);
}

void LCDDisplay::setTheme(const DisplayTheme& theme) {
    _theme = theme;
    DISPLAY_LOG_I("Display theme updated");
}

void LCDDisplay::setNumberFormat(const NumberFormat& format) {
    _numberFormat = format;
    DISPLAY_LOG_I("Number format updated");
}

void LCDDisplay::setUnitDisplay(const UnitDisplay& unitDisplay) {
    _unitDisplay = unitDisplay;
    DISPLAY_LOG_I("Unit display updated");
}

void LCDDisplay::drawMainNumber(const String& number, uint16_t x, uint16_t y, 
                               uint16_t width, uint16_t height) {
    if (!_gfx) return;
    
    // 计算显示坐标
    uint16_t displayX = x + 10;
    uint16_t displayY = y + height/2;
    
    // 设置大字体显示主数字
    _gfx->setTextColor(_theme.resultColor);
    _gfx->setTextSize(_theme.mainFontSize);
    
    // 格式化数字
    String formattedNumber = formatDisplayNumber(number);
    
    // 使用计算后的坐标
    _gfx->setCursor(displayX, displayY);
    _gfx->print(formattedNumber);
}

void LCDDisplay::drawExpression(const String& expression, uint16_t x, uint16_t y,
                               uint16_t width, uint16_t height) {
    if (!_gfx || expression.isEmpty()) return;
    
    _gfx->setTextColor(_theme.expressionColor);
    _gfx->setTextSize(_theme.expressionFontSize);
    _gfx->setCursor(x + 5, y + 5);
    _gfx->print(expression);
}

void LCDDisplay::drawHistory(const std::vector<CalculationHistory>& history,
                            uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    if (!_gfx || history.empty()) return;
    
    _gfx->setTextSize(1);
    _gfx->setTextColor(_theme.textColor);
    
    int lineHeight = 12;
    int maxLines = height / lineHeight;
    int startIndex = max(0, (int)history.size() - maxLines);
    
    for (int i = startIndex; i < history.size(); i++) {
        int lineY = y + (i - startIndex) * lineHeight;
        _gfx->setCursor(x + 2, lineY);
        _gfx->print(history[i].expression);
        _gfx->print(" = ");
        _gfx->print(history[i].result, 2);
    }
}

void LCDDisplay::drawStatusBar(CalculatorState state, uint16_t x, uint16_t y,
                              uint16_t width, uint16_t height) {
    if (!_gfx) return;
    
    _gfx->setTextColor(_theme.statusColor);
    _gfx->setTextSize(_theme.statusFontSize);
    _gfx->setCursor(x + 5, y + 5);
    
    switch (state) {
        case CalculatorState::INPUT_NUMBER:
            _gfx->print("INPUT");
            break;
        case CalculatorState::INPUT_OPERATOR:
            _gfx->print("OP");
            break;
        case CalculatorState::DISPLAY_RESULT:
            _gfx->print("RESULT");
            break;
        case CalculatorState::ERROR:
            _gfx->print("ERROR");
            break;
        default:
            _gfx->print("READY");
            break;
    }
}

void LCDDisplay::drawUnitLabels(const String& number, uint16_t x, uint16_t y) {
    if (!_gfx || !_unitDisplay.enabled) return;
    
    double value = number.toDouble();
    if (value == 0) return;
    
    _gfx->setTextColor(_unitDisplay.unitColor);
    _gfx->setTextSize(_unitDisplay.unitFontSize);
    
    // 显示最高的有效单位
    for (int i = _unitDisplay.unitValues.size() - 1; i >= 0; i--) {
        if (abs(value) >= _unitDisplay.unitValues[i]) {
            _gfx->setCursor(x, y + i * 15);
            _gfx->print(_unitDisplay.unitLabels[i]);
            break;
        }
    }
}

String LCDDisplay::formatDisplayNumber(const String& number) {
    if (!_numberFormat.useThousandsSeparator) {
        return number;
    }
    
    // 简单的千位分隔符实现
    String result = number;
    int dotPos = result.indexOf('.');
    int intPart = (dotPos == -1) ? result.length() : dotPos;
    
    for (int i = intPart - 3; i > 0; i -= 3) {
        result = result.substring(0, i) + _numberFormat.thousandsSeparator + result.substring(i);
    }
    
    return result;
}

DisplayTheme LCDDisplay::getDefaultTheme() {
    DisplayTheme theme;
    theme.backgroundColor = 0x0000;    // 黑色
    theme.textColor = 0xFFFF;          // 白色
    theme.expressionColor = 0x7BEF;    // 浅灰色
    theme.resultColor = 0x07E0;        // 绿色
    theme.errorColor = 0xF800;         // 红色
    theme.statusColor = 0x001F;        // 蓝色
    theme.borderColor = 0x7BEF;        // 浅灰色
    theme.mainFontSize = 4;      // 主数字字体从2改为4，更大更清晰
    theme.expressionFontSize = 2; // 表达式字体从1改为2
    theme.statusFontSize = 1;
    theme.padding = 5;
    theme.lineSpacing = 2;
    theme.showBorders = false;
    return theme;
}

NumberFormat LCDDisplay::getDefaultNumberFormat() {
    NumberFormat format;
    format.useThousandsSeparator = false;
    format.showTrailingZeros = false;
    format.decimalPlaces = 6;
    format.scientificNotation = false;
    format.showSign = false;
    format.currency = "";
    format.thousandsSeparator = ",";
    format.decimalSeparator = ".";
    return format;
}

void LCDDisplay::setCalculatorCore(CalculatorCore* core) {
    _calculatorCore = core;
    DISPLAY_LOG_I("Calculator core reference set");
}

void LCDDisplay::calculateDynamicLayout(const String& number, 
                                       const String& expression,
                                       CalculatorState state) {
    if (!_gfx) return;
    
    // 基于5x7字体的动态布局计算
    const uint16_t margin = 10;
    const uint16_t lineSpacing = 5;
    
    // 历史记录区域（右上角，占屏幕宽度的40%）
    uint16_t historyWidth = _displayWidth * 0.4;
    uint16_t historyHeight = 60; // 预留3行历史记录
    uint16_t historyX = _displayWidth - historyWidth - margin;
    uint16_t historyY = margin;
    
    // 表达式显示区域（右侧，历史记录下方）
    uint16_t exprWidth = historyWidth;
    uint16_t exprHeight = calculateTextHeight(_theme.expressionFontSize) + lineSpacing;
    uint16_t exprX = historyX;
    uint16_t exprY = historyY + historyHeight + lineSpacing;
    
    // 主数字显示区域（左侧，垂直居中）
    String formattedNumber = formatDisplayNumber(number);
    uint16_t numberWidth = calculateTextWidth(formattedNumber, _theme.mainFontSize);
    uint16_t numberHeight = calculateTextHeight(_theme.mainFontSize);
    uint16_t numberX = margin;
    uint16_t numberY = (_displayHeight - numberHeight) / 2;
    
    // 确保主数字不会超出可用空间
    uint16_t availableWidth = historyX - margin * 2;
    if (numberWidth > availableWidth) {
        // 如果数字太长，调整字体大小
        uint8_t adjustedFontSize = _theme.mainFontSize;
        while (adjustedFontSize > 1 && calculateTextWidth(formattedNumber, adjustedFontSize) > availableWidth) {
            adjustedFontSize--;
        }
        numberHeight = calculateTextHeight(adjustedFontSize);
        numberY = (_displayHeight - numberHeight) / 2;
        
        // 绘制调整后的主数字
        drawMainNumberAt(formattedNumber, numberX, numberY, adjustedFontSize);
    } else {
        // 绘制正常大小的主数字
        drawMainNumberAt(formattedNumber, numberX, numberY, _theme.mainFontSize);
    }
    
    // 绘制表达式
    if (!expression.isEmpty()) {
        drawExpressionAt(expression, exprX, exprY, exprWidth, exprHeight);
    }
    
    // 绘制历史记录滚动区域
    drawScrollingHistory(historyX, historyY, historyWidth, historyHeight);
    
    DISPLAY_LOG_V("Dynamic layout calculated and drawn");
}

uint16_t LCDDisplay::calculateTextWidth(const String& text, uint8_t textSize) {
    // 基于5x7字体：每个字符宽5像素，字符间距1像素
    const uint8_t charWidth = 5;
    const uint8_t charSpacing = 1;
    
    if (text.length() == 0) return 0;
    
    return (charWidth + charSpacing) * text.length() * textSize;
}

uint16_t LCDDisplay::calculateTextHeight(uint8_t textSize) {
    // 基于5x7字体：每个字符高7像素
    const uint8_t charHeight = 7;
    return charHeight * textSize;
}

void LCDDisplay::flipCoordinates180(uint16_t x, uint16_t y, uint16_t& flippedX, uint16_t& flippedY) {
    // 180度坐标翻转，考虑文本尺寸
    flippedX = _displayWidth - x - 50;   // 减去文字预估宽度
    flippedY = _displayHeight - y - 20;   // 减去文字高度
    
    // 确保坐标在有效范围内
    if (flippedX > _displayWidth) flippedX = 10;
    if (flippedY > _displayHeight) flippedY = 10;
}

void LCDDisplay::drawScrollingHistory(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    if (!_gfx || !_calculatorCore) return;
    
    const auto& history = _calculatorCore->getHistory();
    if (history.empty()) return;
    
    // 历史记录显示配置
    const uint8_t historyFontSize = 1;  // 小字体显示历史
    const uint16_t lineHeight = calculateTextHeight(historyFontSize) + 2;
    const uint8_t maxLines = height / lineHeight;
    
    // 显示最新的几条记录
    int startIndex = max(0, (int)history.size() - maxLines);
    
    _gfx->setTextColor(0x4208);  // 灰色显示历史记录
    _gfx->setTextSize(historyFontSize);
    
    for (int i = startIndex; i < history.size(); i++) {
        uint16_t lineY = y + (i - startIndex) * lineHeight;
        
        // 构建历史记录显示字符串（简化）
        String historyLine = history[i].expression + "=" + String(history[i].result, 2);
        
        // 如果太长，截断显示
        if (calculateTextWidth(historyLine, historyFontSize) > width) {
            while (historyLine.length() > 5 && calculateTextWidth(historyLine, historyFontSize) > width) {
                historyLine = historyLine.substring(0, historyLine.length() - 1);
            }
            historyLine += "...";
        }
        
        // 应用180度翻转（如果需要）
        uint16_t displayX = x;
        uint16_t displayY = lineY;
        
        _gfx->setCursor(displayX, displayY);
        _gfx->print(historyLine);
    }
}

void LCDDisplay::drawMainNumberAt(const String& number, uint16_t x, uint16_t y, uint8_t fontSize) {
    if (!_gfx) return;
    
    _gfx->setTextColor(_theme.resultColor);
    _gfx->setTextSize(fontSize);
    
    // 应用180度翻转坐标（如果需要）
    uint16_t displayX = x;
    uint16_t displayY = y;
    
    _gfx->setCursor(displayX, displayY);
    _gfx->print(number);
}

void LCDDisplay::drawExpressionAt(const String& expression, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    if (!_gfx || expression.isEmpty()) return;
    
    _gfx->setTextColor(_theme.expressionColor);
    _gfx->setTextSize(_theme.expressionFontSize);
    
    // 如果表达式太长，截断显示
    String displayExpression = expression;
    if (calculateTextWidth(displayExpression, _theme.expressionFontSize) > width) {
        while (displayExpression.length() > 5 && calculateTextWidth(displayExpression, _theme.expressionFontSize) > width) {
            displayExpression = displayExpression.substring(0, displayExpression.length() - 1);
        }
        displayExpression += "...";
    }
    
    _gfx->setCursor(x, y);
    _gfx->print(displayExpression);
}


// ============================================================================
// SerialDisplay 实现
// ============================================================================

SerialDisplay::SerialDisplay() 
    : _colorSupport(true) {
    
    _numberFormat = NumberFormat();
    _unitDisplay.enabled = false;
    
    DISPLAY_LOG_I("Serial display manager created");
}

bool SerialDisplay::begin() {
    if (!Serial) {
        return false;
    }
    
    // 测试颜色支持
    Serial.print(ANSIColors::ANSI_GREEN);
    Serial.print("Calculator Terminal Display");
    Serial.println(ANSIColors::RESET);
    printSeparator();
    
    DISPLAY_LOG_I("Serial display initialized");
    return true;
}

void SerialDisplay::clear() {
    Serial.print("\033[2J\033[H");  // 清屏并移动光标到顶部
}

void SerialDisplay::updateDisplay(const String& number, 
                                 const String& expression,
                                 CalculatorState state) {
    
    // 避免频繁刷新相同内容
    if (number == _lastNumber && expression == _lastExpression) {
        return;
    }
    
    printSeparator(50, '-');
    
    // 显示表达式
    if (!expression.isEmpty() && expression != _lastExpression) {
        printColorText("Expression: " + expression, ANSIColors::ANSI_CYAN);
    }
    
    // 显示当前数字
    String formattedNumber = formatSerialNumber(number);
    printColorText("Display: " + formattedNumber, ANSIColors::BRIGHT_WHITE);
    
    // 显示单位标识
    if (_unitDisplay.enabled) {
        printUnitLabels(number);
    }
    
    // 显示状态
    String stateStr = "State: ";
    switch (state) {
        case CalculatorState::INPUT_NUMBER:
            stateStr += "INPUT";
            break;
        case CalculatorState::INPUT_OPERATOR:
            stateStr += "OPERATOR";
            break;
        case CalculatorState::DISPLAY_RESULT:
            stateStr += "RESULT";
            break;
        case CalculatorState::ERROR:
            stateStr += "ERROR";
            break;
        default:
            stateStr += "READY";
            break;
    }
    printColorText(stateStr, ANSIColors::ANSI_YELLOW);
    
    
    printSeparator(50, '-');
    
    _lastNumber = number;
    _lastExpression = expression;
    
    DISPLAY_LOG_V("Serial display updated");
}

void SerialDisplay::showError(CalculatorError error, const String& message) {
    printSeparator(50, '!');
    printColorText("ERROR: " + message, ANSIColors::BRIGHT_RED);
    printSeparator(50, '!');
}

void SerialDisplay::showStatus(const String& message) {
    printColorText("Status: " + message, ANSIColors::ANSI_BLUE);
}

void SerialDisplay::setTheme(const DisplayTheme& theme) {
    // 串口显示不使用主题，但记录日志
    DISPLAY_LOG_I("Theme setting ignored for serial display");
}

void SerialDisplay::setNumberFormat(const NumberFormat& format) {
    _numberFormat = format;
    DISPLAY_LOG_I("Serial number format updated");
}

void SerialDisplay::setUnitDisplay(const UnitDisplay& unitDisplay) {
    _unitDisplay = unitDisplay;
    DISPLAY_LOG_I("Serial unit display updated");
}

void SerialDisplay::printSeparator(uint8_t length, char character) {
    for (uint8_t i = 0; i < length; i++) {
        Serial.print(character);
    }
    Serial.println();
}

void SerialDisplay::printColorText(const String& text, const String& colorCode) {
    if (_colorSupport && !colorCode.isEmpty()) {
        Serial.print(colorCode);
        Serial.print(text);
        Serial.println(ANSIColors::RESET);
    } else {
        Serial.println(text);
    }
}

String SerialDisplay::formatSerialNumber(const String& number) {
    if (!_numberFormat.useThousandsSeparator) {
        return number;
    }
    
    // 与LCD相同的格式化逻辑
    String result = number;
    int dotPos = result.indexOf('.');
    int intPart = (dotPos == -1) ? result.length() : dotPos;
    
    for (int i = intPart - 3; i > 0; i -= 3) {
        result = result.substring(0, i) + _numberFormat.thousandsSeparator + result.substring(i);
    }
    
    return result;
}

void SerialDisplay::printUnitLabels(const String& number) {
    if (!_unitDisplay.enabled) return;
    
    double value = number.toDouble();
    if (value == 0) return;
    
    Serial.print("Units: ");
    
    // 显示所有适用的单位
    for (int i = 0; i < _unitDisplay.unitValues.size(); i++) {
        if (abs(value) >= _unitDisplay.unitValues[i]) {
            Serial.print(_unitDisplay.unitLabels[i]);
            Serial.print(" ");
        }
    }
    Serial.println();
}

// ============================================================================
// DualDisplay 实现
// ============================================================================

DualDisplay::DualDisplay(std::shared_ptr<LCDDisplay> lcdDisplay,
                        std::shared_ptr<SerialDisplay> serialDisplay)
    : _lcdDisplay(lcdDisplay)
    , _serialDisplay(serialDisplay) {
    
    DISPLAY_LOG_I("Dual display manager created");
}

bool DualDisplay::begin() {
    bool lcdOk = _lcdDisplay ? _lcdDisplay->begin() : false;
    bool serialOk = _serialDisplay ? _serialDisplay->begin() : false;
    
    DISPLAY_LOG_I("Dual display initialized (LCD: %s, Serial: %s)", 
                  lcdOk ? "OK" : "FAIL", serialOk ? "OK" : "FAIL");
    
    return lcdOk || serialOk;  // 至少一个成功即可
}

void DualDisplay::clear() {
    if (_lcdDisplay) _lcdDisplay->clear();
    if (_serialDisplay) _serialDisplay->clear();
}

void DualDisplay::updateDisplay(const String& number, 
                               const String& expression,
                               CalculatorState state) {
    
    if (_lcdDisplay) {
        _lcdDisplay->updateDisplay(number, expression, state);
    }
    
    if (_serialDisplay) {
        _serialDisplay->updateDisplay(number, expression, state);
    }
}

void DualDisplay::showError(CalculatorError error, const String& message) {
    if (_lcdDisplay) _lcdDisplay->showError(error, message);
    if (_serialDisplay) _serialDisplay->showError(error, message);
}

void DualDisplay::showStatus(const String& message) {
    if (_lcdDisplay) _lcdDisplay->showStatus(message);
    if (_serialDisplay) _serialDisplay->showStatus(message);
}

void DualDisplay::setTheme(const DisplayTheme& theme) {
    if (_lcdDisplay) _lcdDisplay->setTheme(theme);
    if (_serialDisplay) _serialDisplay->setTheme(theme);
}

void DualDisplay::setNumberFormat(const NumberFormat& format) {
    if (_lcdDisplay) _lcdDisplay->setNumberFormat(format);
    if (_serialDisplay) _serialDisplay->setNumberFormat(format);
}

void DualDisplay::setUnitDisplay(const UnitDisplay& unitDisplay) {
    if (_lcdDisplay) _lcdDisplay->setUnitDisplay(unitDisplay);
    if (_serialDisplay) _serialDisplay->setUnitDisplay(unitDisplay);
}

void DualDisplay::setCalculatorCore(CalculatorCore* core) {
    if (_lcdDisplay) {
        _lcdDisplay->setCalculatorCore(core);
        DISPLAY_LOG_I("Calculator core reference set for LCD display");
    }
}