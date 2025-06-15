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

// 按键映射表定义（基于实际矩阵扫描顺序）
const KeyMapping CalculatorCore::_keyMappings[] = {
// 扫描顺序索引 -> 实际按键功能
{0,  KeyType::POWER,     "ON",  "POWER",     Operator::NONE,     0},  // 扫描索引0 = KEY1(on/off)
{1,  KeyType::NUMBER,    "7",   "SEVEN",     Operator::NONE,     0},  // 扫描索引1 = KEY2(7)
{2,  KeyType::NUMBER,    "4",   "FOUR",      Operator::NONE,     0},  // 扫描索引2 = KEY3(4)
{3,  KeyType::NUMBER,    "1",   "ONE",       Operator::NONE,     0},  // 扫描索引3 = KEY4(1)
{4,  KeyType::NUMBER,    "0",   "ZERO",      Operator::NONE,     0},  // 扫描索引4 = KEY5(0)
{5,  KeyType::NUMBER,    "5",   "FIVE",      Operator::NONE,     0},  // 扫描索引5 = KEY8(5)
{6,  KeyType::NUMBER,    "2",   "TWO",       Operator::NONE,     0},  // 扫描索引6 = KEY9(2)
{7,  KeyType::NUMBER,    "6",   "SIX",       Operator::NONE,     0},  // 扫描索引7 = KEY12(6)
{8,  KeyType::NUMBER,    "3",   "THREE",     Operator::NONE,     0},  // 扫描索引8 = KEY13(3)
{9,  KeyType::DECIMAL,   ".",   "DOT",       Operator::NONE,     0},  // 扫描索引9 = KEY14(.)
{10, KeyType::OPERATOR,  "-",   "SUB",       Operator::SUBTRACT, 0},  // 扫描索引10 = KEY17(-)
{11, KeyType::OPERATOR,  "+",   "ADD",       Operator::ADD,      0},  // 扫描索引11 = KEY18(+)
{12, KeyType::OPERATOR,  "/",   "DIV",       Operator::DIVIDE,   0},  // 扫描索引12 = KEY21(/)
{13, KeyType::FUNCTION,  "=",   "EQUALS",    Operator::EQUALS,   0},  // 扫描索引13 = KEY22(=)
{14, KeyType::FUNCTION,  "±",   "SIGN",      Operator::NONE,     0},  // 扫描索引14 = KEY20(-/+)
{15, KeyType::OPERATOR,  "*",   "MUL",       Operator::MULTIPLY, 0},  // 扫描索引15 = KEY16(*)
{16, KeyType::NUMBER,    "9",   "NINE",      Operator::NONE,     0},  // 扫描索引16 = KEY11(9)
{17, KeyType::NUMBER,    "8",   "EIGHT",     Operator::NONE,     0},  // 扫描索引17 = KEY7(8)
{18, KeyType::FUNCTION,  "BL",  "BACKLIGHT", Operator::NONE,     0},  // 扫描索引18 = KEY6(bl)
{19, KeyType::FUNCTION,  "%",   "PERCENT",   Operator::NONE,     0},  // 扫描索引19 = KEY10(%)
{20, KeyType::FUNCTION,  "C",   "CLEAR",     Operator::NONE,     0},  // 扫描索引20 = KEY15(c)
{21, KeyType::FUNCTION,  "DEL", "DELETE",    Operator::NONE,     0}   // 扫描索引21 = KEY19(del)
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

bool CalculatorCore::handleKeyInput(uint8_t keyPosition) {
    CALC_LOG_D("Handling key input: position %d", keyPosition);
    
    // 查找按键映射
    const KeyMapping* mapping = findKeyMapping(keyPosition);
    if (!mapping) {
        CALC_LOG_W("No mapping found for key position %d", keyPosition);
        return false;
    }
    
    CALC_LOG_V("Key mapped to: %s (type: %d)", mapping->symbol, (int)mapping->type);
    
    // 清除错误状态
    if (_lastError != CalculatorError::NONE) {
        _lastError = CalculatorError::NONE;
        _state = CalculatorState::INPUT_NUMBER;
    }
    
    // 处理不同类型的按键
    switch (mapping->type) {
        case KeyType::NUMBER:
            handleDigitInput(mapping->symbol[0]);
            break;
            
        case KeyType::DECIMAL:
            if (!_hasDecimalPoint) {
                handleDigitInput('.');
                _hasDecimalPoint = true;
            }
            break;
            
        case KeyType::OPERATOR:
            handleOperatorInput(mapping->operation);
            break;
            
        case KeyType::FUNCTION:
            handleFunctionInput(mapping);
            break;
            
        case KeyType::MODE_SWITCH:
            // 模式切换逻辑
            CALC_LOG_I("Mode switch requested");
            break;
            
        default:
            CALC_LOG_W("Unhandled key type: %d", (int)mapping->type);
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
        _display->updateDisplay(_currentDisplay, _expressionDisplay, _history, _state);
        
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
    
    if (_state == CalculatorState::DISPLAY_RESULT) {
        // 如果当前显示结果，输入数字开始新计算
        _inputBuffer = "";
        _state = CalculatorState::INPUT_NUMBER;
    }
    
    if (digit == '.') {
        if (_inputBuffer.isEmpty()) {
            _inputBuffer = "0.";
        } else {
            _inputBuffer += digit;
        }
    } else {
        if (_inputBuffer == "0") {
            _inputBuffer = String(digit);
        } else {
            _inputBuffer += digit;
        }
    }
    
    _currentDisplay = _inputBuffer;
    _currentNumber = _inputBuffer.toDouble();
}

void CalculatorCore::handleOperatorInput(Operator op) {
    CALC_LOG_V("Operator input: %d", (int)op);
    
    if (_state == CalculatorState::INPUT_NUMBER) {
        if (_pendingOperator != Operator::NONE) {
            // 执行待处理的运算
            if (!performCalculation()) {
                return;
            }
        } else {
            _previousNumber = _currentNumber;
        }
        
        _pendingOperator = op;
        _state = CalculatorState::INPUT_OPERATOR;
        _waitingForOperand = true;
        _hasDecimalPoint = false;
        
        // 更新表达式显示
        _expressionDisplay = formatNumber(_previousNumber) + " " + String(_keyMappings[0].symbol) + " ";
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
    } else if (String(mapping->label) == "CE") {
        clearEntry();
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
        _currentNumber = result.value;
        _currentDisplay = formatNumber(_currentNumber);
        _state = CalculatorState::DISPLAY_RESULT;
        
        // 添加到历史记录
        String expr = formatNumber(_previousNumber) + " " + 
                     String(_keyMappings[0].symbol) + " " + 
                     formatNumber(_currentNumber);
        addToHistory(expr, result.value);
        
        _pendingOperator = Operator::NONE;
        _waitingForOperand = false;
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
    if (abs(number) < 0.000001 && number != 0.0) {
        return String(number, 6);  // 科学计数法
    } else {
        return String(number, 6);
    }
}