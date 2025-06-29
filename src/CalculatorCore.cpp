/**
 * @file CalculatorCore.cpp
 * @brief 计算器核心实现
 * 
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "CalculatorCore.h"
#include "CalculatorDisplay.h"
#include "CalculationEngine.h"
#include "CalculatorModes.h"
#include "KeyboardConfig.h"

// 按键映射表定义（基于实际物理按键编号 1-22）
// 修改后的布局：Key 19改为清除，Key 15改为向前删除，Key 6改为Tab
const KeyMapping CalculatorCore::_keyMappings[] = {
// 物理按键编号 -> 实际按键功能
{1,  KeyType::POWER,     "ON",  "POWER",     Operator::NONE,     0},  // Key 1: ON/OFF
{2,  KeyType::NUMBER,    "7",   "SEVEN",     Operator::NONE,     0},  // Key 2: 7
{3,  KeyType::NUMBER,    "4",   "FOUR",      Operator::NONE,     0},  // Key 3: 4
{4,  KeyType::NUMBER,    "1",   "ONE",       Operator::NONE,     0},  // Key 4: 1
{5,  KeyType::NUMBER,    "0",   "ZERO",      Operator::NONE,     0},  // Key 5: 0
{6,  KeyType::MODE_SWITCH,"TAB", "LAYER_SWITCH", Operator::NONE, 0},  // Key 6: TAB (层级切换)
{7,  KeyType::NUMBER,    "8",   "EIGHT",     Operator::NONE,     0},  // Key 7: 8
{8,  KeyType::NUMBER,    "5",   "FIVE",      Operator::NONE,     0},  // Key 8: 5
{9,  KeyType::NUMBER,    "2",   "TWO",       Operator::NONE,     0},  // Key 9: 2
{10, KeyType::FUNCTION,  "%",   "PERCENT",   Operator::PERCENT,  0},  // Key 10: PCT
{11, KeyType::NUMBER,    "9",   "NINE",      Operator::NONE,     0},  // Key 11: 9
{12, KeyType::NUMBER,    "6",   "SIX",       Operator::NONE,     0},  // Key 12: 6
{13, KeyType::NUMBER,    "3",   "THREE",     Operator::NONE,     0},  // Key 13: 3
{14, KeyType::DECIMAL,   ".",   "DOT",       Operator::NONE,     0},  // Key 14: .
{15, KeyType::FUNCTION,  "⌫",   "BACKSPACE", Operator::NONE,     0},  // Key 15: 向前删除
{16, KeyType::OPERATOR,  "*",   "MUL",       Operator::MULTIPLY, 0},  // Key 16: MUL
{17, KeyType::OPERATOR,  "-",   "SUB",       Operator::SUBTRACT, 0},  // Key 17: SUB
{18, KeyType::OPERATOR,  "+",   "ADD",       Operator::ADD,      0},  // Key 18: ADD
{19, KeyType::FUNCTION,  "C",   "CLEAR",     Operator::NONE,     0},  // Key 19: 清除
{20, KeyType::FUNCTION,  "±",   "SIGN",      Operator::NONE,     0},  // Key 20: +/-
{21, KeyType::OPERATOR,  "/",   "DIV",       Operator::DIVIDE,   0},  // Key 21: DIV
{22, KeyType::FUNCTION,  "=",   "EQUALS",    Operator::EQUALS,   0}   // Key 22: EQ
};


const size_t CalculatorCore::_keyMappingsSize = sizeof(_keyMappings) / sizeof(_keyMappings[0]);

CalculatorCore::CalculatorCore() 
    : _state(CalculatorState::INPUT_NUMBER)
    , _currentModeId(0)
    , _lastError(CalculatorError::NONE)
    , _currentNumber(0.0)
    , _previousNumber(0.0)
    , _pendingOperator(Operator::NONE)
    , _waitingForOperand(false)
    , _hasDecimalPoint(false)
    , _maxHistorySize(10)
    , _memoryValue(0.0)
    , _hasMemoryValue(false) {
    
    CALC_LOG_I("Calculator core initialized");
}

CalculatorCore::~CalculatorCore() {
    CALC_LOG_I("Calculator core destroyed");
}

bool CalculatorCore::begin() {
    CALC_LOG_I("Starting calculator core initialization");
    
    // 初始化键盘配置管理器
    if (!keyboardConfig.begin()) {
        CALC_LOG_E("Failed to initialize keyboard configuration");
        return false;
    }
    
    // 初始化状态
    _state = CalculatorState::INPUT_NUMBER;
    _currentDisplay = "0";
    _inputBuffer = "";
    _expressionDisplay = "";
    
    // 清空历史记录
    _history.clear();
    _history.reserve(_maxHistorySize);
    
    CALC_LOG_I("Calculator core initialization completed");
    return true;
}

bool CalculatorCore::handleKeyInput(uint8_t keyPosition, bool isLongPress) {
    CALC_LOG_D("Handling key input: position %d, long press: %d", keyPosition, isLongPress);
    
    // 首先检查是否为Tab键（层级切换）
    if (keyPosition == keyboardConfig.getLayoutConfig().tabKeyPosition) {
        return keyboardConfig.handleTabKey(isLongPress);
    }
    
    // 从键盘配置管理器获取当前层级的按键配置
    const KeyConfig* keyConfig = keyboardConfig.getKeyConfig(keyPosition, keyboardConfig.getCurrentLayer());
    if (!keyConfig) {
        // 如果当前层级没有配置，尝试从主层级获取
        keyConfig = keyboardConfig.getKeyConfig(keyPosition, KeyLayer::PRIMARY);
        if (!keyConfig) {
            CALC_LOG_W("No key configuration found for position %d", keyPosition);
            return false;
        }
    }
    
    CALC_LOG_V("Key mapped to: %s (type: %d, layer: %d)", 
               keyConfig->symbol.c_str(), (int)keyConfig->type, (int)keyboardConfig.getCurrentLayer());
    
    // 清除错误状态
    if (_lastError != CalculatorError::NONE) {
        _lastError = CalculatorError::NONE;
        _state = CalculatorState::INPUT_NUMBER;
    }
    
    // 处理不同类型的按键
    switch (keyConfig->type) {
        case KeyType::NUMBER:
            if (!keyConfig->symbol.isEmpty()) {
                handleDigitInput(keyConfig->symbol[0]);
            }
            break;
            
        case KeyType::DECIMAL:
            if (!_hasDecimalPoint) {
                handleDigitInput('.');
                _hasDecimalPoint = true;
            }
            break;
            
        case KeyType::OPERATOR:
            handleOperatorInput(keyConfig->operation);
            break;
            
        case KeyType::FUNCTION:
            handleFunctionInput(keyConfig);
            break;
            
        case KeyType::CLEAR:
            handleClear();
            break;
            
        case KeyType::DELETE:
            handleBackspace();
            break;
            
        case KeyType::LAYER_SWITCH:
            // Tab键已在上面处理
            break;
            
        case KeyType::MODE_SWITCH:
            handleModeSwitch();
            break;
            
        default:
            CALC_LOG_W("Unhandled key type: %d", (int)keyConfig->type);
            return false;
    }
    
    updateDisplay();
    return true;
}

void CalculatorCore::setDisplay(std::shared_ptr<CalculatorDisplay> display) {
    _display = display;
    CALC_LOG_I("Display manager set");
}

void CalculatorCore::setCalculationEngine(std::shared_ptr<CalculationEngine> engine) {
    _engine = engine;
    CALC_LOG_I("Calculation engine set");
}

uint8_t CalculatorCore::addMode(std::shared_ptr<CalculatorMode> mode) {
    _modes.push_back(mode);
    uint8_t modeId = _modes.size() - 1;
    mode->setModeId(modeId);
    CALC_LOG_I("Mode added: %s (ID: %d)", mode->getName().c_str(), modeId);
    return modeId;
}

bool CalculatorCore::switchMode(uint8_t modeId) {
    if (modeId >= _modes.size()) {
        CALC_LOG_E("Invalid mode ID: %d", modeId);
        return false;
    }
    
    // 去激活当前模式
    if (_currentModeId < _modes.size()) {
        _modes[_currentModeId]->deactivate();
    }
    
    // 激活新模式
    _currentModeId = modeId;
    _modes[_currentModeId]->activate(_display, _engine);
    
    CALC_LOG_I("Switched to mode: %s", _modes[_currentModeId]->getName().c_str());
    updateDisplay();
    return true;
}

void CalculatorCore::clearEntry() {
    CALC_LOG_D("Clear entry");
    _inputBuffer = "";
    _currentDisplay = "0";
    _hasDecimalPoint = false;
    _state = CalculatorState::INPUT_NUMBER;
    updateDisplay();
}

void CalculatorCore::clearAll() {
    CALC_LOG_D("Clear all");
    clearEntry();
    _expressionDisplay = "";
    _currentNumber = 0.0;
    _previousNumber = 0.0;
    _pendingOperator = Operator::NONE;
    _waitingForOperand = false;
    _lastError = CalculatorError::NONE;
    updateDisplay();
}

void CalculatorCore::updateDisplay() {
    if (_display) {
        _display->updateDisplay(_currentDisplay, _expressionDisplay, _state);
        
        if (_lastError != CalculatorError::NONE) {
            String errorMsg = "Error: ";
            switch (_lastError) {
                case CalculatorError::DIVISION_BY_ZERO:
                    errorMsg += "Division by zero";
                    break;
                case CalculatorError::OVERFLOW:
                    errorMsg += "Overflow";
                    break;
                case CalculatorError::INVALID_OPERATION:
                    errorMsg += "Invalid operation";
                    break;
                default:
                    errorMsg += "Unknown error";
                    break;
            }
            _display->showError(_lastError, errorMsg);
        }
    }
}

void CalculatorCore::update() {
    // 定期更新逻辑（如果需要）
}

// 私有方法实现
const KeyMapping* CalculatorCore::findKeyMapping(uint8_t position) const {
    for (size_t i = 0; i < _keyMappingsSize; i++) {
        if (_keyMappings[i].keyPosition == position) {
            return &_keyMappings[i];
        }
    }
    return nullptr;
}

void CalculatorCore::handleDigitInput(char digit) {
    CALC_LOG_V("Digit input: %c", digit);
    
    if (_state == CalculatorState::DISPLAY_RESULT || _state == CalculatorState::INPUT_OPERATOR) {
        // 如果当前显示结果或刚输入运算符，输入数字开始新输入
        _inputBuffer = "";
        _state = CalculatorState::INPUT_NUMBER;
        _hasDecimalPoint = false;  // 重置小数点标志
    }
    
    if (digit == '.') {
        if (_inputBuffer.isEmpty()) {
            _inputBuffer = "0.";
            _hasDecimalPoint = true;
        } else if (!_hasDecimalPoint) {
            _inputBuffer += digit;
            _hasDecimalPoint = true;
        }
        // 如果已有小数点，忽略此次输入
    } else {
        if (_inputBuffer.isEmpty() || _inputBuffer == "0") {
            _inputBuffer = String(digit);
        } else {
            _inputBuffer += digit;
        }
    }
    
    _currentDisplay = _inputBuffer;
    _currentNumber = _inputBuffer.toDouble();
    
    CALC_LOG_D("After digit input: buffer='%s', number=%.6f", _inputBuffer.c_str(), _currentNumber);
}

void CalculatorCore::handleOperatorInput(Operator op) {
    CALC_LOG_V("Operator input: %d", (int)op);
    
    if (_state == CalculatorState::INPUT_NUMBER || _state == CalculatorState::DISPLAY_RESULT) {
        if (_pendingOperator != Operator::NONE && _state == CalculatorState::INPUT_NUMBER) {
            // 执行待处理的运算
            if (!performCalculation()) {
                return;
            }
            // performCalculation会设置_previousNumber = result
        } else {
            _previousNumber = _currentNumber;
        }
        
        _pendingOperator = op;
        _state = CalculatorState::INPUT_OPERATOR;
        _waitingForOperand = true;
        
        // 更新表达式显示
        String opSymbol = "";
        switch(op) {
            case Operator::ADD: opSymbol = "+"; break;
            case Operator::SUBTRACT: opSymbol = "-"; break;
            case Operator::MULTIPLY: opSymbol = "*"; break;
            case Operator::DIVIDE: opSymbol = "/"; break;
            default: opSymbol = "?"; break;
        }
        _expressionDisplay = formatNumber(_previousNumber) + " " + opSymbol + " ";
        
        CALC_LOG_D("Operator set: previous=%.6f, op=%s, waiting for operand", 
                   _previousNumber, opSymbol.c_str());
    } else if (_state == CalculatorState::INPUT_OPERATOR) {
        // 如果已经在等待操作数状态，只是更换运算符
        _pendingOperator = op;
        String opSymbol = "";
        switch(op) {
            case Operator::ADD: opSymbol = "+"; break;
            case Operator::SUBTRACT: opSymbol = "-"; break;
            case Operator::MULTIPLY: opSymbol = "*"; break;
            case Operator::DIVIDE: opSymbol = "/"; break;
            default: opSymbol = "?"; break;
        }
        _expressionDisplay = formatNumber(_previousNumber) + " " + opSymbol + " ";
        CALC_LOG_D("Operator changed to: %s", opSymbol.c_str());
    }
}

void CalculatorCore::handleFunctionInput(const KeyMapping* mapping) {
    CALC_LOG_V("Function input: %s", mapping->label);
    
    if (mapping->operation == Operator::EQUALS) {
        if (_pendingOperator != Operator::NONE) {
            performCalculation();
        }
    } else if (String(mapping->label) == "CLEAR") {
        clearAll();
    } else if (String(mapping->label) == "DELETE") {
        // 删除最后一个字符
        if (!_inputBuffer.isEmpty()) {
            if (_inputBuffer.charAt(_inputBuffer.length() - 1) == '.') {
                _hasDecimalPoint = false;
            }
            _inputBuffer.remove(_inputBuffer.length() - 1);
            if (_inputBuffer.isEmpty()) {
                _inputBuffer = "0";
            }
            _currentDisplay = _inputBuffer;
            _currentNumber = _inputBuffer.toDouble();
        }
    } else if (String(mapping->label) == "SIGN") {
        // 正负号切换
        if (_inputBuffer.isEmpty() || _inputBuffer == "0") {
            _inputBuffer = "0";
        } else {
            if (_inputBuffer.charAt(0) == '-') {
                _inputBuffer = _inputBuffer.substring(1);
            } else {
                _inputBuffer = "-" + _inputBuffer;
            }
            _currentDisplay = _inputBuffer;
            _currentNumber = _inputBuffer.toDouble();
        }
    } else if (String(mapping->label) == "PERCENT") {
        // 百分号功能
        _currentNumber = _currentNumber / 100.0;
        _inputBuffer = formatNumber(_currentNumber);
        _currentDisplay = _inputBuffer;
    } else if (mapping->operation == Operator::SQUARE_ROOT) {
        if (_engine) {
            auto result = _engine->calculate(_currentNumber, Operator::SQUARE_ROOT);
            if (result.isValid) {
                _currentNumber = result.value;
                _currentDisplay = formatNumber(_currentNumber);
                _state = CalculatorState::DISPLAY_RESULT;
                addToHistory("√" + formatNumber(_currentNumber), result.value);
            } else {
                setError(result.error);
            }
        }
    }
}

bool CalculatorCore::performCalculation() {
    if (!_engine || _pendingOperator == Operator::NONE) {
        return false;
    }
    
    CALC_LOG_D("Performing calculation: %.6f %d %.6f", 
               _previousNumber, (int)_pendingOperator, _currentNumber);
    
    auto result = _engine->calculate(_previousNumber, _currentNumber, _pendingOperator);
    
    if (result.isValid) {
        // 保存操作数用于历史记录
        double leftOperand = _previousNumber;
        double rightOperand = _currentNumber;
        
        // 更新当前数字和显示
        _currentNumber = result.value;
        _previousNumber = result.value;  // 为链式运算准备
        _currentDisplay = formatNumber(_currentNumber);
        _state = CalculatorState::DISPLAY_RESULT;
        
        // 添加到历史记录
        String opSymbol = "";
        switch(_pendingOperator) {
            case Operator::ADD: opSymbol = "+"; break;
            case Operator::SUBTRACT: opSymbol = "-"; break;
            case Operator::MULTIPLY: opSymbol = "*"; break;
            case Operator::DIVIDE: opSymbol = "/"; break;
            default: opSymbol = "?"; break;
        }
        String expr = formatNumber(leftOperand) + " " + opSymbol + " " + formatNumber(rightOperand);
        addToHistory(expr, result.value);
        
        _pendingOperator = Operator::NONE;
        _waitingForOperand = false;
        
        CALC_LOG_D("Calculation result: %.6f, updated previous=%.6f", 
                   _currentNumber, _previousNumber);
        return true;
    } else {
        setError(result.error);
        return false;
    }
}

void CalculatorCore::setError(CalculatorError error) {
    _lastError = error;
    _state = CalculatorState::ERROR;
    CALC_LOG_E("Calculator error: %d", (int)error);
}

void CalculatorCore::addToHistory(const String& expression, double result) {
    CalculationHistory entry;
    entry.expression = expression;
    entry.result = result;
    entry.timestamp = millis();
    entry.modeId = _currentModeId;
    
    _history.push_back(entry);
    
    // 限制历史记录数量
    if (_history.size() > _maxHistorySize) {
        _history.erase(_history.begin());
    }
    
    CALC_LOG_D("Added to history: %s = %.6f", expression.c_str(), result);
}

void CalculatorCore::resetInputState() {
    _inputBuffer = "";
    _hasDecimalPoint = false;
    _waitingForOperand = false;
}

String CalculatorCore::formatNumber(double number) const {
    // 处理非常小的数字
    if (abs(number) < 0.000001 && number != 0.0) {
        return String(number, 6);  // 科学计数法
    }
    
    // 检查是否为整数
    if (number == floor(number) && abs(number) < 1000000) {
        return String((int)number);  // 整数显示，不带小数点
    }
    
    // 浮点数显示，去除末尾的零
    String result = String(number, 6);
    
    // 移除末尾的零和小数点
    if (result.indexOf('.') != -1) {
        while (result.endsWith("0")) {
            result.remove(result.length() - 1);
        }
        if (result.endsWith(".")) {
            result.remove(result.length() - 1);
        }
    }
    
    return result;
}

// ============================================================================
// 新的按键处理方法实现
// ============================================================================

void CalculatorCore::handleFunctionInput(const KeyConfig* keyConfig) {
    CALC_LOG_V("Function input (new): %s", keyConfig->label.c_str());
    
    if (keyConfig->operation == Operator::EQUALS) {
        if (_pendingOperator != Operator::NONE) {
            performCalculation();
        }
    } else if (keyConfig->operation == Operator::PERCENT) {
        // 处理百分比
        if (_state == CalculatorState::INPUT_NUMBER) {
            _currentNumber = _currentNumber / 100.0;
            _currentDisplay = formatNumber(_currentNumber);
            _inputBuffer = _currentDisplay;
        }
    } else if (keyConfig->functionName == "sign") {
        // 处理正负号切换
        if (_state == CalculatorState::INPUT_NUMBER) {
            _currentNumber = -_currentNumber;
            _currentDisplay = formatNumber(_currentNumber);
            _inputBuffer = _currentDisplay;
        }
    } else {
        // 处理其他自定义函数
        CALC_LOG_I("Custom function: %s", keyConfig->functionName.c_str());
        // 这里可以扩展其他功能
    }
}

void CalculatorCore::handleClear() {
    CALC_LOG_D("Clear operation");
    clearAll();
}

void CalculatorCore::handleBackspace() {
    CALC_LOG_D("Backspace operation");
    
    if (_state == CalculatorState::INPUT_NUMBER && !_inputBuffer.isEmpty()) {
        // 删除最后一个字符
        char lastChar = _inputBuffer.charAt(_inputBuffer.length() - 1);
        _inputBuffer.remove(_inputBuffer.length() - 1);
        
        // 如果删除的是小数点，重置小数点标志
        if (lastChar == '.') {
            _hasDecimalPoint = false;
        }
        
        // 更新当前数字和显示
        if (_inputBuffer.isEmpty()) {
            _currentNumber = 0.0;
            _currentDisplay = "0";
            _inputBuffer = "";
        } else {
            _currentNumber = _inputBuffer.toDouble();
            _currentDisplay = _inputBuffer;
        }
        
        CALC_LOG_V("After backspace: buffer='%s', number=%.6f", 
                   _inputBuffer.c_str(), _currentNumber);
    }
}

void CalculatorCore::handleModeSwitch() {
    CALC_LOG_I("Mode switch requested");
    
    // 简单的模式切换：在基本模式和财务模式间切换
    uint8_t nextModeId = (_currentModeId == 0) ? 1 : 0;
    
    if (nextModeId < _modes.size()) {
        switchMode(nextModeId);
    } else {
        CALC_LOG_W("Mode %d not available", nextModeId);
    }
}