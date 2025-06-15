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

/**
 * @brief 计算精度级别
 */
enum class PrecisionLevel {
    STANDARD = 0,           ///< 标准精度（双精度浮点）
    HIGH_PRECISION,         ///< 高精度（扩展精度）
    FINANCIAL,              ///< 财务精度（固定小数位）
    SCIENTIFIC              ///< 科学精度（任意精度）
};

/**
 * @brief 计算结果结构
 */
struct CalculationResult {
    double value;                   ///< 计算结果值
    bool isValid;                   ///< 结果是否有效
    CalculatorError error;          ///< 错误类型
    String errorMessage;            ///< 错误消息
    PrecisionLevel precision;       ///< 使用的精度级别
    uint32_t calculationTime;       ///< 计算耗时（微秒）
};

/**
 * @brief 数学常量
 */
namespace MathConstants {
    constexpr double MATH_PI = 3.141592653589793238462643383279502884;
    constexpr double E = 2.718281828459045235360287471352662498;
    constexpr double SQRT2 = 1.414213562373095048801688724209698079;
    constexpr double LN2 = 0.693147180559945309417232121458176568;
    constexpr double LN10 = 2.302585092994045684017991454684364208;
    constexpr double GOLDEN_RATIO = 1.618033988749894848204586834365638118;
}

/**
 * @brief 计算函数接口
 */
class CalculationFunction {
public:
    /**
     * @brief 构造函数
     * @param name 函数名称
     * @param description 函数描述
     */
    CalculationFunction(const String& name, const String& description)
        : _name(name), _description(description) {}
    
    /**
     * @brief 虚析构函数
     */
    virtual ~CalculationFunction() = default;
    
    /**
     * @brief 执行计算
     * @param args 参数列表
     * @return 计算结果
     */
    virtual CalculationResult calculate(const std::vector<double>& args) = 0;
    
    /**
     * @brief 获取函数名称
     * @return 函数名称
     */
    const String& getName() const { return _name; }
    
    /**
     * @brief 获取函数描述
     * @return 函数描述
     */
    const String& getDescription() const { return _description; }
    
    /**
     * @brief 获取所需参数数量
     * @return 参数数量
     */
    virtual uint8_t getParameterCount() const = 0;

protected:
    String _name;           ///< 函数名称
    String _description;    ///< 函数描述
};

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
     * @brief 计算表达式
     * @param expression 表达式字符串
     * @return 计算结果
     */
    CalculationResult evaluateExpression(const String& expression);
    
    /**
     * @brief 添加自定义函数
     * @param function 函数对象
     * @return true 成功，false 失败
     */
    bool addFunction(std::shared_ptr<CalculationFunction> function);
    
    /**
     * @brief 调用自定义函数
     * @param functionName 函数名称
     * @param args 参数列表
     * @return 计算结果
     */
    CalculationResult callFunction(const String& functionName, 
                                  const std::vector<double>& args);
    
    /**
     * @brief 设置精度级别
     * @param precision 精度级别
     */
    void setPrecisionLevel(PrecisionLevel precision) { _precisionLevel = precision; }
    
    /**
     * @brief 获取精度级别
     * @return 当前精度级别
     */
    PrecisionLevel getPrecisionLevel() const { return _precisionLevel; }
    
    /**
     * @brief 设置角度单位（弧度/度）
     * @param useDegrees true使用度，false使用弧度
     */
    void setAngleMode(bool useDegrees) { _useDegrees = useDegrees; }
    
    /**
     * @brief 检查数字是否有效
     * @param number 要检查的数字
     * @return true 有效，false 无效
     */
    bool isValidNumber(double number) const;
    
    /**
     * @brief 格式化数字（根据精度）
     * @param number 原始数字
     * @return 格式化后的字符串
     */
    String formatNumber(double number) const;
    
    /**
     * @brief 获取支持的函数列表
     * @return 函数名称列表
     */
    std::vector<String> getSupportedFunctions() const;

private:
    PrecisionLevel _precisionLevel;     ///< 当前精度级别
    bool _useDegrees;                   ///< 是否使用度为角度单位
    
    // 自定义函数注册表
    std::vector<std::shared_ptr<CalculationFunction>> _functions;
    
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
     * @brief 角度转弧度
     * @param degrees 角度值
     * @return 弧度值
     */
    double degreesToRadians(double degrees) const;
    
    /**
     * @brief 弧度转角度
     * @param radians 弧度值
     * @return 角度值
     */
    double radiansToDegrees(double radians) const;
    
    /**
     * @brief 创建错误结果
     * @param error 错误类型
     * @param message 错误消息
     * @return 错误结果
     */
    CalculationResult createErrorResult(CalculatorError error, 
                                       const String& message) const;
    
    /**
     * @brief 创建成功结果
     * @param value 计算值
     * @param calculationTime 计算时间
     * @return 成功结果
     */
    CalculationResult createSuccessResult(double value, uint32_t calculationTime) const;
};

// ============================================================================
// 预定义的数学函数类
// ============================================================================

/**
 * @brief 正弦函数
 */
class SinFunction : public CalculationFunction {
public:
    SinFunction() : CalculationFunction("sin", "Sine function") {}
    CalculationResult calculate(const std::vector<double>& args) override;
    uint8_t getParameterCount() const override { return 1; }
};

/**
 * @brief 余弦函数
 */
class CosFunction : public CalculationFunction {
public:
    CosFunction() : CalculationFunction("cos", "Cosine function") {}
    CalculationResult calculate(const std::vector<double>& args) override;
    uint8_t getParameterCount() const override { return 1; }
};

/**
 * @brief 正切函数
 */
class TanFunction : public CalculationFunction {
public:
    TanFunction() : CalculationFunction("tan", "Tangent function") {}
    CalculationResult calculate(const std::vector<double>& args) override;
    uint8_t getParameterCount() const override { return 1; }
};

/**
 * @brief 自然对数函数
 */
class LogFunction : public CalculationFunction {
public:
    LogFunction() : CalculationFunction("ln", "Natural logarithm") {}
    CalculationResult calculate(const std::vector<double>& args) override;
    uint8_t getParameterCount() const override { return 1; }
};

/**
 * @brief 常用对数函数
 */
class Log10Function : public CalculationFunction {
public:
    Log10Function() : CalculationFunction("log10", "Base-10 logarithm") {}
    CalculationResult calculate(const std::vector<double>& args) override;
    uint8_t getParameterCount() const override { return 1; }
};

/**
 * @brief 指数函数
 */
class ExpFunction : public CalculationFunction {
public:
    ExpFunction() : CalculationFunction("exp", "Exponential function") {}
    CalculationResult calculate(const std::vector<double>& args) override;
    uint8_t getParameterCount() const override { return 1; }
};

/**
 * @brief 幂函数
 */
class PowerFunction : public CalculationFunction {
public:
    PowerFunction() : CalculationFunction("pow", "Power function (x^y)") {}
    CalculationResult calculate(const std::vector<double>& args) override;
    uint8_t getParameterCount() const override { return 2; }
};

/**
 * @brief 绝对值函数
 */
class AbsFunction : public CalculationFunction {
public:
    AbsFunction() : CalculationFunction("abs", "Absolute value") {}
    CalculationResult calculate(const std::vector<double>& args) override;
    uint8_t getParameterCount() const override { return 1; }
};

/**
 * @brief 阶乘函数
 */
class FactorialFunction : public CalculationFunction {
public:
    FactorialFunction() : CalculationFunction("fact", "Factorial function") {}
    CalculationResult calculate(const std::vector<double>& args) override;
    uint8_t getParameterCount() const override { return 1; }
};

#endif // CALCULATION_ENGINE_H