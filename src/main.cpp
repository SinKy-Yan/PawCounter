/**
 * @file main_calculator.cpp
 * @brief 新版计算器主程序
 * @details 展示可扩展计算器架构的完整功能
 * 
 * 功能特性：
 * - 双重显示输出（LCD + 串口）
 * - 多种计算模式（基本、科学、财务）
 * - 完整的计算历史
 * - 财务模式的单位显示
 * - 可扩展的架构设计
 * 
 * @author Calculator Project
 * @date 2024-01-07
 */

#include <Arduino.h>
#include <memory>
#include "Logger.h"
#include "KeypadControl.h"
#include "Initialization.h"
#include "BackLightControl.h"

// 新计算器系统头文件
#include "CalculatorCore.h"
#include "CalculatorDisplay.h"
#include "CalculationEngine.h"
#include "CalculatorModes.h"

// 全局对象
std::shared_ptr<CalculatorCore> calculator;
std::shared_ptr<CalculatorDisplay> display;
std::shared_ptr<CalculationEngine> engine;
std::shared_ptr<FinancialMode> financialMode;
KeypadControl keypad;

// 当前模式指示
uint8_t currentModeId = 0;
const char* modeNames[] = {"基本", "科学", "财务"};

/**
 * @brief 按键事件回调函数
 */
void onKeyEvent(KeyEventType type, uint8_t key, uint8_t* combo, uint8_t count) {
    // 只处理按下事件
    if (type != KEY_EVENT_PRESS) {
        return;
    }
    
    LOG_I(TAG_MAIN, "Key pressed: %d", key);
    
    // 传递给计算器核心处理
    if (calculator) {
        calculator->handleKeyInput(key);
    }
}

/**
 * @brief 简化的基本模式实现
 */
class SimpleBasicMode : public CalculatorMode {
public:
    SimpleBasicMode() : CalculatorMode(createBasicConfig()) {}
    
    bool initialize() override {
        LOG_I(TAG_MAIN, "Basic mode initialized");
        return true;
    }
    
    bool handleKeyInput(uint8_t keyPosition, bool isLongPress = false,
                       bool isSecondFunction = false) override {
        // 基本模式的按键处理逻辑
        LOG_D(TAG_MAIN, "Basic mode handling key: %d", keyPosition);
        return true;
    }
    
    void updateDisplay() override {
        // 更新显示
    }
    
    String getHelpText() const override {
        return "基本计算模式 - 支持四则运算";
    }

private:
    static ModeConfig createBasicConfig() {
        ModeConfig config;
        config.name = "基本模式";
        config.description = "基础四则运算";
        config.type = ModeType::BASIC;
        config.defaultPrecision = PrecisionLevel::STANDARD;
        config.supportsMemory = true;
        config.supportsHistory = true;
        config.supportsSecondFunction = false;
        return config;
    }
};

/**
 * @brief 串口命令处理
 */
void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "help" || command == "h") {
            LOG_I(TAG_MAIN, "=== 计算器命令帮助 ===");
            LOG_I(TAG_MAIN, "help/h    - 显示帮助");
            LOG_I(TAG_MAIN, "mode/m    - 切换模式");
            LOG_I(TAG_MAIN, "test      - 测试计算");
            LOG_I(TAG_MAIN, "financial - 财务模式演示");
            LOG_I(TAG_MAIN, "clear/c   - 清除显示");
            LOG_I(TAG_MAIN, "status/s  - 显示状态");
            
        } else if (command == "mode" || command == "m") {
            currentModeId = (currentModeId + 1) % 3;
            calculator->switchMode(currentModeId);
            LOG_I(TAG_MAIN, "切换到: %s", modeNames[currentModeId]);
            
        } else if (command == "test") {
            LOG_I(TAG_MAIN, "=== 计算测试 ===");
            if (engine) {
                auto result = engine->calculate(123.45, 67.89, Operator::ADD);
                if (result.isValid) {
                    LOG_I(TAG_MAIN, "123.45 + 67.89 = %.2f", result.value);
                } else {
                    LOG_E(TAG_MAIN, "计算错误: %s", result.errorMessage.c_str());
                }
            }
            
        } else if (command == "financial") {
            LOG_I(TAG_MAIN, "=== 财务模式演示 ===");
            calculator->switchMode(2);  // 切换到财务模式
            
            // 演示不同金额的单位显示
            double amounts[] = {1234.56, 98765.43, 1234567.89, 99999999.99};
            const char* descriptions[] = {"小额", "中额", "大额", "巨额"};
            
            for (int i = 0; i < 4; i++) {
                LOG_I(TAG_MAIN, "%s示例: ¥%.2f", descriptions[i], amounts[i]);
                if (financialMode) {
                    financialMode->setAmount(amounts[i]);
                }
                delay(2000);  // 等待2秒显示
            }
            
        } else if (command == "clear" || command == "c") {
            calculator->clearAll();
            LOG_I(TAG_MAIN, "显示已清除");
            
        } else if (command == "status" || command == "s") {
            LOG_I(TAG_MAIN, "=== 计算器状态 ===");
            LOG_I(TAG_MAIN, "当前模式: %s", modeNames[currentModeId]);
            LOG_I(TAG_MAIN, "状态: %d", (int)calculator->getState());
            LOG_I(TAG_MAIN, "显示: %s", calculator->getCurrentDisplay().c_str());
            LOG_I(TAG_MAIN, "历史记录: %d 条", calculator->getHistory().size());
            
        } else if (command.length() > 0) {
            LOG_W(TAG_MAIN, "未知命令: %s (输入 help 查看帮助)", command.c_str());
        }
    }
}

/**
 * @brief 系统初始化
 */
void setup() {
    Serial.begin(115200);
    delay(1000);  // 等待串口稳定
    
    // 初始化日志系统
    Logger& logger = Logger::getInstance();
    LoggerConfig logConfig = Logger::getDefaultConfig();
    logConfig.level = LOG_LEVEL_INFO;  // 设置为WARN级别，大幅减少日志输出
    logger.begin(logConfig);
    
    LOG_I(TAG_MAIN, "=== 新版计算器系统启动 ===");
    LOG_I(TAG_MAIN, "版本: 2.0 - 可扩展架构");
    LOG_I(TAG_MAIN, "特性: 双显示 + 多模式 + 单位显示");
    
    // 初始化硬件
    LOG_I(TAG_MAIN, "初始化硬件系统...");
    initDisplay();
    
    // 初始化背光控制
    BacklightControl::getInstance().begin();
    BacklightControl::getInstance().setBacklight(50);
    
    // 初始化键盘
    keypad.begin();
    keypad.setKeyEventCallback(onKeyEvent);
    
    // 创建计算引擎
    LOG_I(TAG_MAIN, "创建计算引擎...");
    engine = std::make_shared<CalculationEngine>();
    engine->begin();
    
    // 创建显示管理器
    LOG_I(TAG_MAIN, "创建显示系统...");
    auto lcdDisplay = std::make_shared<LCDDisplay>(gfx);
    auto serialDisplay = std::make_shared<SerialDisplay>();
    display = std::make_shared<DualDisplay>(lcdDisplay, serialDisplay);
    
    // 初始化显示管理器（LCD已在initDisplay()中初始化了硬件）
    display->begin();
    
    // 创建计算器核心
    LOG_I(TAG_MAIN, "创建计算器核心...");
    calculator = std::make_shared<CalculatorCore>();
    calculator->setDisplay(display);
    calculator->setCalculationEngine(engine);
    calculator->begin();
    
    // 注册计算模式
    LOG_I(TAG_MAIN, "注册计算模式...");
    
    // 基本模式
    auto basicMode = std::make_shared<SimpleBasicMode>();
    calculator->addMode(basicMode);
    
    // 财务模式  
    financialMode = std::make_shared<FinancialMode>();
    calculator->addMode(financialMode);
    
    // 激活财务模式（默认模式）
    calculator->switchMode(1);
    
    // 显示欢迎信息
    display->showStatus("计算器已就绪 - 输入 help 查看命令");
    
    LOG_I(TAG_MAIN, "=== 系统初始化完成 ===");
    LOG_I(TAG_MAIN, "可用模式: 基本, 财务");
    LOG_I(TAG_MAIN, "串口命令: help, mode, test, financial, clear, status");
    LOG_I(TAG_MAIN, "按键输入已启用");
}

/**
 * @brief 主循环
 */
void loop() {
    // 更新硬件系统
    keypad.update();
    BacklightControl::getInstance().update();
    
    // 更新计算器系统
    if (calculator) {
        calculator->update();
    }
    
    // 处理串口命令
    handleSerialCommands();
    
    // 保持响应性
    delay(1);
}