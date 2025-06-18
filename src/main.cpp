#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>

// 项目头文件
#include "config.h"
#include "Logger.h"
#include "BackLightControl.h"
#include "KeypadControl.h"
#include "BatteryManager.h"
#include "FeedbackManager.h"
#include "CalculatorCore.h"
#include "CalculatorDisplay.h"
#include "CalculationEngine.h"

// 全局对象
Arduino_DataBus *bus = nullptr;
Arduino_GFX *gfx = nullptr;
KeypadControl keypad;
BatteryManager batteryManager;

// LED数组定义（在config.h中声明为extern）
CRGB leds[NUM_LEDS];

// 计算器系统
std::shared_ptr<CalculationEngine> engine;
std::shared_ptr<CalculatorDisplay> display;
std::shared_ptr<CalculatorCore> calculator;

// 函数声明
void initDisplay();
void initLEDs();
void onKeyEvent(KeyEventType type, uint8_t key, uint8_t* combo, uint8_t count);
void handleSerialCommands();
void updateSystems();

void setup() {
    Serial.begin(115200);
    delay(1000); // 等待串口稳定
    
    Serial.println("=== ESP32-S3 计算器系统启动 ===");
    Serial.println("配置模式:");
#ifdef DEBUG_MODE
    Serial.println("  - 调试模式: 已启用");
#else
    Serial.println("  - 调试模式: 已禁用");
#endif
#ifdef ENABLE_BATTERY_MANAGER
    Serial.println("  - 电池管理: 已启用");
#else
    Serial.println("  - 电池管理: 已禁用");
#endif
    Serial.println();
    
    // 1. 初始化日志系统
    Serial.println("1. 初始化日志系统...");
    Logger& logger = Logger::getInstance();
    LoggerConfig logConfig = Logger::getDefaultConfig();
    logConfig.level = LOG_LEVEL_INFO;
    logger.begin(logConfig);
    LOG_I(TAG_MAIN, "日志系统初始化完成");
    
    // 2. 初始化硬件显示系统
    Serial.println("2. 初始化显示系统...");
    initDisplay();
    LOG_I(TAG_MAIN, "显示系统初始化完成");
    
    // 3. 初始化LED系统
    Serial.println("3. 初始化LED系统...");
    initLEDs();
    LOG_I(TAG_MAIN, "LED系统初始化完成");
    
    // 4. 初始化背光控制
    Serial.println("4. 初始化背光控制...");
    BacklightControl::getInstance().begin();
    BacklightControl::getInstance().setBacklight(100, 2000); // 100%亮度，2秒渐变
    LOG_I(TAG_MAIN, "背光控制初始化完成");
    
    // 5. 初始化反馈系统（LED效果 + 蜂鸣器）
    Serial.println("5. 初始化反馈系统...");
    if (!FEEDBACK_MGR.begin()) {
        LOG_E(TAG_MAIN, "反馈系统初始化失败");
        Serial.println("❌ 反馈系统初始化失败");
    } else {
        LOG_I(TAG_MAIN, "反馈系统初始化完成");
        Serial.println("✅ 反馈系统初始化完成");
    }
    
    // 6. 初始化键盘控制
    Serial.println("6. 初始化键盘系统...");
    keypad.begin();
    keypad.setKeyEventCallback(onKeyEvent);
    LOG_I(TAG_MAIN, "键盘系统初始化完成");
    
    // 7. 初始化电池管理系统（可选）
    Serial.println("7. 初始化电池管理...");
#ifdef ENABLE_BATTERY_MANAGER
    if (!batteryManager.begin()) {
        LOG_W(TAG_MAIN, "电池管理系统初始化失败 - 可能未连接电池硬件");
        Serial.println("⚠️ 电池管理系统初始化失败 - 调试模式下可忽略");
    } else {
        LOG_I(TAG_MAIN, "电池管理系统初始化完成");
        Serial.println("✅ 电池管理系统初始化完成");
    }
#else
    LOG_I(TAG_MAIN, "电池管理系统已禁用 - 调试模式");
    Serial.println("🔧 电池管理系统已禁用 - 调试模式");
#endif
    
    // 8. 创建计算引擎
    Serial.println("8. 初始化计算引擎...");
    engine = std::make_shared<CalculationEngine>();
    if (!engine->begin()) {
        LOG_E(TAG_MAIN, "计算引擎初始化失败");
        Serial.println("❌ 计算引擎初始化失败");
        return;
    }
    LOG_I(TAG_MAIN, "计算引擎初始化完成");
    
    // 9. 创建显示管理器
    Serial.println("9. 初始化显示管理器...");
    auto lcdDisplay = std::make_shared<LCDDisplay>(gfx);
    auto serialDisplay = std::make_shared<SerialDisplay>();
    display = std::make_shared<DualDisplay>(lcdDisplay, serialDisplay);
    if (!display->begin()) {
        LOG_E(TAG_MAIN, "显示管理器初始化失败");
        Serial.println("❌ 显示管理器初始化失败");
        return;
    }
    LOG_I(TAG_MAIN, "显示管理器初始化完成");
    
    // 10. 创建计算器核心
    Serial.println("10. 初始化计算器核心...");
    calculator = std::make_shared<CalculatorCore>();
    calculator->setDisplay(display);
    calculator->setCalculationEngine(engine);
    if (!calculator->begin()) {
        LOG_E(TAG_MAIN, "计算器核心初始化失败");
        Serial.println("❌ 计算器核心初始化失败");
        return;
    }
    LOG_I(TAG_MAIN, "计算器核心初始化完成");
    
    // 11. 显示启动信息（应用180度旋转）
    if (gfx) {
        gfx->fillScreen(0x0000); // 黑色背景
        gfx->setTextColor(0x07E0); // 绿色文字
        gfx->setTextSize(3);       // 标题字体
        
        // 计算180度翻转后的坐标
        // 原始坐标(10, 30) -> 翻转后坐标
        uint16_t titleX = DISPLAY_WIDTH - 10 - 200; // 减去标题宽度估计
        uint16_t titleY = DISPLAY_HEIGHT - 30 - 24;  // 减去字体高度
        gfx->setCursor(titleX, titleY);
        gfx->print("Calculator Ready");
        
        gfx->setTextColor(0xFFFF); // 白色文字
        gfx->setTextSize(2);       // 版本信息字体
        
        // 版本信息翻转坐标
        uint16_t versionX = DISPLAY_WIDTH - 10 - 250;
        uint16_t versionY = DISPLAY_HEIGHT - 70 - 16;
        gfx->setCursor(versionX, versionY);
        gfx->print("ESP32-S3 Calculator v2.0");
        
        // 提示信息翻转坐标
        uint16_t promptX = DISPLAY_WIDTH - 10 - 200;
        uint16_t promptY = DISPLAY_HEIGHT - 100 - 16;
        gfx->setCursor(promptX, promptY);
        gfx->print("Press any key to start");
    }
    
    // 12. 启动效果
    FEEDBACK_MGR.triggerSystemFeedback(SCENE_SYSTEM_STARTUP);
    
    Serial.println("=== 计算器系统启动完成 ===");
    Serial.println("系统就绪，可以开始使用");
    Serial.println("发送 'help' 查看可用命令");
    
    LOG_I(TAG_MAIN, "系统启动完成，所有组件已就绪");
}

void loop() {
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    
    // 每10ms更新一次系统状态
    if (currentTime - lastUpdate >= 10) {
        lastUpdate = currentTime;
        updateSystems();
    }
    
    // 处理串口命令
    handleSerialCommands();
    
    // 小延迟避免过度占用CPU
    delay(1);
}

void initDisplay() {
    // 背光控制 - 确保在屏幕初始化后再开启
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW); // 先关闭背光
    
    Serial.println("  - 初始化显示总线...");
    bus = new Arduino_HWSPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);
    
    Serial.println("  - 初始化显示驱动...");
    gfx = new Arduino_NV3041A(bus, LCD_RST, 0, true, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, 0, 0);
    
    Serial.println("  - 启动显示硬件...");
    if (!gfx->begin()) {
        Serial.println("❌ 显示硬件启动失败！");
        LOG_E(TAG_MAIN, "显示硬件启动失败");
        return;
    }
    
    // 延迟确保初始化完成
    delay(100);
    
    // 设置显示旋转（硬件限制：只有0和1有效，需要180度只能用软件实现）
    gfx->setRotation(0);  // 使用0度，然后在软件层面实现180度翻转
    Serial.println("  - 显示旋转设置为0度（将通过软件实现180度翻转）");
    
    // 背光保持关闭，等待后续软件控制
    Serial.println("  - 背光硬件准备完成，等待软件控制");
    
    // 清屏
    gfx->fillScreen(0x0000); // 黑色
    
    Serial.println("✅ 显示系统初始化完成");
}

void initLEDs() {
    Serial.println("  - 配置FastLED...");
    FastLED.addLeds<WS2812, RGB_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    
    Serial.println("  - 清除所有LED...");
    FastLED.clear();
    FastLED.show();
    
    // 启动LED测试序列
    Serial.println("  - 执行LED测试序列...");
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Blue;
        FastLED.show();
        delay(50);
        leds[i] = CRGB::Black;
    }
    FastLED.show();
    
    Serial.println("✅ LED系统初始化完成");
}

void onKeyEvent(KeyEventType type, uint8_t key, uint8_t* combo, uint8_t count) {
    // 记录按键事件
    const char* eventStr = "";
    switch (type) {
        case KEY_EVENT_PRESS: eventStr = "PRESSED"; break;
        case KEY_EVENT_RELEASE: eventStr = "RELEASED"; break;
        case KEY_EVENT_LONGPRESS: eventStr = "LONG_PRESSED"; break;
        case KEY_EVENT_REPEAT: eventStr = "REPEAT"; break;
        case KEY_EVENT_COMBO: eventStr = "COMBO"; break;
    }
    
    LOG_I(TAG_KEYPAD, "按键事件: Key=%d, Event=%s", key, eventStr);
    
    // 只处理按下事件，避免重复
    if (type == KEY_EVENT_PRESS) {
        if (calculator) {
            calculator->handleKeyInput(key);
        }
        
        // 播放按键反馈
        FEEDBACK_MGR.triggerKeyFeedback(key, true);
    }
}

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();
        
        if (command == "help" || command == "h") {
            Serial.println("\n=== 计算器系统命令 ===");
            Serial.println("== 系统状态 ==");
            Serial.println("help/h         - 显示命令帮助");
            Serial.println("status/s       - 显示系统状态");
#ifdef ENABLE_BATTERY_MANAGER
            Serial.println("battery/b      - 显示电池状态");
#endif
            Serial.println();
            Serial.println("== 硬件控制 ==");
            Serial.println("backlight [0-100] - 设置背光亮度");
            Serial.println("test           - 执行系统测试");
            Serial.println();
            Serial.println("== 计算器操作 ==");
            Serial.println("clear/c        - 清除显示");
            Serial.println("mode/m         - 切换计算模式");
            Serial.println();
            Serial.println("== 调试功能 ==");
            Serial.println("layout         - 显示按键布局");
            Serial.println("keymap         - 按键映射测试");
            Serial.println("rotate [0-1]   - 设置显示旋转角度(0=0度,1=90度)");
            Serial.println("log [level]    - 设置日志级别 (e/w/i/d/v)");
            Serial.println();
            Serial.println("== 配置状态 ==");
#ifdef DEBUG_MODE
            Serial.println("调试模式: 已启用");
#else
            Serial.println("调试模式: 已禁用");
#endif
#ifdef ENABLE_BATTERY_MANAGER
            Serial.println("电池管理: 已启用");
#else
            Serial.println("电池管理: 已禁用");
#endif
            Serial.println("=========================\n");
        }
        else if (command == "status" || command == "s") {
            Serial.println("\n=== 系统状态 ===");
            Serial.printf("运行时间: %lu ms\n", millis());
            Serial.printf("背光亮度: %d%%\n", BacklightControl::getInstance().getCurrentBrightness() * 100 / 255);
            if (calculator) {
                Serial.printf("计算器状态: 就绪\n");
                Serial.printf("当前显示: %s\n", calculator->getCurrentDisplay().c_str());
            }
#ifdef ENABLE_BATTERY_MANAGER
            batteryManager.update();
            Serial.printf("电池电压: %.2fV\n", batteryManager.getVoltage());
            Serial.printf("电池电量: %d%%\n", batteryManager.getPercentage());
#else
            Serial.printf("电池管理: 已禁用\n");
#endif
            Serial.println("==================\n");
        }
        else if (command.startsWith("backlight ")) {
            int brightness = command.substring(10).toInt();
            if (brightness >= 0 && brightness <= 100) {
                BacklightControl::getInstance().setBacklight(brightness * 255 / 100, 500);
                Serial.printf("背光亮度设置为: %d%%\n", brightness);
            } else {
                Serial.println("背光亮度范围: 0-100");
            }
        }
        else if (command == "test") {
            Serial.println("执行系统测试...");
            FEEDBACK_MGR.triggerSystemFeedback(SCENE_SUCCESS_NOTIFICATION);
        }
        else if (command == "clear" || command == "c") {
            if (calculator) {
                calculator->handleKeyInput(15); // Clear键
            }
        }
        else if (command.startsWith("log ")) {
            char level = command.charAt(4);
            switch (level) {
                case 'e': Logger::getInstance().setLevel(LOG_LEVEL_ERROR); break;
                case 'w': Logger::getInstance().setLevel(LOG_LEVEL_WARN); break;
                case 'i': Logger::getInstance().setLevel(LOG_LEVEL_INFO); break;
                case 'd': Logger::getInstance().setLevel(LOG_LEVEL_DEBUG); break;
                case 'v': Logger::getInstance().setLevel(LOG_LEVEL_VERBOSE); break;
                default:
                    Serial.println("日志级别: e(错误) w(警告) i(信息) d(调试) v(详细)");
                    return;
            }
            Serial.printf("日志级别设置为: %c\n", level);
        }
        else if (command == "mode" || command == "m") {
            if (calculator) {
                calculator->handleKeyInput(6); // BT键作为模式切换
            }
        }
        else if (command == "layout") {
            Serial.println("\n=== 按键布局 ===");
            Serial.println("┌───────┬───────┬───────┬───────┬───────┐");
            Serial.println("│Key 1  │Key 6  │Key 10 │Key 15 │Key 19 │");
            Serial.println("│ON/OFF │  BT   │ PCT   │  C    │ DEL   │");
            Serial.println("├───────┼───────┼───────┼───────┼───────┤");
            Serial.println("│Key 2  │Key 7  │Key 11 │Key 16 │Key 20 │");
            Serial.println("│  7    │  8    │  9    │ MUL   │ +/-   │");
            Serial.println("├───────┼───────┼───────┼───────┼───────┤");
            Serial.println("│Key 3  │Key 8  │Key 12 │Key 17 │Key 21 │");
            Serial.println("│  4    │  5    │  6    │ SUB   │ DIV   │");
            Serial.println("├───────┼───────┼───────┼───────┼───────┤");
            Serial.println("│Key 4  │Key 9  │Key 13 │Key 18 │Key 22 │");
            Serial.println("│  1    │  2    │  3    │ ADD   │ EQ    │");
            Serial.println("├───────┼───────┼───────┴───────┴───────┤");
            Serial.println("│Key 5  │Key 14 │                       │");
            Serial.println("│  0    │  .    │                       │");
            Serial.println("└───────┴───────┴───────────────────────┘");
            Serial.println("==================\n");
        }
        else if (command == "keymap") {
            if (calculator) {
                // 切换按键映射测试模式
                Serial.println("按键映射测试模式已切换");
            }
        }
        else if (command.startsWith("rotate ")) {
            int rotation = command.substring(7).toInt();
            if (rotation >= 0 && rotation <= 1) {
                if (gfx) {
                    gfx->setRotation(rotation);
                    gfx->fillScreen(0x0000);
                    gfx->setTextColor(0x07E0);
                    gfx->setTextSize(3);
                    // 应用180度翻转坐标显示旋转测试文本
                    uint16_t testX = DISPLAY_WIDTH - 10 - 120;
                    uint16_t testY = DISPLAY_HEIGHT - 50 - 24;
                    gfx->setCursor(testX, testY);
                    gfx->printf("Rotation: %d", rotation);
                    
                    uint16_t descX = DISPLAY_WIDTH - 10 - 100;
                    uint16_t descY = DISPLAY_HEIGHT - 80 - 24;
                    gfx->setCursor(descX, descY);
                    gfx->print(rotation == 0 ? "(0 degree)" : "(90 degree)");
                    Serial.printf("显示旋转设置为: %d (%s)\n", rotation, 
                                rotation == 0 ? "0度" : "90度");
                }
            } else {
                Serial.println("旋转角度范围: 0-1 (0=0度, 1=90度) - 此LCD只支持这两个角度");
            }
        }
#ifdef ENABLE_BATTERY_MANAGER
        else if (command == "battery" || command == "b") {
            Serial.println("\n=== 电池状态 ===");
            batteryManager.update();
            Serial.printf("电池电压: %.2fV\n", batteryManager.getVoltage());
            Serial.printf("电池电量: %d%%\n", batteryManager.getPercentage());
            
            // TP4056充电状态
            bool charging = !digitalRead(TP4056_CHRG_PIN);
            bool standby = !digitalRead(TP4056_STDBY_PIN);
            
            if (charging) {
                Serial.println("充电状态: 正在充电");
            } else if (standby) {
                Serial.println("充电状态: 充电完成");
            } else {
                Serial.println("充电状态: 未充电");
            }
            Serial.println("==================\n");
        }
#endif
        else {
            Serial.println("未知命令，发送 'help' 查看可用命令");
        }
    }
}

void updateSystems() {
    // 更新键盘扫描
    keypad.update();
    
    // 更新背光控制
    BacklightControl::getInstance().update();
    
    // 更新反馈系统
    FEEDBACK_MGR.update();
    
    // 定期更新电池状态（每5秒）
#ifdef ENABLE_BATTERY_MANAGER
    static unsigned long lastBatteryUpdate = 0;
    if (millis() - lastBatteryUpdate > 5000) {
        lastBatteryUpdate = millis();
        batteryManager.update();
        
        // 检查低电量警告
        if (batteryManager.getPercentage() < 10) {
            static unsigned long lastWarning = 0;
            if (millis() - lastWarning > 30000) { // 每30秒警告一次
                lastWarning = millis();
                FEEDBACK_MGR.triggerSystemFeedback(SCENE_BATTERY_LOW);
                LOG_W(TAG_MAIN, "低电量警告: %d%%", batteryManager.getPercentage());
            }
        }
    }
#endif
    
    // 更新计算器核心
    if (calculator) {
        calculator->update();
    }
}