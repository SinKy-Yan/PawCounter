/**
 * @file CalculatorCore.cpp
 * @brief 计算器核心实现
 * 
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "CalculatorCore.h"
#include "CalculatorDisplay_base.h"
#include "CalculationEngine.h"
// 移除模式系统相关代码
#include "KeyboardConfig.h"
#include "NumberFormatter.h"

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
    // , _currentModeId(0) // 移除模式系统
    , _lastError(CalculatorError::NONE)
    , _currentNumber(0.0)
    , _previousNumber(0.0)
    , _pendingOperator(Operator::NONE)
    , _waitingForOperand(false)
    , _hasDecimalPoint(false)
    , _maxHistorySize(10)
    , _memoryValue(0.0)
    , _hasMemoryValue(false) {
    
    CALC_LOG_I("计算器核心对象创建完成");
}

CalculatorCore::~CalculatorCore() {
    CALC_LOG_I("计算器核心被销毁");
}

bool CalculatorCore::begin() {
    CALC_LOG_I("开始初始化计算器核心");
    
    // 初始化键盘配置管理器
    if (!keyboardConfig.begin()) {
        CALC_LOG_E("键盘配置初始化失败");
        return false;
    }
    
    // 验证关键按键配置
    const KeyConfig* equalsKey = keyboardConfig.getKeyConfig(22, KeyLayer::PRIMARY);
    if (equalsKey) {
        CALC_LOG_I("等号键 (22) 已配置: 符号='%s', 类型=%d, 操作=%d", 
                   equalsKey->symbol.c_str(), (int)equalsKey->type, (int)equalsKey->operation);
    } else {
        CALC_LOG_E("等号键 (22) 在配置中未找到！");
    }
    
    // 打印配置信息（调试用）
    keyboardConfig.printConfig();
    
    // 初始化状态
    _state = CalculatorState::INPUT_NUMBER;
    _currentDisplay = "0";
    _inputBuffer = "";
    _expressionDisplay = "";
    
    // 清空历史记录
    _history.clear();
    _history.reserve(_maxHistorySize);
    
    CALC_LOG_I("计算器核心初始化完成");
    return true;
}

bool CalculatorCore::handleKeyInput(uint8_t keyPosition, bool isLongPress) {
    CALC_LOG_D("处理按键输入: 位置 %d, 长按: %d", keyPosition, isLongPress);
    
    // 首先检查是否为Tab键（层级切换）
    if (keyPosition == keyboardConfig.getLayoutConfig().tabKeyPosition) {
        CALC_LOG_D("检测到Tab键，委托给键盘配置处理");
        return keyboardConfig.handleTabKey(isLongPress);
    }
    
    // 从键盘配置管理器获取当前层级的按键配置
    const KeyConfig* keyConfig = keyboardConfig.getKeyConfig(keyPosition, keyboardConfig.getCurrentLayer());
    if (!keyConfig) {
        // 如果当前层级没有配置，尝试从主层级获取
        CALC_LOG_D("当前层级中未找到按键，尝试主层级");
        keyConfig = keyboardConfig.getKeyConfig(keyPosition, KeyLayer::PRIMARY);
        if (!keyConfig) {
            CALC_LOG_W("位置 %d 没有找到按键配置", keyPosition);
            return false;
        }
    }
    
    CALC_LOG_V("按键映射到: %s (类型: %d, 层级: %d)", 
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
                CALC_LOG_D("处理数字键: 位置=%d, 符号='%s'", keyPosition, keyConfig->symbol.c_str());
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
            // handleModeSwitch(); // 模式切换已移除
            break;
            
        default:
            CALC_LOG_W("未处理的按键类型: %d", (int)keyConfig->type);
            return false;
    }
    
    updateDisplay();
    
    // 调试日志：确认按键处理状态
    Serial.printf("[Core] Key %d handled, mainText=%s state=%d\n",
                  keyPosition, getCurrentDisplay().c_str(), (int)getState());
    
    return true;
}

void CalculatorCore::setDisplay(std::shared_ptr<CalculatorDisplay> display) {
    _display = display;
    CALC_LOG_I("显示管理器已设置");
}

void CalculatorCore::setCalculationEngine(std::shared_ptr<CalculationEngine> engine) {
    _engine = engine;
    CALC_LOG_I("计算引擎已设置");
}

// 模式系统相关函数已移除
// uint8_t CalculatorCore::addMode(...) 
// bool CalculatorCore::switchMode(...)

void CalculatorCore::clearEntry() {
    CALC_LOG_D("清除当前输入");
    _inputBuffer = "";
    _currentDisplay = "0";
    _hasDecimalPoint = false;
    _state = CalculatorState::INPUT_NUMBER;
}

void CalculatorCore::clearAll() {
    CALC_LOG_D("清除所有内容");
    clearEntry();
    _expressionDisplay = "";
    _currentNumber = 0.0;
    _previousNumber = 0.0;
    _pendingOperator = Operator::NONE;
    _waitingForOperand = false;
    _lastError = CalculatorError::NONE;
    _state = CalculatorState::INPUT_NUMBER;
}

void CalculatorCore::updateDisplay() {
    if (_display) {
        Serial.printf("[Core] updateDisplay called: display='%s', expr='%s', state=%d\n",
                      _currentDisplay.c_str(), _expressionDisplay.c_str(), (int)_state);
        _display->updateDisplay(_currentDisplay, _expressionDisplay, _state);
        
        if (_lastError != CalculatorError::NONE) {
            String errorMsg = "Error: ";
            switch (_lastError) {
                case CalculatorError::DIVISION_BY_ZERO:
                    errorMsg += "除数为零";
                    break;
                case CalculatorError::OVERFLOW:
                    errorMsg += "数据溢出";
                    break;
                case CalculatorError::INVALID_OPERATION:
                    errorMsg += "无效操作";
                    break;
                default:
                    errorMsg += "未知错误";
                    break;
            }
            _display->showError(_lastError, errorMsg);
        }
    }
}

void CalculatorCore::update() {
    // 定期更新逻辑（如果需要）
    
    // 调试：确认update被调用（但不要频繁打印）
    static unsigned long lastDebugPrint = 0;
    if (millis() - lastDebugPrint > 5000) { // 每5秒打印一次
        lastDebugPrint = millis();
        Serial.printf("[Core] update() called, display='%s', state=%d\n",
                      _currentDisplay.c_str(), (int)_state);
    }
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
    CALC_LOG_D("数字输入: '%c', 当前状态=%d, 缓冲区='%s'", digit, (int)_state, _inputBuffer.c_str());
    
    if (_state == CalculatorState::DISPLAY_RESULT) {
        // 如果当前显示结果，输入数字开始全新计算
        _inputBuffer = "";
        _expressionDisplay = "";  // 清空表达式
        _state = CalculatorState::INPUT_NUMBER;
        _hasDecimalPoint = false;
        _pendingOperator = Operator::NONE;
        _previousNumber = 0.0;
        _waitingForOperand = false;
        
        CALC_LOG_D("结果显示后开始新计算");
    } else if (_state == CalculatorState::INPUT_OPERATOR) {
        // 如果刚输入运算符，输入数字开始新输入
        _inputBuffer = "";
        _state = CalculatorState::INPUT_NUMBER;
        _hasDecimalPoint = false;
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
        if (_inputBuffer.isEmpty()) {
            _inputBuffer = String(digit);
        } else if (_inputBuffer == "0" && digit != '0') {
            // 如果当前是单个"0"且输入的不是"0"，则替换
            _inputBuffer = String(digit);
        } else {
            // 其他情况都是追加，包括"0"后面再输入"0"
            _inputBuffer += digit;
        }
    }
    
    _currentDisplay = _inputBuffer;
    _currentNumber = _inputBuffer.toDouble();
    
    CALC_LOG_D("数字输入后: 缓冲区='%s', 数字=%.6f", _inputBuffer.c_str(), _currentNumber);
}

void CalculatorCore::handleOperatorInput(Operator op) {
    CALC_LOG_V("运算符输入: %d", (int)op);
    
    String opSymbol = "";
    switch(op) {
        case Operator::ADD: opSymbol = "+"; break;
        case Operator::SUBTRACT: opSymbol = "-"; break;
        case Operator::MULTIPLY: opSymbol = "*"; break;
        case Operator::DIVIDE: opSymbol = "/"; break;
        default: opSymbol = "?"; break;
    }
    
    if (_state == CalculatorState::DISPLAY_RESULT) {
        // 如果当前显示结果，开始新的表达式
        _expressionDisplay = formatNumber(_currentNumber) + opSymbol;
        _previousNumber = _currentNumber;
        _pendingOperator = op;
        
        CALC_LOG_D("结果后开始新表达式: %s", _expressionDisplay.c_str());
        
    } else if (_state == CalculatorState::INPUT_NUMBER) {
        // 如果有待处理的运算符，先执行之前的计算
        if (_pendingOperator != Operator::NONE) {
            // 先将当前数字添加到表达式中
            _expressionDisplay += formatNumber(_currentNumber);
            
            // 执行计算但不修改表达式显示
            if (performCalculation()) {
                // 计算完成后，在表达式后添加新运算符
                _expressionDisplay += opSymbol;
            } else {
                return; // 计算错误，终止
            }
        } else {
            // 将当前数字添加到表达式中
            if (_expressionDisplay.isEmpty()) {
                // 第一个数字和运算符
                _expressionDisplay = formatNumber(_currentNumber) + opSymbol;  
            } else {
                // 继续积累表达式：添加当前数字和新运算符
                _expressionDisplay += formatNumber(_currentNumber) + opSymbol;
            }
        }
        
        // 设置新的待处理运算符和操作数
        _previousNumber = _currentNumber;
        _pendingOperator = op;
        
    } else if (_state == CalculatorState::INPUT_OPERATOR) {
        // 如果已经在等待操作数状态，只是更换最后一个运算符
        if (!_expressionDisplay.isEmpty()) {
            // 移除最后一个运算符，添加新的运算符
            _expressionDisplay = _expressionDisplay.substring(0, _expressionDisplay.length() - 1) + opSymbol;
        }
        _pendingOperator = op;
        CALC_LOG_D("表达式中的运算符已更改: %s", _expressionDisplay.c_str());
    }
    
    // 统一设置状态
    _state = CalculatorState::INPUT_OPERATOR;
    _waitingForOperand = true;
    
    // 重置输入区显示为0，等待下一个数字
    _currentDisplay = "0";
    _inputBuffer = "";
    _hasDecimalPoint = false;
    
    CALC_LOG_D("表达式累计: %s, 当前显示重置为 0", _expressionDisplay.c_str());
}

void CalculatorCore::handleFunctionInput(const KeyMapping* mapping) {
    CALC_LOG_V("功能输入: %s", mapping->label);
    
    if (mapping->operation == Operator::EQUALS) {
        if (_pendingOperator != Operator::NONE && !_expressionDisplay.isEmpty()) {
            // 将最后输入的数字添加到表达式中，形成完整表达式
            String completeExpression = _expressionDisplay + formatNumber(_currentNumber);
            
            // 执行计算
            if (performCalculation()) {
                // 计算完成后，_currentNumber已经是结果
                double result = _currentNumber;
                
                // 新方案：表达式行显示"公式=结果"格式
                _expressionDisplay = completeExpression + "=" + formatNumber(result);
                _currentDisplay = formatNumber(result);   // 结果显示在主显示区
                _state = CalculatorState::DISPLAY_RESULT;
                
                // 将完整表达式添加到历史记录
                addToHistory(completeExpression, result);
                
                CALC_LOG_D("等号执行: %s", _expressionDisplay.c_str());
            }
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
    
    CALC_LOG_D("执行计算: %.6f %d %.6f", 
               _previousNumber, (int)_pendingOperator, _currentNumber);
    
    auto result = _engine->calculate(_previousNumber, _currentNumber, _pendingOperator);
    
    if (result.isValid) {
        // 更新当前数字和显示
        _currentNumber = result.value;
        _previousNumber = result.value;  // 为链式运算准备
        _currentDisplay = formatNumber(_currentNumber);
        
        // 重置运算符状态
        _pendingOperator = Operator::NONE;
        _waitingForOperand = false;
        _inputBuffer = "";
        _hasDecimalPoint = false;
        
        CALC_LOG_D("计算结果: %.6f", _currentNumber);
        return true;
    } else {
        setError(result.error);
        return false;
    }
}

void CalculatorCore::setError(CalculatorError error) {
    _lastError = error;
    _state = CalculatorState::ERROR;
    CALC_LOG_E("计算器错误: %d", (int)error);
}

void CalculatorCore::addToHistory(const String& expression, double result) {
    CalculationHistory entry;
    entry.expression = expression;
    entry.result = result;
    entry.timestamp = millis();
    entry.modeId = 0; // 固定为基本模式
    
    _history.push_back(entry);
    
    // 历史记录已保存在内部，显示器可以通过getHistory()获取
    
    // 限制历史记录数量
    if (_history.size() > _maxHistorySize) {
        _history.erase(_history.begin());
    }
    
    CALC_LOG_D("已添加到历史记录: %s = %.6f", expression.c_str(), result);
}

void CalculatorCore::resetInputState() {
    _inputBuffer = "";
    _hasDecimalPoint = false;
    _waitingForOperand = false;
}

String CalculatorCore::formatNumber(double number) const {
    // 使用统一的NumberFormatter进行格式化
    return NumberFormatter::format(number);
}

// ============================================================================
// 新的按键处理方法实现
// ============================================================================

void CalculatorCore::handleFunctionInput(const KeyConfig* keyConfig) {
    CALC_LOG_V("功能输入 (新): %s", keyConfig->label.c_str());
    
    if (keyConfig->operation == Operator::EQUALS) {
        CALC_LOG_D("等号键被按下. 待处理运算符: %d, 表达式: '%s'", 
                   (int)_pendingOperator, _expressionDisplay.c_str());
        
        if (_pendingOperator != Operator::NONE && !_expressionDisplay.isEmpty()) {
            // 将最后输入的数字添加到表达式中，形成完整表辽式
            String completeExpression = _expressionDisplay + formatNumber(_currentNumber);
            
            // 执行计算
            if (performCalculation()) {
                // 计算完成后，_currentNumber已经是结果
                double result = _currentNumber;
                
                // 新方案：表达式行显示"公式=结果"格式
                _expressionDisplay = completeExpression + "=" + formatNumber(result);
                _currentDisplay = formatNumber(result);   // 结果显示在主显示区
                _state = CalculatorState::DISPLAY_RESULT;
                
                // 将完整表达式添加到历史记录
                addToHistory(completeExpression, result);
                
                CALC_LOG_D("等号执行: %s", _expressionDisplay.c_str());
            }
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
        CALC_LOG_I("自定义功能: %s", keyConfig->functionName.c_str());
        // 这里可以扩展其他功能
    }
}

void CalculatorCore::handleClear() {
    CALC_LOG_D("清除操作");
    clearAll();
}

void CalculatorCore::handleBackspace() {
    CALC_LOG_D("退格操作");
    
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
        
        CALC_LOG_V("退格后: 缓冲区='%s', 数字=%.6f", 
                   _inputBuffer.c_str(), _currentNumber);
    }
}

// 模式切换函数已移除
// void CalculatorCore::handleModeSwitch() { ... }