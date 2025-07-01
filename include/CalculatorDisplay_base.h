/**
 * @file CalculatorDisplay_base.h
 * @brief 计算器显示管理器基类
 * @details 抽象基类定义，供显示适配器继承使用
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef CALCULATOR_DISPLAY_BASE_H
#define CALCULATOR_DISPLAY_BASE_H

#include <Arduino.h>
#include <vector>
#include "CalculatorCore.h"

/**
 * @brief 数字格式化选项
 */
struct NumberFormat {
    bool useThousandsSeparator;     ///< 使用千位分隔符
    bool showTrailingZeros;         ///< 显示尾随零
    uint8_t decimalPlaces;          ///< 小数位数
    bool scientificNotation;        ///< 科学计数法
    bool showSign;                  ///< 显示正负号
    String currency;                ///< 货币符号
    String thousandsSeparator;      ///< 千位分隔符
    String decimalSeparator;        ///< 小数分隔符
};

/**
 * @brief 显示主题配置
 */
struct DisplayTheme {
    // 颜色配置
    uint16_t backgroundColor;       ///< 背景色
    uint16_t textColor;            ///< 文本颜色
    uint16_t expressionColor;      ///< 表达式颜色
    uint16_t resultColor;          ///< 结果颜色
    uint16_t errorColor;           ///< 错误颜色
    uint16_t statusColor;          ///< 状态颜色
    uint16_t borderColor;          ///< 边框颜色
    
    // 字体配置
    uint8_t mainFontSize;          ///< 主字体大小
    uint8_t expressionFontSize;    ///< 表达式字体大小
    uint8_t statusFontSize;        ///< 状态字体大小
    
    // 布局配置
    uint8_t padding;               ///< 内边距
    uint8_t lineSpacing;           ///< 行间距
    bool showBorders;              ///< 显示边框
};

/**
 * @brief 单位显示配置（用于财务等专业模式）
 */
struct UnitDisplay {
    bool enabled;                   ///< 是否启用单位显示
    std::vector<String> unitLabels; ///< 单位标签（如："个", "十", "百", "千", "万"）
    std::vector<uint32_t> unitValues; ///< 单位数值（如：1, 10, 100, 1000, 10000）
    uint16_t unitColor;            ///< 单位显示颜色
    uint8_t unitFontSize;          ///< 单位字体大小
};

/**
 * @brief 显示管理器抽象基类
 */
class CalculatorDisplay {
public:
    /**
     * @brief 构造函数
     */
    CalculatorDisplay() = default;
    
    /**
     * @brief 析构函数
     */
    virtual ~CalculatorDisplay() = default;
    
    /**
     * @brief 初始化显示系统
     * @return true 成功，false 失败
     */
    virtual bool begin() = 0;
    
    /**
     * @brief 清屏
     */
    virtual void clear() = 0;
    
    /**
     * @brief 更新显示内容
     * @param number 当前数字
     * @param expression 当前表达式
     * @param state 计算器状态
     */
    virtual void updateDisplay(const String& number, 
                              const String& expression,
                              CalculatorState state) = 0;
    
    /**
     * @brief 显示错误消息
     * @param error 错误类型
     * @param message 错误消息
     */
    virtual void showError(CalculatorError error, const String& message) = 0;
    
    /**
     * @brief 显示状态信息
     * @param message 状态消息
     */
    virtual void showStatus(const String& message) = 0;
    
    /**
     * @brief 设置显示主题
     * @param theme 主题配置
     */
    virtual void setTheme(const DisplayTheme& theme) = 0;
    
    /**
     * @brief 设置数字格式
     * @param format 格式配置
     */
    virtual void setNumberFormat(const NumberFormat& format) = 0;
    
    /**
     * @brief 设置单位显示
     * @param unitDisplay 单位显示配置
     */
    virtual void setUnitDisplay(const UnitDisplay& unitDisplay) = 0;
};

#endif // CALCULATOR_DISPLAY_BASE_H