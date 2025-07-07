/**
 * @file CalculationEngine.cpp
 * @brief 简化的计算引擎实现
 * 
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "CalculationEngine.h"
#include <cmath>

CalculationEngine::CalculationEngine() {
    CALC_LOG_I("简化计算引擎已初始化");
}

bool CalculationEngine::begin() {
    CALC_LOG_I("启动简化计算引擎");
    // 不再注册复杂的数学函数，只支持基本运算
    CALC_LOG_I("简化计算引擎就绪，支持基本四则运算");
    return true;
}

CalculationResult CalculationEngine::calculate(double left, double right, Operator op) {
    CALC_LOG_V("计算: %.6f %d %.6f", left, (int)op, right);
    
    CalculationResult result = performBasicOperation(left, right, op);
    
    CALC_LOG_V("结果: %.6f (有效: %s)", 
               result.value, result.isValid ? "true" : "false");
    
    return result;
}

CalculationResult CalculationEngine::calculate(double operand, Operator op) {
    CALC_LOG_V("一元运算: %.6f %d", operand, (int)op);
    
    CalculationResult result = performUnaryOperation(operand, op);
    
    return result;
}

// 数字格式化功能已简化，使用NumberFormatter统一处理
// 函数列表功能已移除

bool CalculationEngine::isValidNumber(double number) const {
    return !isnan(number) && !isinf(number);
}

CalculationResult CalculationEngine::performBasicOperation(double left, double right, Operator op) {
    double result = 0.0;
    
    switch (op) {
        case Operator::ADD:
            result = left + right;
            break;
            
        case Operator::SUBTRACT:
            result = left - right;
            break;
            
        case Operator::MULTIPLY:
            result = left * right;
            break;
            
        case Operator::DIVIDE:
            if (abs(right) < 1e-10) {
                return createErrorResult(CalculatorError::DIVISION_BY_ZERO);
            }
            result = left / right;
            break;
            
        case Operator::PERCENT:
            result = left * right / 100.0;
            break;
            
        default:
            return createErrorResult(CalculatorError::INVALID_OPERATION);
    }
    
    CalculatorError error = validateResult(result);
    if (error != CalculatorError::NONE) {
        return createErrorResult(error);
    }
    
    return createSuccessResult(result);
}

CalculationResult CalculationEngine::performUnaryOperation(double operand, Operator op) {
    double result = 0.0;
    
    switch (op) {
        case Operator::SQUARE_ROOT:
            if (operand < 0) {
                return createErrorResult(CalculatorError::INVALID_OPERATION);
            }
            result = sqrt(operand);
            break;
            
        case Operator::SQUARE:
            result = operand * operand;
            break;
            
        case Operator::RECIPROCAL:
            if (abs(operand) < 1e-10) {
                return createErrorResult(CalculatorError::DIVISION_BY_ZERO);
            }
            result = 1.0 / operand;
            break;
            
        default:
            return createErrorResult(CalculatorError::INVALID_OPERATION);
    }
    
    CalculatorError error = validateResult(result);
    if (error != CalculatorError::NONE) {
        return createErrorResult(error);
    }
    
    return createSuccessResult(result);
}

CalculatorError CalculationEngine::validateResult(double result) const {
    if (isnan(result)) {
        return CalculatorError::INVALID_OPERATION;
    }
    if (isinf(result)) {
        return result > 0 ? CalculatorError::OVERFLOW : CalculatorError::UNDERFLOW;
    }
    return CalculatorError::NONE;
}

CalculationResult CalculationEngine::createErrorResult(CalculatorError error) const {
    CalculationResult result;
    result.value = 0.0;
    result.isValid = false;
    result.error = error;
    return result;
}

CalculationResult CalculationEngine::createSuccessResult(double value) const {
    CalculationResult result;
    result.value = value;
    result.isValid = true;
    result.error = CalculatorError::NONE;
    return result;
}

// 复杂的数学函数实现已被移除，只保留基本计算功能