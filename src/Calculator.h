#ifndef _CALCULATOR_H_
#define _CALCULATOR_H_

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// 按键定义结构体和映射表声明
struct KeyFunction {
    uint8_t position;
    const char* label;
    uint8_t type;
};

// 按键类型定义
enum KeyType {
    KEY_NUMBER = 0,
    KEY_OPERATOR = 1,
    KEY_FUNCTION = 2,
    KEY_RESERVED = 3,
    KEY_DECIMAL = 4
};

// 按键映射表声明
extern const KeyFunction KEY_FUNCTIONS[];

class Calculator {
public:
    Calculator(Arduino_GFX *gfx);  
    ~Calculator();  // 析构函数声明
    void onKeyInput(uint8_t keyPosition);
    void clearDisplay();
    void clearAll();
    float getResult();

private:
    Arduino_GFX *_gfx;
    char _displayBuffer[32];
    char _historyBuffer[32];
    int _bufferIndex;

    float _currentNumber;
    float _previousNumber;
    char _currentOperator;
    bool _newNumber;
    bool _hasDecimalPoint;
    bool _hasHistory;

    // 按键重复控制
    uint32_t _lastKeyPressTime;   // 上次按键时间
    uint8_t _lastKeyPressed;      // 上次按下的键位
    bool _keyProcessed;           // 当前按键是否已处理
    bool _zeroKeyPressed;         // 0键是否按下（用于组合键）

    // 长按检测相关的时间戳
    uint32_t _powerKeyPressStart;  // 记录电源键的按下时间
    uint32_t _btKeyPressStart;     // 记录蓝牙键的按下时间

    void appendNumber(char num);
    void appendDecimalPoint();
    void setOperator(const char* op);
    void deleteLastChar();
    void toggleSign();
    void updateDisplay();
    void calculate();
    float performCalculation(float num1, float num2, char op);
    void formatNumber(float num, char* buffer);
    void resetKeyState() { _keyProcessed = false; }  // 添加此方法
};

#endif