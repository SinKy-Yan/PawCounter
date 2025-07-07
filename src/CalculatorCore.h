/**
 * @file CalculatorCore.h
 * @brief 可扩展的计算器核心架构
 * @details 设计了模块化的计算器系统，支持多种计算模式和显示方式
 * 
 * 核心特性：
 * - 可扩展的计算模式系统
 * - 双显示输出（LCD + 串口）
 * - 完整的计算历史追踪
 * - 灵活的数字格式化系统
 * - 插件式的功能扩展
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 2.0
 */

#ifndef CALCULATOR_CORE_H
#define CALCULATOR_CORE_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include "Logger.h"
#include "KeyboardConfig.h"

// 前向声明
class CalcDisplay;
class CalculationEngine;
class NumberFormatter;

// 使用 KeyboardConfig.h 中定义的枚举类型
// 避免重复定义 KeyType 和 Operator

/**
 * @brief 计算状态枚举
 */
enum class CalculatorState {
    INPUT_NUMBER,       ///< 输入数字状态
    INPUT_OPERATOR,     ///< 输入运算符状态
    DISPLAY_RESULT,     ///< 显示结果状态
    ERROR,              ///< 错误状态
    WAITING             ///< 等待状态
};

// 旧的KeyMapping结构已被移除，使用KeyboardConfig系统中的KeyConfig

/**
 * @brief 计算历史记录结构
 */
struct CalculationHistory {
    String expression;          ///< 计算表达式
    double result;              ///< 计算结果
    uint32_t timestamp;         ///< 时间戳
    uint8_t modeId;            ///< 计算模式ID
};

/**
 * @brief 错误类型枚举
 */
enum class CalculatorError {
    NONE = 0,
    DIVISION_BY_ZERO,           ///< 除零错误
    OVERFLOW,                   ///< 溢出错误
    UNDERFLOW,                  ///< 下溢错误
    INVALID_OPERATION,          ///< 无效操作
    SYNTAX_ERROR,               ///< 语法错误
    MEMORY_ERROR                ///< 内存错误
};

/**
 * @brief 计算器核心类
 */
class CalculatorCore {
public:
    /**
     * @brief 构造函数
     */
    CalculatorCore();
    
    /**
     * @brief 析构函数
     */
    ~CalculatorCore();
    
    /**
     * @brief 初始化计算器
     * @return true 成功，false 失败
     */
    bool begin();
    
    /**
     * @brief 处理按键输入
     * @param keyPosition 按键位置 (1-22)
     * @param isLongPress 是否为长按
     * @return true 处理成功，false 处理失败
     */
    bool handleKeyInput(uint8_t keyPosition, bool isLongPress = false);
    
    /**
     * @brief 设置显示管理器
     * @param display 显示管理器指针
     */
    void setDisplay(CalcDisplay* display);
    
    /**
     * @brief 设置计算引擎
     * @param engine 计算引擎指针
     */
    void setCalculationEngine(std::shared_ptr<CalculationEngine> engine);
    
    /**
     * @brief 添加计算模式
     * @param mode 计算模式指针
     * @return 模式ID
     */
    // 模式系统已移除
    // uint8_t addMode(std::shared_ptr<CalculatorMode> mode);
    // bool switchMode(uint8_t modeId);
    
    /**
     * @brief 获取当前模式ID
     * @return 当前模式ID
     */
    // uint8_t getCurrentModeId() const { return _currentModeId; } // 移除模式系统
    
    /**
     * @brief 获取当前状态
     * @return 当前计算器状态
     */
    CalculatorState getState() const { return _state; }
    
    /**
     * @brief 获取当前显示内容
     * @return 显示内容字符串
     */
    String getCurrentDisplay() const { return _currentDisplay; }
    
    /**
     * @brief 获取计算历史
     * @return 计算历史数组
     */
    const std::vector<CalculationHistory>& getHistory() const { return _history; }
    
    /**
     * @brief 清除当前输入
     */
    void clearEntry();
    
    /**
     * @brief 清除所有内容
     */
    void clearAll();
    
    /**
     * @brief 获取最后的错误
     * @return 错误类型
     */
    CalculatorError getLastError() const { return _lastError; }
    
    /**
     * @brief 更新显示
     */
    void updateDisplay();
    
    /**
     * @brief 定期更新
     */
    void update();

private:
    // 核心组件
    CalcDisplay* _display;                              ///< 显示管理器
    std::shared_ptr<CalculationEngine> _engine;         ///< 计算引擎
    
    // 状态管理
    CalculatorState _state;             ///< 当前状态
    // uint8_t _currentModeId;            // 移除模式系统
    CalculatorError _lastError;         ///< 最后的错误
    
    // 输入缓冲
    String _inputBuffer;                ///< 输入缓冲区
    String _currentDisplay;             ///< 当前显示内容
    String _expressionDisplay;          ///< 表达式显示
    
    // 计算状态
    double _currentNumber;              ///< 当前数字
    double _previousNumber;             ///< 上一个数字
    Operator _pendingOperator;          ///< 待处理的运算符
    bool _waitingForOperand;           ///< 是否等待操作数
    bool _hasDecimalPoint;             ///< 是否有小数点
    
    // 历史记录
    std::vector<CalculationHistory> _history;   ///< 计算历史
    size_t _maxHistorySize;            ///< 最大历史记录数
    
    // 内存功能
    double _memoryValue;               ///< 内存值
    bool _hasMemoryValue;              ///< 是否有内存值
    
    // 按键映射系统 (已废弃，由KeyboardConfig代替)
    // static const KeyConfig _keyMappings[];
    // static const size_t _keyMappingsSize;
    
    /**
     * @brief 处理数字输入
     * @param digit 数字字符
     */
    void handleDigitInput(char digit);
    
    /**
     * @brief 处理运算符输入
     * @param op 运算符
     */
    void handleOperatorInput(Operator op);
    
    // 旧的handleFunctionInput(KeyMapping*)方法已被移除
    
    /**
     * @brief 处理功能键输入 (新版)
     * @param keyConfig 按键配置
     */
    void handleFunctionInput(const KeyConfig* keyConfig);
    
    /**
     * @brief 处理清除操作
     */
    void handleClear();
    
    /**
     * @brief 处理退格删除操作
     */
    void handleBackspace();
    
    /**
     * @brief 处理模式切换
     */
    void handleModeSwitch();
    
    /**
     * @brief 执行计算
     * @return true 成功，false 失败
     */
    bool performCalculation();
    
    /**
     * @brief 设置错误状态
     * @param error 错误类型
     */
    void setError(CalculatorError error);
    
    /**
     * @brief 添加到历史记录
     * @param expression 表达式
     * @param result 结果
     */
    void addToHistory(const String& expression, double result);
    
    /**
     * @brief 重置输入状态
     */
    void resetInputState();
    
    // formatNumber方法已被移除，统一使用NumberFormatter::format
};

#endif // CALCULATOR_CORE_H