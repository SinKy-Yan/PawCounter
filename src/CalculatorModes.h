/**
 * @file CalculatorModes.h
 * @brief 计算器模式系统
 * @details 提供可扩展的计算模式支持，包括基本、科学、财务等模式
 * 
 * 功能特性：
 * - 模块化的计算模式设计
 * - 自定义按键映射
 * - 专用显示格式
 * - 模式特定的功能
 * - 动态模式切换
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef CALCULATOR_MODES_H
#define CALCULATOR_MODES_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include "Logger.h"
#include "CalculatorCore.h"
#include "CalculatorDisplay.h"
#include "CalculationEngine.h"

/**
 * @brief 模式类型枚举
 */
enum class ModeType {
    BASIC = 0,              ///< 基本计算模式
    SCIENTIFIC,             ///< 科学计算模式
    FINANCIAL,              ///< 财务计算模式
    PROGRAMMER,             ///< 程序员模式
    STATISTICS,             ///< 统计模式
    UNIT_CONVERTER,         ///< 单位转换模式
    CUSTOM                  ///< 自定义模式
};

/**
 * @brief 模式特定的按键映射
 */
struct ModeKeyMapping {
    uint8_t keyPosition;        ///< 物理按键位置
    String primaryLabel;        ///< 主标签
    String secondaryLabel;      ///< 次标签（长按或组合键）
    KeyType keyType;           ///< 按键类型
    Operator operation;         ///< 对应操作
    String functionName;        ///< 自定义函数名
    bool requiresSecondFunction; ///< 是否需要二次功能
};

/**
 * @brief 模式配置结构
 */
struct ModeConfig {
    String name;                        ///< 模式名称
    String description;                 ///< 模式描述
    ModeType type;                      ///< 模式类型
    PrecisionLevel defaultPrecision;    ///< 默认精度
    NumberFormat numberFormat;          ///< 数字格式
    UnitDisplay unitDisplay;            ///< 单位显示配置
    DisplayTheme theme;                 ///< 显示主题
    std::vector<ModeKeyMapping> keyMappings; ///< 按键映射
    bool supportsMemory;                ///< 是否支持内存功能
    bool supportsHistory;               ///< 是否支持历史记录
    bool supportsSecondFunction;        ///< 是否支持二次功能
};

/**
 * @brief 计算器模式抽象基类
 */
class CalculatorMode {
public:
    /**
     * @brief 构造函数
     * @param config 模式配置
     */
    explicit CalculatorMode(const ModeConfig& config);
    
    /**
     * @brief 虚析构函数
     */
    virtual ~CalculatorMode() = default;
    
    /**
     * @brief 初始化模式
     * @return true 成功，false 失败
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief 激活模式
     * @param display 显示管理器
     * @param engine 计算引擎
     */
    virtual void activate(std::shared_ptr<CalculatorDisplay> display,
                         std::shared_ptr<CalculationEngine> engine);
    
    /**
     * @brief 去激活模式
     */
    virtual void deactivate();
    
    /**
     * @brief 处理按键输入
     * @param keyPosition 按键位置
     * @param isLongPress 是否长按
     * @param isSecondFunction 是否二次功能
     * @return true 处理成功，false 处理失败
     */
    virtual bool handleKeyInput(uint8_t keyPosition, 
                               bool isLongPress = false,
                               bool isSecondFunction = false) = 0;
    
    /**
     * @brief 更新显示
     */
    virtual void updateDisplay() = 0;
    
    /**
     * @brief 获取模式配置
     * @return 模式配置
     */
    const ModeConfig& getConfig() const { return _config; }
    
    /**
     * @brief 获取模式ID
     * @return 模式ID
     */
    uint8_t getModeId() const { return _modeId; }
    
    /**
     * @brief 设置模式ID
     * @param id 模式ID
     */
    void setModeId(uint8_t id) { _modeId = id; }
    
    /**
     * @brief 获取模式名称
     * @return 模式名称
     */
    const String& getName() const { return _config.name; }
    
    /**
     * @brief 是否处于活动状态
     * @return true 活动，false 非活动
     */
    bool isActive() const { return _isActive; }
    
    /**
     * @brief 获取帮助信息
     * @return 帮助信息字符串
     */
    virtual String getHelpText() const = 0;

protected:
    ModeConfig _config;                             ///< 模式配置
    uint8_t _modeId;                               ///< 模式ID
    bool _isActive;                                ///< 是否活动
    bool _secondFunctionActive;                    ///< 二次功能是否激活
    
    std::shared_ptr<CalculatorDisplay> _display;    ///< 显示管理器
    std::shared_ptr<CalculationEngine> _engine;     ///< 计算引擎
    
    /**
     * @brief 查找按键映射
     * @param keyPosition 按键位置
     * @return 按键映射指针，未找到返回nullptr
     */
    const ModeKeyMapping* findKeyMapping(uint8_t keyPosition) const;
    
    /**
     * @brief 切换二次功能状态
     */
    void toggleSecondFunction() { _secondFunctionActive = !_secondFunctionActive; }
    
    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param format 格式字符串
     * @param ... 参数
     */
    void logMessage(const char* level, const char* format, ...) const;
};

// ============================================================================
// 具体的计算器模式实现
// ============================================================================

/**
 * @brief 基本计算模式
 */
class BasicMode : public CalculatorMode {
public:
    /**
     * @brief 构造函数
     */
    BasicMode();
    
    /**
     * @brief 析构函数
     */
    ~BasicMode() override = default;
    
    // 实现基类接口
    bool initialize() override;
    bool handleKeyInput(uint8_t keyPosition, bool isLongPress = false,
                       bool isSecondFunction = false) override;
    void updateDisplay() override;
    String getHelpText() const override;

private:
    String _currentInput;               ///< 当前输入
    String _currentExpression;          ///< 当前表达式
    double _lastResult;                 ///< 上次结果
    bool _clearOnNextInput;             ///< 下次输入时清除
    
    /**
     * @brief 处理数字输入
     * @param digit 数字字符
     */
    void handleDigit(char digit);
    
    /**
     * @brief 处理运算符输入
     * @param op 运算符
     */
    void handleOperator(Operator op);
    
    /**
     * @brief 处理功能键
     * @param keyPosition 按键位置
     */
    void handleFunction(uint8_t keyPosition);
    
    /**
     * @brief 创建默认按键映射
     * @return 按键映射向量
     */
    static std::vector<ModeKeyMapping> createDefaultKeyMappings();
    
    /**
     * @brief 创建基本模式配置
     * @return 模式配置
     */
    static ModeConfig createBasicConfig();
};

/**
 * @brief 科学计算模式
 */
class ScientificMode : public CalculatorMode {
public:
    /**
     * @brief 构造函数
     */
    ScientificMode();
    
    /**
     * @brief 析构函数
     */
    ~ScientificMode() override = default;
    
    // 实现基类接口
    bool initialize() override;
    bool handleKeyInput(uint8_t keyPosition, bool isLongPress = false,
                       bool isSecondFunction = false) override;
    void updateDisplay() override;
    String getHelpText() const override;

private:
    bool _angleInDegrees;               ///< 角度单位是否为度
    bool _inverseMode;                  ///< 反三角函数模式
    String _pendingFunction;            ///< 待执行的函数
    
    /**
     * @brief 处理科学函数
     * @param functionName 函数名
     */
    void handleScientificFunction(const String& functionName);
    
    /**
     * @brief 切换角度单位
     */
    void toggleAngleMode();
    
    /**
     * @brief 创建科学模式按键映射
     * @return 按键映射向量
     */
    static std::vector<ModeKeyMapping> createScientificKeyMappings();
    
    /**
     * @brief 创建科学模式配置
     * @return 模式配置
     */
    static ModeConfig createScientificConfig();
};

/**
 * @brief 财务计算模式
 */
class FinancialMode : public CalculatorMode {
public:
    /**
     * @brief 构造函数
     */
    FinancialMode();
    
    /**
     * @brief 析构函数
     */
    ~FinancialMode() override = default;
    
    // 实现基类接口
    bool initialize() override;
    bool handleKeyInput(uint8_t keyPosition, bool isLongPress = false,
                       bool isSecondFunction = false) override;
    void updateDisplay() override;
    String getHelpText() const override;
    
    /**
     * @brief 设置金额并更新显示
     * @param amount 金额
     */
    void setAmount(double amount);

private:
    String _currencySymbol;             ///< 货币符号
    uint8_t _decimalPlaces;            ///< 小数位数
    bool _showThousandsSeparator;       ///< 显示千位分隔符
    
    // 财务计算变量
    double _presentValue;               ///< 现值
    double _futureValue;                ///< 终值
    double _payment;                    ///< 支付金额
    double _interestRate;               ///< 利率
    double _periods;                    ///< 期数
    
    /**
     * @brief 计算现值
     * @return 计算结果
     */
    double calculatePresentValue() const;
    
    /**
     * @brief 计算终值
     * @return 计算结果
     */
    double calculateFutureValue() const;
    
    /**
     * @brief 计算月供
     * @return 计算结果
     */
    double calculatePayment() const;
    
    /**
     * @brief 显示单位标识
     * @param number 数字
     */
    void showUnitLabels(double number);
    
    /**
     * @brief 创建财务模式按键映射
     * @return 按键映射向量
     */
    static std::vector<ModeKeyMapping> createFinancialKeyMappings();
    
    /**
     * @brief 创建财务模式单位显示配置
     * @return 单位显示配置
     */
    static UnitDisplay createFinancialUnitDisplay();
    
    /**
     * @brief 创建财务模式配置
     * @return 模式配置
     */
    static ModeConfig createFinancialConfig();
};

/**
 * @brief 程序员模式
 */
class ProgrammerMode : public CalculatorMode {
public:
    /**
     * @brief 构造函数
     */
    ProgrammerMode();
    
    /**
     * @brief 析构函数
     */
    ~ProgrammerMode() override = default;
    
    // 实现基类接口
    bool initialize() override;
    bool handleKeyInput(uint8_t keyPosition, bool isLongPress = false,
                       bool isSecondFunction = false) override;
    void updateDisplay() override;
    String getHelpText() const override;

private:
    enum NumberBase { BINARY = 2, OCTAL = 8, DECIMAL = 10, HEXADECIMAL = 16 };
    
    NumberBase _currentBase;            ///< 当前进制
    int64_t _currentValue;              ///< 当前值（整数）
    
    /**
     * @brief 切换进制
     * @param base 目标进制
     */
    void switchBase(NumberBase base);
    
    /**
     * @brief 转换数字到指定进制
     * @param value 数值
     * @param base 进制
     * @return 转换后的字符串
     */
    String convertToBase(int64_t value, NumberBase base) const;
    
    /**
     * @brief 执行位运算
     * @param operation 运算类型
     */
    void performBitwiseOperation(const String& operation);
    
    /**
     * @brief 创建程序员模式按键映射
     * @return 按键映射向量
     */
    static std::vector<ModeKeyMapping> createProgrammerKeyMappings();
    
    /**
     * @brief 创建程序员模式配置
     * @return 模式配置
     */
    static ModeConfig createProgrammerConfig();
};

/**
 * @brief 模式管理器
 */
class ModeManager {
public:
    /**
     * @brief 构造函数
     */
    ModeManager();
    
    /**
     * @brief 析构函数
     */
    ~ModeManager() = default;
    
    /**
     * @brief 初始化模式管理器
     * @return true 成功，false 失败
     */
    bool begin();
    
    /**
     * @brief 注册模式
     * @param mode 模式对象
     * @return 模式ID
     */
    uint8_t registerMode(std::shared_ptr<CalculatorMode> mode);
    
    /**
     * @brief 激活模式
     * @param modeId 模式ID
     * @param display 显示管理器
     * @param engine 计算引擎
     * @return true 成功，false 失败
     */
    bool activateMode(uint8_t modeId,
                     std::shared_ptr<CalculatorDisplay> display,
                     std::shared_ptr<CalculationEngine> engine);
    
    /**
     * @brief 获取当前活动模式
     * @return 当前模式指针，未激活返回nullptr
     */
    std::shared_ptr<CalculatorMode> getCurrentMode() const;
    
    /**
     * @brief 获取模式列表
     * @return 模式名称列表
     */
    std::vector<String> getModeList() const;
    
    /**
     * @brief 获取模式数量
     * @return 注册的模式数量
     */
    size_t getModeCount() const { return _modes.size(); }

private:
    std::vector<std::shared_ptr<CalculatorMode>> _modes;  ///< 注册的模式列表
    uint8_t _currentModeId;                              ///< 当前活动模式ID
    uint8_t _nextModeId;                                 ///< 下一个分配的模式ID
};

#endif // CALCULATOR_MODES_H