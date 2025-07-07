/**
 * @file CalculationEngine.h
 * @brief 可扩展的计算引擎系统
 * @details 提供高精度计算和可扩展的运算功能
 * 
 * 功能特性：
 * - 基础四则运算（+, -, *, /）
 * - 高级数学函数（平方根、平方、倒数等）
 * - 高精度计算支持
 * - 错误检测和处理
 * - 可扩展的计算功能
 * - 表达式解析和求值
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef CALCULATION_ENGINE_H
#define CALCULATION_ENGINE_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include "Logger.h"
#include "CalculatorCore.h"

// 简化精度系统，只保留标准精度

/**
 * @brief 简化的计算结果结构
 */
struct CalculationResult {
    double value;                   ///< 计算结果值
    bool isValid;                   ///< 结果是否有效
    CalculatorError error;          ///< 错误类型
};

// 简化的数学常量，只保留必要的
namespace MathConstants {
    constexpr double MATH_PI = 3.141592653589793;
}

// 复杂的函数系统已被移除，使用简化的基本运算

/**
 * @brief 基础计算引擎类
 */
class CalculationEngine {
public:
    /**
     * @brief 构造函数
     */
    CalculationEngine();
    
    /**
     * @brief 析构函数
     */
    ~CalculationEngine() = default;
    
    /**
     * @brief 初始化计算引擎
     * @return true 成功，false 失败
     */
    bool begin();
    
    /**
     * @brief 执行二元运算
     * @param left 左操作数
     * @param right 右操作数
     * @param op 运算符
     * @return 计算结果
     */
    CalculationResult calculate(double left, double right, Operator op);
    
    /**
     * @brief 执行一元运算
     * @param operand 操作数
     * @param op 运算符
     * @return 计算结果
     */
    CalculationResult calculate(double operand, Operator op);
    
    /**
     * @brief 检查数字是否有效
     * @param number 要检查的数字
     * @return true 有效，false 无效
     */
    bool isValidNumber(double number) const;

private:
    // 简化的私有成员变量
    
    /**
     * @brief 执行基础二元运算
     * @param left 左操作数
     * @param right 右操作数
     * @param op 运算符
     * @return 计算结果
     */
    CalculationResult performBasicOperation(double left, double right, Operator op);
    
    /**
     * @brief 执行一元运算
     * @param operand 操作数
     * @param op 运算符
     * @return 计算结果
     */
    CalculationResult performUnaryOperation(double operand, Operator op);
    
    /**
     * @brief 检查计算结果的有效性
     * @param result 计算结果
     * @return 错误类型
     */
    CalculatorError validateResult(double result) const;
    
    /**
     * @brief 创建错误结果
     * @param error 错误类型
     * @return 错误结果
     */
    CalculationResult createErrorResult(CalculatorError error) const;
    
    /**
     * @brief 创建成功结果
     * @param value 计算值
     * @return 成功结果
     */
    CalculationResult createSuccessResult(double value) const;
};

// 复杂的数学函数类已被移除，使用简化的基本运算

#endif // CALCULATION_ENGINE_H