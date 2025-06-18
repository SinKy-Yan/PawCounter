/**
 * @file CalculatorDisplay.h
 * @brief 计算器显示管理系统
 * @details 统一管理LCD屏幕和串口终端的显示输出
 * 
 * 功能特性：
 * - 双重显示输出（LCD + 串口）
 * - 多行显示布局管理
 * - 数字格式化和单位显示
 * - 历史记录滚动显示
 * - 状态指示器
 * - 可配置的显示主题
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef CALCULATOR_DISPLAY_H
#define CALCULATOR_DISPLAY_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <vector>
#include "Logger.h"
#include "CalculatorCore.h"

/**
 * @brief 显示区域枚举
 */
enum class DisplayArea {
    MAIN_DISPLAY = 0,       ///< 主显示区域（当前数字/结果）
    EXPRESSION,             ///< 表达式显示区域
    HISTORY,                ///< 历史记录区域
    STATUS,                 ///< 状态栏区域
    MODE_INDICATOR,         ///< 模式指示器
    MEMORY_INDICATOR,       ///< 内存指示器
    ERROR_MESSAGE           ///< 错误消息区域
};

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
     * @param history 历史记录
     * @param state 计算器状态
     */
    virtual void updateDisplay(const String& number, 
                              const String& expression,
                              const std::vector<CalculationHistory>& history,
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

/**
 * @brief LCD显示管理器
 */
class LCDDisplay : public CalculatorDisplay {
public:
    /**
     * @brief 构造函数
     * @param gfx Arduino_GFX显示对象指针
     */
    explicit LCDDisplay(Arduino_GFX* gfx);
    
    /**
     * @brief 析构函数
     */
    ~LCDDisplay() override = default;
    
    // 实现基类接口
    bool begin() override;
    void clear() override;
    void updateDisplay(const String& number, 
                      const String& expression,
                      const std::vector<CalculationHistory>& history,
                      CalculatorState state) override;
    void showError(CalculatorError error, const String& message) override;
    void showStatus(const String& message) override;
    void setTheme(const DisplayTheme& theme) override;
    void setNumberFormat(const NumberFormat& format) override;
    void setUnitDisplay(const UnitDisplay& unitDisplay) override;

private:
    Arduino_GFX* _gfx;                  ///< 显示对象
    DisplayTheme _theme;                ///< 当前主题
    NumberFormat _numberFormat;         ///< 数字格式
    UnitDisplay _unitDisplay;           ///< 单位显示配置
    
    uint16_t _displayWidth;             ///< 显示宽度
    uint16_t _displayHeight;            ///< 显示高度
    
    /**
     * @brief 绘制主数字区域
     * @param number 要显示的数字
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    void drawMainNumber(const String& number, uint16_t x, uint16_t y, 
                       uint16_t width, uint16_t height);
    
    /**
     * @brief 绘制表达式区域
     * @param expression 表达式字符串
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    void drawExpression(const String& expression, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height);
    
    /**
     * @brief 绘制历史记录区域
     * @param history 历史记录
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    void drawHistory(const std::vector<CalculationHistory>& history,
                    uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    
    /**
     * @brief 绘制状态栏
     * @param state 计算器状态
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    void drawStatusBar(CalculatorState state, uint16_t x, uint16_t y,
                      uint16_t width, uint16_t height);
    
    /**
     * @brief 绘制单位标识
     * @param number 数字
     * @param x X坐标
     * @param y Y坐标
     */
    void drawUnitLabels(const String& number, uint16_t x, uint16_t y);
    
    /**
     * @brief 转换坐标实现180度翻转
     * @param x 原始X坐标
     * @param y 原始Y坐标
     * @param flippedX 翻转后X坐标（输出）
     * @param flippedY 翻转后Y坐标（输出）
     */
    void flipCoordinates180(uint16_t x, uint16_t y, uint16_t& flippedX, uint16_t& flippedY);
    
    /**
     * @brief 格式化数字显示
     * @param number 原始数字
     * @return 格式化后的字符串
     */
    String formatDisplayNumber(const String& number);
    
    /**
     * @brief 获取默认主题
     * @return 默认主题配置
     */
    DisplayTheme getDefaultTheme();
    
    /**
     * @brief 获取默认数字格式
     * @return 默认数字格式
     */
    NumberFormat getDefaultNumberFormat();
};

/**
 * @brief 串口显示管理器
 */
class SerialDisplay : public CalculatorDisplay {
public:
    /**
     * @brief 构造函数
     */
    SerialDisplay();
    
    /**
     * @brief 析构函数
     */
    ~SerialDisplay() override = default;
    
    // 实现基类接口
    bool begin() override;
    void clear() override;
    void updateDisplay(const String& number, 
                      const String& expression,
                      const std::vector<CalculationHistory>& history,
                      CalculatorState state) override;
    void showError(CalculatorError error, const String& message) override;
    void showStatus(const String& message) override;
    void setTheme(const DisplayTheme& theme) override;
    void setNumberFormat(const NumberFormat& format) override;
    void setUnitDisplay(const UnitDisplay& unitDisplay) override;

private:
    NumberFormat _numberFormat;         ///< 数字格式
    UnitDisplay _unitDisplay;           ///< 单位显示配置
    bool _colorSupport;                 ///< 是否支持颜色输出
    
    String _lastNumber;                 ///< 上次显示的数字
    String _lastExpression;             ///< 上次显示的表达式
    
    /**
     * @brief 打印分隔线
     * @param length 长度
     * @param character 字符
     */
    void printSeparator(uint8_t length = 50, char character = '=');
    
    /**
     * @brief 打印带颜色的文本
     * @param text 文本内容
     * @param colorCode ANSI颜色代码
     */
    void printColorText(const String& text, const String& colorCode = "");
    
    /**
     * @brief 格式化串口显示数字
     * @param number 原始数字
     * @return 格式化后的字符串
     */
    String formatSerialNumber(const String& number);
    
    /**
     * @brief 显示单位标识
     * @param number 数字
     */
    void printUnitLabels(const String& number);
};

/**
 * @brief 双重显示管理器（同时支持LCD和串口）
 */
class DualDisplay : public CalculatorDisplay {
public:
    /**
     * @brief 构造函数
     * @param lcdDisplay LCD显示管理器
     * @param serialDisplay 串口显示管理器
     */
    DualDisplay(std::shared_ptr<LCDDisplay> lcdDisplay,
                std::shared_ptr<SerialDisplay> serialDisplay);
    
    /**
     * @brief 析构函数
     */
    ~DualDisplay() override = default;
    
    // 实现基类接口
    bool begin() override;
    void clear() override;
    void updateDisplay(const String& number, 
                      const String& expression,
                      const std::vector<CalculationHistory>& history,
                      CalculatorState state) override;
    void showError(CalculatorError error, const String& message) override;
    void showStatus(const String& message) override;
    void setTheme(const DisplayTheme& theme) override;
    void setNumberFormat(const NumberFormat& format) override;
    void setUnitDisplay(const UnitDisplay& unitDisplay) override;

private:
    std::shared_ptr<LCDDisplay> _lcdDisplay;        ///< LCD显示管理器
    std::shared_ptr<SerialDisplay> _serialDisplay;  ///< 串口显示管理器
};

// ANSI颜色代码常量
namespace ANSIColors {
    const String RESET = "\033[0m";
    const String ANSI_BLACK = "\033[30m";
    const String ANSI_RED = "\033[31m";
    const String ANSI_GREEN = "\033[32m";
    const String ANSI_YELLOW = "\033[33m";
    const String ANSI_BLUE = "\033[34m";
    const String ANSI_MAGENTA = "\033[35m";
    const String ANSI_CYAN = "\033[36m";
    const String ANSI_WHITE = "\033[37m";
    const String BRIGHT_RED = "\033[91m";
    const String BRIGHT_GREEN = "\033[92m";
    const String BRIGHT_YELLOW = "\033[93m";
    const String BRIGHT_BLUE = "\033[94m";
    const String BRIGHT_MAGENTA = "\033[95m";
    const String BRIGHT_CYAN = "\033[96m";
    const String BRIGHT_WHITE = "\033[97m";
}

#endif // CALCULATOR_DISPLAY_H