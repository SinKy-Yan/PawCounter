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
                              const std::vector<CalculationHistory>& history,
                              CalculatorState state) {
    
    if (!_gfx) return;
    
    // 清除屏幕
    clear();
    
    // 计算布局
    uint16_t mainHeight = _displayHeight * 0.5;     // 主显示区域占50%
    uint16_t exprHeight = _displayHeight * 0.25;    // 表达式区域占25%
    uint16_t statusHeight = _displayHeight * 0.25;  // 状态区域占25%
    
    // 绘制表达式区域
    if (!expression.isEmpty()) {
        drawExpression(expression, 0, 0, _displayWidth, exprHeight);
    }
    
    // 绘制主数字区域
    drawMainNumber(number, 0, exprHeight, _displayWidth, mainHeight);
    
    // 绘制状态栏
    drawStatusBar(state, 0, exprHeight + mainHeight, _displayWidth, statusHeight);
    
    // 绘制单位标识（如果启用）
    if (_unitDisplay.enabled) {
        drawUnitLabels(number, _displayWidth - 100, exprHeight + 10);
    }
    
    DISPLAY_LOG_V("LCD display updated");
}

void LCDDisplay::showError(CalculatorError error, const String& message) {
    if (!_gfx) return;
    
    _gfx->setTextColor(_theme.errorColor);
    _gfx->setTextSize(2);
    _gfx->setCursor(10, _displayHeight/2);
    _gfx->print("ERROR: ");
    _gfx->println(message);
    
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
    
    // 设置大字体显示主数字
    _gfx->setTextColor(_theme.resultColor);
    _gfx->setTextSize(_theme.mainFontSize);
    
    // 格式化数字
    String formattedNumber = formatDisplayNumber(number);
    
    // 右对齐显示
    int16_t textWidth = formattedNumber.length() * (_theme.mainFontSize * 6);
    int16_t startX = x + width - textWidth - 10;
    if (startX < x) startX = x + 5;
    
    _gfx->setCursor(startX, y + height/2);
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
    theme.mainFontSize = 3;
    theme.expressionFontSize = 2;
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
                                 const std::vector<CalculationHistory>& history,
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
    
    // 显示最近的历史记录
    if (!history.empty()) {
        Serial.println();
        printColorText("Recent calculations:", ANSIColors::ANSI_MAGENTA);
        int showCount = min(3, (int)history.size());
        for (int i = history.size() - showCount; i < history.size(); i++) {
            Serial.print("  ");
            Serial.print(history[i].expression);
            Serial.print(" = ");
            Serial.println(history[i].result, 6);
        }
    }
    
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
                               const std::vector<CalculationHistory>& history,
                               CalculatorState state) {
    
    if (_lcdDisplay) {
        _lcdDisplay->updateDisplay(number, expression, history, state);
    }
    
    if (_serialDisplay) {
        _serialDisplay->updateDisplay(number, expression, history, state);
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