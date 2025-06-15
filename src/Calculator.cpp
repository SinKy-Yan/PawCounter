#include "Calculator.h"
#include "config.h"
#include <FastLED.h>

extern CRGB leds[NUM_LEDS];  // 引用LED数组

// 修改后的按键映射表，运算符使用英文命名
const KeyFunction KEY_FUNCTIONS[] = {
    {1,  "ON/OFF", 3},  // Key 1
    {2,  "7",      0},  // Key 2
    {3,  "4",      0},  // Key 3
    {4,  "1",      0},  // Key 4
    {5,  "0",      0},  // Key 5
    {6,  "BT",     3},  // Key 6
    {7,  "8",      0},  // Key 7
    {8,  "5",      0},  // Key 8
    {9,  "2",      0},  // Key 9
    {10, "PCT",    1},  // Key 10 - 百分号改为PCT
    {11, "9",      0},  // Key 11
    {12, "6",      0},  // Key 12
    {13, "3",      0},  // Key 13
    {14, ".",      4},  // Key 14
    {15, "C",      2},  // Key 15
    {16, "MUL",    1},  // Key 16 - 乘号改为MUL
    {17, "SUB",    1},  // Key 17 - 减号改为SUB
    {18, "ADD",    1},  // Key 18 - 加号改为ADD
    {19, "DEL",    2},  // Key 19
    {20, "+/-",    2},  // Key 20
    {21, "DIV",    1},  // Key 21 - 除号改为DIV
    {22, "EQ",     2}   // Key 22 - 等号改为EQ
};

Calculator::Calculator(Arduino_GFX *gfx) {
    _gfx = gfx;
    _hasHistory = false;
    _powerKeyPressStart = 0;    
    _btKeyPressStart = 0;       
    
    // 初始化按键控制相关变量
    _lastKeyPressed = 0;
    _keyProcessed = true;
    
    // 设置文本参数
    _gfx->setTextWrap(false);   
    
    clearAll();
}

void Calculator::formatNumber(float num, char* buffer) {
    // 转换数字为字符串
    char tempBuffer[32];
    dtostrf(num, 1, 0, tempBuffer);  // 先不带小数点
    
    // 检查是否是整数
    if (num == (int)num) {
        strcpy(buffer, tempBuffer);
    } else {
        // 对于小数，最多显示6位有效数字
        dtostrf(num, 1, 6, tempBuffer);
        // 删除尾部多余的0
        char *p = tempBuffer + strlen(tempBuffer) - 1;
        while (*p == '0' && p > tempBuffer) {
            p--;
        }
        if (*p == '.') p--;
        *(p + 1) = 0;
        strcpy(buffer, tempBuffer);
    }
}

void Calculator::updateDisplay() {
    // 清除整个显示区域
    _gfx->fillScreen(BLACK);
    
    // 显示历史记录（使用灰色）
    if (_hasHistory) {
        _gfx->setTextSize(2);  // 历史记录使用较小字体
        _gfx->setTextColor(0x7BEF);  // 使用灰色
        _gfx->setCursor(10, 20);  // 调整位置
        _gfx->print(_historyBuffer);
    }
    
    // 显示当前输入（使用大号白色字体）
    _gfx->setTextSize(5);  // 增大主显示字体
    _gfx->setTextColor(WHITE);
    _gfx->setCursor(10, 70);  // 调整位置
    
    // 构建显示字符串
    char displayStr[64] = {0};
    strcpy(displayStr, _displayBuffer);
    
    // 添加运算符（如果有）
    if (_currentOperator != 0 && !_newNumber) {
        char opStr[2] = {_currentOperator, 0};
        strcat(displayStr, " ");
        strcat(displayStr, opStr);
    }
    
    // 显示文本
    _gfx->print(displayStr);
}

void Calculator::appendNumber(char num) {
    if(_newNumber) {
        _bufferIndex = 0;
        memset(_displayBuffer, 0, sizeof(_displayBuffer));
        _newNumber = false;
    }
    
    if(_bufferIndex < sizeof(_displayBuffer) - 1) {
        _displayBuffer[_bufferIndex++] = num;
        _displayBuffer[_bufferIndex] = 0;
        _currentNumber = atof(_displayBuffer);
        updateDisplay();
    }
}

void Calculator::appendDecimalPoint() {
    if(!_hasDecimalPoint && _bufferIndex < sizeof(_displayBuffer) - 1) {
        if(_newNumber) {
            _bufferIndex = 0;
            memset(_displayBuffer, 0, sizeof(_displayBuffer));
            _displayBuffer[_bufferIndex++] = '0';
            _newNumber = false;
        }
        _displayBuffer[_bufferIndex++] = '.';
        _displayBuffer[_bufferIndex] = 0;
        _hasDecimalPoint = true;
        updateDisplay();
    }
}

void Calculator::setOperator(const char* op) {
    if(_currentOperator != 0) {
        // 如果已经有运算符，先计算
        calculate();
    }
    
    // 设置新的运算符
    if(strcmp(op, "ADD") == 0) _currentOperator = '+';
    else if(strcmp(op, "SUB") == 0) _currentOperator = '-';
    else if(strcmp(op, "MUL") == 0) _currentOperator = '*';
    else if(strcmp(op, "DIV") == 0) _currentOperator = '/';
    
    if(!_hasHistory) {
        _previousNumber = _currentNumber;
    }
    _newNumber = true;
    updateDisplay();
}

void Calculator::calculate() {
    if(_currentOperator && !_newNumber) {
        float result = performCalculation(_previousNumber, _currentNumber, _currentOperator);
        
        // 构建历史记录（完整的计算式）
        char numStr1[32], numStr2[32];
        formatNumber(_previousNumber, numStr1);
        formatNumber(_currentNumber, numStr2);
        snprintf(_historyBuffer, sizeof(_historyBuffer), 
                "%s %c %s =", 
                numStr1, _currentOperator, numStr2);
        _hasHistory = true;
        
        // 设置结果
        _currentNumber = result;
        formatNumber(result, _displayBuffer);
        _bufferIndex = strlen(_displayBuffer);
        _newNumber = true;
        _currentOperator = 0;
        
        updateDisplay();
    }
}

float Calculator::performCalculation(float num1, float num2, char op) {
    switch(op) {
        case '+': return num1 + num2;
        case '-': return num1 - num2;
        case '*': return num1 * num2;
        case '/': return num2 != 0 ? num1 / num2 : 0;
        default: return num2;
    }
}

void Calculator::deleteLastChar() {
    if(_bufferIndex > 0) {
        if(_displayBuffer[_bufferIndex - 1] == '.') {
            _hasDecimalPoint = false;
        }
        _displayBuffer[--_bufferIndex] = 0;
        _currentNumber = atof(_displayBuffer);
        updateDisplay();
    }
}

void Calculator::toggleSign() {
    _currentNumber = -_currentNumber;
    formatNumber(_currentNumber, _displayBuffer);
    _bufferIndex = strlen(_displayBuffer);
    updateDisplay();
}

void Calculator::clearAll() {
    memset(_displayBuffer, 0, sizeof(_displayBuffer));
    memset(_historyBuffer, 0, sizeof(_historyBuffer));
    _bufferIndex = 0;
    _currentNumber = 0;
    _previousNumber = 0;
    _currentOperator = 0;
    _newNumber = true;
    _hasDecimalPoint = false;
    _hasHistory = false;
    updateDisplay();
}

float Calculator::getResult() {
    return _currentNumber;
}

// Calculator.cpp 中的析构函数定义
Calculator::~Calculator() {
    if (_gfx) {
        delete _gfx;
        _gfx = nullptr;
    }
}

void Calculator::onKeyInput(uint8_t keyPosition) {
    // 如果是同一个键且已经处理过，忽略此次输入
    if (keyPosition == _lastKeyPressed && _keyProcessed) {
        return;
    }
    
    // 更新最后按下的键
    _lastKeyPressed = keyPosition;
    
    // 处理按键
    for(int i = 0; i < sizeof(KEY_FUNCTIONS)/sizeof(KEY_FUNCTIONS[0]); i++) {
        if(KEY_FUNCTIONS[i].position == keyPosition) {
            switch(KEY_FUNCTIONS[i].type) {
                case KEY_NUMBER:
                    appendNumber(KEY_FUNCTIONS[i].label[0]);
                    break;
                    
                case KEY_OPERATOR:
                    if(strcmp(KEY_FUNCTIONS[i].label, "PCT") == 0) {
                        _currentNumber *= 0.01;
                        formatNumber(_currentNumber, _displayBuffer);
                        updateDisplay();
                    } else {
                        setOperator(KEY_FUNCTIONS[i].label);
                    }
                    break;
                    
                case KEY_FUNCTION:
                    if(strcmp(KEY_FUNCTIONS[i].label, "C") == 0) {
                        clearAll();
                    } else if(strcmp(KEY_FUNCTIONS[i].label, "DEL") == 0) {
                        deleteLastChar();
                    } else if(strcmp(KEY_FUNCTIONS[i].label, "+/-") == 0) {
                        toggleSign();
                    } else if(strcmp(KEY_FUNCTIONS[i].label, "EQ") == 0) {
                        calculate();
                    }
                    break;
                    
                case KEY_DECIMAL:
                    appendDecimalPoint();
                    break;
            }
            _keyProcessed = true;
            break;
        }
    }
}