/**
 * @file CalculationEngine.cpp
 * @brief 计算引擎实现
 * 
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "CalculationEngine.h"
#include <cmath>

CalculationEngine::CalculationEngine()
    : _precisionLevel(PrecisionLevel::STANDARD)
    , _useDegrees(true) {
    
    CALC_LOG_I("Calculation engine initialized");
}

bool CalculationEngine::begin() {
    CALC_LOG_I("Starting calculation engine");
    
    // 注册基础数学函数
    addFunction(std::make_shared<SinFunction>());
    addFunction(std::make_shared<CosFunction>());
    addFunction(std::make_shared<TanFunction>());
    addFunction(std::make_shared<LogFunction>());
    addFunction(std::make_shared<Log10Function>());
    addFunction(std::make_shared<ExpFunction>());
    addFunction(std::make_shared<PowerFunction>());
    addFunction(std::make_shared<AbsFunction>());
    addFunction(std::make_shared<FactorialFunction>());
    
    CALC_LOG_I("Calculation engine ready with %d functions", _functions.size());
    return true;
}

CalculationResult CalculationEngine::calculate(double left, double right, Operator op) {
    uint32_t startTime = micros();
    
    CALC_LOG_V("Calculating: %.6f %d %.6f", left, (int)op, right);
    
    CalculationResult result = performBasicOperation(left, right, op);
    result.calculationTime = micros() - startTime;
    result.precision = _precisionLevel;
    
    CALC_LOG_V("Result: %.6f (valid: %s, time: %d μs)", 
               result.value, result.isValid ? "true" : "false", result.calculationTime);
    
    return result;
}

CalculationResult CalculationEngine::calculate(double operand, Operator op) {
    uint32_t startTime = micros();
    
    CALC_LOG_V("Calculating unary: %.6f %d", operand, (int)op);
    
    CalculationResult result = performUnaryOperation(operand, op);
    result.calculationTime = micros() - startTime;
    result.precision = _precisionLevel;
    
    return result;
}

CalculationResult CalculationEngine::evaluateExpression(const String& expression) {
    // 简化的表达式解析（这里只是示例）
    CALC_LOG_D("Evaluating expression: %s", expression.c_str());
    
    // 实际实现需要完整的表达式解析器
    return createErrorResult(CalculatorError::INVALID_OPERATION, "Expression parsing not implemented");
}

bool CalculationEngine::addFunction(std::shared_ptr<CalculationFunction> function) {
    if (!function) {
        return false;
    }
    
    _functions.push_back(function);
    CALC_LOG_D("Added function: %s", function->getName().c_str());
    return true;
}

CalculationResult CalculationEngine::callFunction(const String& functionName, 
                                                 const std::vector<double>& args) {
    
    for (auto& func : _functions) {
        if (func->getName() == functionName) {
            CALC_LOG_V("Calling function: %s with %d args", functionName.c_str(), args.size());
            return func->calculate(args);
        }
    }
    
    CALC_LOG_W("Function not found: %s", functionName.c_str());
    return createErrorResult(CalculatorError::INVALID_OPERATION, "Function not found");
}

bool CalculationEngine::isValidNumber(double number) const {
    return !isnan(number) && !isinf(number);
}

String CalculationEngine::formatNumber(double number) const {
    if (!isValidNumber(number)) {
        return "Error";
    }
    
    switch (_precisionLevel) {
        case PrecisionLevel::STANDARD:
            return String(number, 6);
        case PrecisionLevel::HIGH_PRECISION:
            return String(number, 12);
        case PrecisionLevel::FINANCIAL:
            return String(number, 2);
        case PrecisionLevel::SCIENTIFIC:
            return String(number, 10);
        default:
            return String(number);
    }
}

std::vector<String> CalculationEngine::getSupportedFunctions() const {
    std::vector<String> functionList;
    for (const auto& func : _functions) {
        functionList.push_back(func->getName());
    }
    return functionList;
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
                return createErrorResult(CalculatorError::DIVISION_BY_ZERO, "Division by zero");
            }
            result = left / right;
            break;
            
        case Operator::PERCENT:
            result = left * right / 100.0;
            break;
            
        default:
            return createErrorResult(CalculatorError::INVALID_OPERATION, "Unsupported operation");
    }
    
    CalculatorError error = validateResult(result);
    if (error != CalculatorError::NONE) {
        return createErrorResult(error, "Calculation error");
    }
    
    return createSuccessResult(result, 0);
}

CalculationResult CalculationEngine::performUnaryOperation(double operand, Operator op) {
    double result = 0.0;
    
    switch (op) {
        case Operator::SQUARE_ROOT:
            if (operand < 0) {
                return createErrorResult(CalculatorError::INVALID_OPERATION, "Square root of negative number");
            }
            result = sqrt(operand);
            break;
            
        case Operator::SQUARE:
            result = operand * operand;
            break;
            
        case Operator::RECIPROCAL:
            if (abs(operand) < 1e-10) {
                return createErrorResult(CalculatorError::DIVISION_BY_ZERO, "Reciprocal of zero");
            }
            result = 1.0 / operand;
            break;
            
        default:
            return createErrorResult(CalculatorError::INVALID_OPERATION, "Unsupported unary operation");
    }
    
    CalculatorError error = validateResult(result);
    if (error != CalculatorError::NONE) {
        return createErrorResult(error, "Calculation error");
    }
    
    return createSuccessResult(result, 0);
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

double CalculationEngine::degreesToRadians(double degrees) const {
    return degrees * MathConstants::MATH_PI / 180.0;
}

double CalculationEngine::radiansToDegrees(double radians) const {
    return radians * 180.0 / MathConstants::MATH_PI;
}

CalculationResult CalculationEngine::createErrorResult(CalculatorError error, 
                                                      const String& message) const {
    CalculationResult result;
    result.value = 0.0;
    result.isValid = false;
    result.error = error;
    result.errorMessage = message;
    result.precision = _precisionLevel;
    result.calculationTime = 0;
    return result;
}

CalculationResult CalculationEngine::createSuccessResult(double value, uint32_t calculationTime) const {
    CalculationResult result;
    result.value = value;
    result.isValid = true;
    result.error = CalculatorError::NONE;
    result.errorMessage = "";
    result.precision = _precisionLevel;
    result.calculationTime = calculationTime;
    return result;
}

// ============================================================================
// 数学函数实现
// ============================================================================

CalculationResult SinFunction::calculate(const std::vector<double>& args) {
    if (args.size() != 1) {
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Sin function requires exactly 1 argument";
        return result;
    }
    
    double angle = args[0];
    // 假设输入是度，转换为弧度
    double radians = angle * MathConstants::MATH_PI / 180.0;
    double result = sin(radians);
    
    CalculationResult calcResult;
    calcResult.value = result;
    calcResult.isValid = true;
    calcResult.error = CalculatorError::NONE;
    return calcResult;
}

CalculationResult CosFunction::calculate(const std::vector<double>& args) {
    if (args.size() != 1) {
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Cos function requires exactly 1 argument";
        return result;
    }
    
    double angle = args[0];
    double radians = angle * MathConstants::MATH_PI / 180.0;
    double result = cos(radians);
    
    CalculationResult calcResult;
    calcResult.value = result;
    calcResult.isValid = true;
    calcResult.error = CalculatorError::NONE;
    return calcResult;
}

CalculationResult TanFunction::calculate(const std::vector<double>& args) {
    if (args.size() != 1) {
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Tan function requires exactly 1 argument";
        return result;
    }
    
    double angle = args[0];
    double radians = angle * MathConstants::MATH_PI / 180.0;
    double result = tan(radians);
    
    CalculationResult calcResult;
    calcResult.value = result;
    calcResult.isValid = true;
    calcResult.error = CalculatorError::NONE;
    return calcResult;
}

CalculationResult LogFunction::calculate(const std::vector<double>& args) {
    if (args.size() != 1) {
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Ln function requires exactly 1 argument";
        return result;
    }
    
    if (args[0] <= 0) {
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Logarithm of non-positive number";
        return result;
    }
    
    CalculationResult result;
    result.value = log(args[0]);
    result.isValid = true;
    result.error = CalculatorError::NONE;
    return result;
}

CalculationResult Log10Function::calculate(const std::vector<double>& args) {
    if (args.size() != 1) {
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Log10 function requires exactly 1 argument";
        return result;
    }
    
    if (args[0] <= 0) {
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Logarithm of non-positive number";
        return result;
    }
    
    CalculationResult result;
    result.value = log10(args[0]);
    result.isValid = true;
    result.error = CalculatorError::NONE;
    return result;
}

CalculationResult ExpFunction::calculate(const std::vector<double>& args) {
    if (args.size() != 1) {
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Exp function requires exactly 1 argument";
        return result;
    }
    
    CalculationResult result;
    result.value = exp(args[0]);
    result.isValid = true;
    result.error = CalculatorError::NONE;
    return result;
}

CalculationResult PowerFunction::calculate(const std::vector<double>& args) {
    if (args.size() != 2) {
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Power function requires exactly 2 arguments";
        return result;
    }
    
    CalculationResult result;
    result.value = pow(args[0], args[1]);
    result.isValid = true;
    result.error = CalculatorError::NONE;
    return result;
}

CalculationResult AbsFunction::calculate(const std::vector<double>& args) {
    if (args.size() != 1) {
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Abs function requires exactly 1 argument";
        return result;
    }
    
    CalculationResult result;
    result.value = abs(args[0]);
    result.isValid = true;
    result.error = CalculatorError::NONE;
    return result;
}

CalculationResult FactorialFunction::calculate(const std::vector<double>& args) {
    if (args.size() != 1) {
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Factorial function requires exactly 1 argument";
        return result;
    }
    
    double n = args[0];
    if (n < 0 || n != floor(n) || n > 170) {  // 170! 是双精度能表示的最大阶乘
        CalculationResult result;
        result.isValid = false;
        result.error = CalculatorError::INVALID_OPERATION;
        result.errorMessage = "Invalid factorial input";
        return result;
    }
    
    double result = 1.0;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    
    CalculationResult calcResult;
    calcResult.value = result;
    calcResult.isValid = true;
    calcResult.error = CalculatorError::NONE;
    return calcResult;
}