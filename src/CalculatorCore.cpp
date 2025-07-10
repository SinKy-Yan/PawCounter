/**
 * @file CalculatorCore.cpp
 * @brief 计算器核心实现
 * 
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "CalculatorCore.h"
#include "LVGLDisplay.h"
#include "LVGLCalculatorUI.h"
#include "CalculationEngine.h"
#include "KeyboardConfig.h"
#include "NumberFormatter.h"

// 按键映射表已移除，现在使用KeyboardConfig系统

CalculatorCore::CalculatorCore() 
    : _lvgl_display(nullptr)
    , _lvgl_ui(nullptr)
    , _state(CalculatorState::INPUT_NUMBER)
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
    
    // 检查按键是否启用
    if (!keyConfig->isEnabled) {
        CALC_LOG_W("按键 %d 已禁用", keyPosition);
        return false;
    }
    
    CALC_LOG_V("按键映射到: %s (类型: %d, 层级: %d)", 
               keyConfig->symbol.c_str(), (int)keyConfig->type, (int)keyboardConfig.getCurrentLayer());
    
    // 特别调试按键22
    if (keyPosition == 22) {
        Serial.printf("[CALC] 按键22调试: 类型=%d, 操作=%d, 符号='%s', 标签='%s'\n",
                      (int)keyConfig->type, (int)keyConfig->operation, 
                      keyConfig->symbol.c_str(), keyConfig->label.c_str());
    }
    
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
            CALC_LOG_D("处理运算符按键: 位置=%d, 操作=%d", keyPosition, (int)keyConfig->operation);
            handleOperatorInput(keyConfig->operation);
            break;
            
        case KeyType::FUNCTION:
            Serial.printf("[CALC] 调用handleFunctionInput: 位置=%d, 操作=%d\n", keyPosition, (int)keyConfig->operation);
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
    Serial.printf("[核心] 按键 %d 已处理, 主文本=%s 状态=%d\n",
                  keyPosition, getCurrentDisplay().c_str(), (int)getState());
    
    return true;
}

void CalculatorCore::setLVGLDisplay(LVGLDisplay* display) {
    _lvgl_display = display;
    CALC_LOG_I("LVGL显示管理器已设置");
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
    Serial.printf("[核心] updateDisplay 调用: 显示='%s', 表达式='%s', 状态=%d\n",
                  _currentDisplay.c_str(), _expressionDisplay.c_str(), (int)_state);
    
    String displayText = _currentDisplay;
    String errorMsg = "";
    
    // 处理错误显示
    if (_lastError != CalculatorError::NONE) {
        errorMsg = "错误: ";
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
        displayText = errorMsg;
    }
    
    // 更新LVGL UI
    if (_lvgl_ui) {
        _lvgl_ui->updateExpressionDirect(_expressionDisplay);
        _lvgl_ui->updateResultDirect(displayText);
        _lvgl_ui->refresh();
        
        // 确保LVGL处理器能够更新显示
        if (_lvgl_display) {
            _lvgl_display->tick();
        }
    } else {
        // 如果UI对象不存在，尝试重新初始化
        if (_lvgl_display) {
            CALC_LOG_D("LVGL UI对象不存在，尝试重新初始化");
            initLVGLUI();
        }
    }
}

void CalculatorCore::initLVGLUI() {
    if (!_lvgl_display) {
        CALC_LOG_E("LVGL显示管理器未设置");
        return;
    }
    
    CALC_LOG_I("开始初始化LVGL计算器UI");
    
    // 创建LVGL计算器UI
    _lvgl_ui = std::unique_ptr<LVGLCalculatorUI>(new LVGLCalculatorUI(_lvgl_display));
    if (!_lvgl_ui->begin()) {
        CALC_LOG_E("LVGL计算器UI初始化失败");
        _lvgl_ui.reset();
        return;
    }
    
    // 初始化显示内容
    _lvgl_ui->setResult(_currentDisplay);
    _lvgl_ui->setExpression(_expressionDisplay);
    _lvgl_ui->refresh();
    
    CALC_LOG_I("LVGL计算器UI初始化完成，当前显示='%s'", _currentDisplay.c_str());
}

void CalculatorCore::update() {
    // 更新LVGL UI（如果存在）
    if (_lvgl_ui) {
        _lvgl_ui->update();
        // 强制刷新显示
        _lvgl_ui->refresh();
    }
    
    // 确保LVGL处理器能够更新显示
    if (_lvgl_display) {
        _lvgl_display->tick();
    }
    
    // 调试：确认update被调用（但不要频繁打印）
    static unsigned long lastDebugPrint = 0;
    if (millis() - lastDebugPrint > 5000) { // 每5秒打印一次
        lastDebugPrint = millis();
        Serial.printf("[Core] update() called, display='%s', state=%d, ui=%p\n",
                      _currentDisplay.c_str(), (int)_state, _lvgl_ui.get());
    }
}

// 私有方法实现
// findKeyMapping方法已被移除，现在使用KeyboardConfig系统

void CalculatorCore::handleDigitInput(char digit) {
    CALC_LOG_D("数字输入: '%c', 当前状态=%d, 缓冲区='%s'", digit, (int)_state, _inputBuffer.c_str());
    
    // 记录原始的pendingOperator，确保它不会在状态切换中丢失
    Operator originalPendingOperator = _pendingOperator;
    
    if (_state == CalculatorState::DISPLAY_RESULT) {
        // 如果当前显示结果，输入数字开始全新计算
        _inputBuffer = "";
        _expressionDisplay = "";  // 清空表达式
        _state = CalculatorState::INPUT_NUMBER;
        _hasDecimalPoint = false;
        _pendingOperator = Operator::NONE; // 在这种情况下才重置运算符
        _previousNumber = 0.0;
        _waitingForOperand = false;
        
        CALC_LOG_D("结果显示后开始新计算");
    } else if (_state == CalculatorState::INPUT_OPERATOR) {
        // 如果刚输入运算符，输入数字开始新输入，但保留_pendingOperator
        _inputBuffer = "";
        _state = CalculatorState::INPUT_NUMBER;
        _hasDecimalPoint = false;
        // 不重置_pendingOperator
        
        CALC_LOG_D("从运算符状态切换到数字输入状态，保留运算符: %d", (int)_pendingOperator);
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
    
    // 确保_pendingOperator在INPUT_OPERATOR状态切换后不会丢失
    if (_state == CalculatorState::INPUT_NUMBER && originalPendingOperator != Operator::NONE && 
        _pendingOperator == Operator::NONE) {
        _pendingOperator = originalPendingOperator;
        CALC_LOG_D("恢复运算符: %d", (int)_pendingOperator);
    }
    
    CALC_LOG_D("数字输入后: 缓冲区='%s', 数字=%.6f, 运算符=%d", 
               _inputBuffer.c_str(), _currentNumber, (int)_pendingOperator);
}

void CalculatorCore::handleOperatorInput(Operator op) {
    CALC_LOG_D("运算符输入: %d (%s)", (int)op, 
               op == Operator::ADD ? "ADD" :
               op == Operator::SUBTRACT ? "SUBTRACT" :
               op == Operator::MULTIPLY ? "MULTIPLY" :
               op == Operator::DIVIDE ? "DIVIDE" : "UNKNOWN");
    
    String opSymbol = "";
    switch(op) {
        case Operator::ADD: opSymbol = "+"; break;
        case Operator::SUBTRACT: opSymbol = "-"; break;
        case Operator::MULTIPLY: opSymbol = "*"; break;
        case Operator::DIVIDE: opSymbol = "/"; break;
        default: 
            opSymbol = "?"; 
            CALC_LOG_W("未知运算符: %d", (int)op);
            break;
    }
    
    if (_state == CalculatorState::DISPLAY_RESULT) {
        // 如果当前显示结果，开始新的表达式
        _expressionDisplay = NumberFormatter::format(_currentNumber) + opSymbol;
        _previousNumber = _currentNumber;
        _pendingOperator = op;
        
        CALC_LOG_D("结果后开始新表达式: %s", _expressionDisplay.c_str());
        
    } else if (_state == CalculatorState::INPUT_NUMBER) {
        // 如果有待处理的运算符，先执行之前的计算
        if (_pendingOperator != Operator::NONE) {
            // 先将当前数字添加到表达式中
            _expressionDisplay += NumberFormatter::format(_currentNumber);
            
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
                _expressionDisplay = NumberFormatter::format(_currentNumber) + opSymbol;  
            } else {
                // 继续积累表达式：添加当前数字和新运算符
                _expressionDisplay += NumberFormatter::format(_currentNumber) + opSymbol;
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

// 旧的handleFunctionInput(KeyMapping*)方法已被移除，使用新的handleFunctionInput(KeyConfig*)

bool CalculatorCore::performCalculation() {
    // 增加调试日志
    CALC_LOG_D("执行计算开始: _pendingOperator=%d, _previousNumber=%.6f, _currentNumber=%.6f, 表达式='%s'",
               (int)_pendingOperator, _previousNumber, _currentNumber, _expressionDisplay.c_str());
    
    // 当_pendingOperator为NONE时，尝试从表达式中提取运算符和操作数
    if (_pendingOperator == Operator::NONE && !_expressionDisplay.isEmpty()) {
        // 尝试从表达式中提取运算符和操作数
        String numStr = "";
        Operator extractedOp = Operator::NONE;
        
        for (size_t i = 0; i < _expressionDisplay.length(); i++) {
            char c = _expressionDisplay.charAt(i);
            if (c == '+') {
                extractedOp = Operator::ADD;
                break;
            } else if (c == '-') {
                extractedOp = Operator::SUBTRACT;
                break;
            } else if (c == '*') {
                extractedOp = Operator::MULTIPLY;
                break;
            } else if (c == '/') {
                extractedOp = Operator::DIVIDE;
                break;
            } else if (c != ' ') { // 忽略空格
                numStr += c;
            }
        }
        
        if (extractedOp != Operator::NONE) {
            // 设置运算符和第一个操作数
            _pendingOperator = extractedOp;
            if (numStr.length() > 0) {
                _previousNumber = numStr.toDouble();
            }
            CALC_LOG_D("从表达式中提取: 运算符=%d, 第一个操作数=%.6f", 
                       (int)_pendingOperator, _previousNumber);
        }
    }
    
    // 修改判断条件，只要引擎存在且有运算符即可执行计算
    if (!_engine || _pendingOperator == Operator::NONE) {
        CALC_LOG_W("无法执行计算: 引擎=%p, 运算符=%d", _engine.get(), (int)_pendingOperator);
        return false;
    }
    
    CALC_LOG_D("执行计算: %.6f %d %.6f", 
               _previousNumber, (int)_pendingOperator, _currentNumber);
    
    auto result = _engine->calculate(_previousNumber, _currentNumber, _pendingOperator);
    
    if (result.isValid) {
        // 更新当前数字和显示
        _currentNumber = result.value;
        _previousNumber = result.value;  // 为链式运算准备
        _currentDisplay = NumberFormatter::format(_currentNumber);
        
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
    
    // 限制历史记录数量
    if (_history.size() > _maxHistorySize) {
        _history.erase(_history.begin());
    }
    
    // 格式化历史记录行用于显示
    String resultStr = NumberFormatter::format(result, 6);
    String historyLine = expression + " = " + resultStr;
    
    // 更新LVGL UI
    if (_lvgl_ui) {
        _lvgl_ui->pushHistory(historyLine);
    }
    
    CALC_LOG_D("已添加到历史记录: %s = %.6f", expression.c_str(), result);
}

void CalculatorCore::resetInputState() {
    _inputBuffer = "";
    _hasDecimalPoint = false;
    _waitingForOperand = false;
}

// formatNumber方法已被移除，统一使用NumberFormatter::format

// ============================================================================
// 新的按键处理方法实现
// ============================================================================

void CalculatorCore::handleFunctionInput(const KeyConfig* keyConfig) {
    CALC_LOG_V("功能输入 (新): %s", keyConfig->label.c_str());
    
    if (keyConfig->operation == Operator::EQUALS) {
        // 增加更详细的调试信息，帮助分析问题
        Serial.printf("[CALC] 等号键详细信息: _pendingOperator=%d, _expressionDisplay='%s', _currentNumber=%.6f, _state=%d\n", 
                   (int)_pendingOperator, _expressionDisplay.c_str(), _currentNumber, (int)_state);
        
        // 检查表达式格式，确认它是否包含运算符
        bool expressionHasOperator = false;
        for (size_t i = 0; i < _expressionDisplay.length(); i++) {
            char c = _expressionDisplay.charAt(i);
            if (c == '+' || c == '-' || c == '*' || c == '/') {
                expressionHasOperator = true;
                break;
            }
        }
        
        // 修改判断条件，只要有表达式并且表达式中包含运算符即可
        if ((_pendingOperator != Operator::NONE || expressionHasOperator) && !_expressionDisplay.isEmpty()) {
            // 将最后输入的数字添加到表达式中，形成完整表达式
            String completeExpression = _expressionDisplay + NumberFormatter::format(_currentNumber);
            
            // 执行计算
            if (performCalculation()) {
                // 计算完成后，_currentNumber已经是结果
                double result = _currentNumber;
                
                // 新方案：表达式行显示"公式=结果"格式
                _expressionDisplay = completeExpression + "=" + NumberFormatter::format(result);
                _currentDisplay = NumberFormatter::format(result);   // 结果显示在主显示区
                _state = CalculatorState::DISPLAY_RESULT;
                
                // 将完整表达式添加到历史记录
                addToHistory(completeExpression, result);
                
                CALC_LOG_D("等号执行: %s", _expressionDisplay.c_str());
            }
        } else {
            // 如果没有待处理运算符但有表达式，尝试从表达式中提取运算符
            if (_expressionDisplay.isEmpty()) {
                // 没有表达式，无法执行计算
                CALC_LOG_W("等号条件不满足 - 没有表达式可以计算");
            } else if (!expressionHasOperator) {
                // 有表达式但没有运算符，无法执行计算
                CALC_LOG_W("等号条件不满足 - 表达式中没有运算符");
            } else {
                CALC_LOG_W("等号条件不满足 - 待处理运算符: %d (NONE=%d), 表达式为空: %s", 
                       (int)_pendingOperator, (int)Operator::NONE, _expressionDisplay.isEmpty() ? "是" : "否");
            }
        }
    } else if (keyConfig->operation == Operator::PERCENT) {
        // 处理百分比
        if (_state == CalculatorState::INPUT_NUMBER) {
            _currentNumber = _currentNumber / 100.0;
            _currentDisplay = NumberFormatter::format(_currentNumber);
            _inputBuffer = _currentDisplay;
        }
    } else if (keyConfig->functionName == "sign") {
        // 处理正负号切换
        if (_state == CalculatorState::INPUT_NUMBER) {
            _currentNumber = -_currentNumber;
            _currentDisplay = NumberFormatter::format(_currentNumber);
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