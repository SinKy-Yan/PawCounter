#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include "databus/Arduino_ESP32SPIDMA.h"
#include "canvas/Arduino_Canvas.h"

// 项目头文件
#include "config.h"
#include "Logger.h"
#include "BackLightControl.h"
#include "KeypadControl.h"
#include "CalculatorCore.h"
#include "calc_display.h"
#include "CalcDisplayAdapter.h"
#include "CalculationEngine.h"


// 全局对象
Arduino_DataBus *bus = nullptr;
Arduino_GFX *gfx = nullptr;
Arduino_Canvas *canvas = nullptr;
KeypadControl keypad;

// LED数组定义（在config.h中声明为extern）
CRGB leds[NUM_LEDS];

// 计算器系统
std::shared_ptr<CalculationEngine> engine;
std::unique_ptr<CalcDisplay> display;
std::shared_ptr<CalcDisplayAdapter> displayAdapter;
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
#ifdef DEBUG_MODE
    Serial.println("调试模式: 已启用");
#endif
    Serial.println();
    
    // 1. 初始化日志系统
    Serial.println("1. 初始化日志系统...");
    Logger& logger = Logger::getInstance();
    LoggerConfig logConfig = Logger::getDefaultConfig();
    logConfig.level = LOG_LEVEL_INFO;
    logger.begin(logConfig);
    LOG_I(TAG_MAIN, "✅ 日志系统初始化完成");
    
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
    BacklightControl::getInstance().setBacklight(100, 2000);  // 100%最大亮度，2秒渐变
    LOG_I(TAG_MAIN, "背光控制初始化完成");
    
    // 5. 初始化简单LED系统已在步骤3中完成
    // 基本LED初始化已在initLEDs()中完成
    
    // 5. 初始化键盘控制
    Serial.println("5. 初始化键盘系统...");
    keypad.begin();
    keypad.setKeyEventCallback(onKeyEvent);
    LOG_I(TAG_MAIN, "键盘系统初始化完成");
    
    // 6. 创建计算引擎
    Serial.println("6. 初始化计算引擎...");
    engine = std::make_shared<CalculationEngine>();
    if (!engine->begin()) {
        LOG_E(TAG_MAIN, "计算引擎初始化失败");
        Serial.println("❌ 计算引擎初始化失败");
        return;
    }
    LOG_I(TAG_MAIN, "计算引擎初始化完成");
    
    // 7. 创建显示管理器
    Serial.println("7. 初始化显示管理器...");
    Serial.println("  - 使用简化CalcDisplay界面");
    // 使用Canvas优化显示性能，如果Canvas不可用则回退到直接使用gfx
    Arduino_GFX* displayTarget = canvas ? canvas : gfx;
    display = std::unique_ptr<CalcDisplay>(new CalcDisplay(displayTarget, DISPLAY_WIDTH, DISPLAY_HEIGHT));
    displayAdapter = std::make_shared<CalcDisplayAdapter>(display.get());
    LOG_I(TAG_MAIN, "显示管理器初始化完成");
    
    // 8. 创建计算器核心
    Serial.println("8. 初始化计算器核心...");
    calculator = std::make_shared<CalculatorCore>();
    calculator->setDisplay(displayAdapter);
    calculator->setCalculationEngine(engine);
    
    // 设置适配器的计算器核心引用（用于获取历史记录）
    displayAdapter->setCalculatorCore(calculator.get());
    
    if (!calculator->begin()) {
        LOG_E(TAG_MAIN, "计算器核心初始化失败");
        Serial.println("❌ 计算器核心初始化失败");
        return;
    }
    LOG_I(TAG_MAIN, "计算器核心初始化完成");
    
    // 9. 初始化计算器界面（直接进入计算器）
    if (calculator && displayAdapter) {
        // 清除所有内容，设置初始状态
        calculator->clearAll();
        
        // 立即刷新显示，显示计算器界面（显示"0"）
        displayAdapter->updateDisplay(
            calculator->getCurrentDisplay(),
            "",  // 表达式为空
            calculator->getState()
        );
        
        LOG_I(TAG_MAIN, "计算器界面已就绪");
    }
    
    // 10. 启动效果（简化）
    // 简单的启动LED效果
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Blue;
    }
    FastLED.show();
    delay(500);
    FastLED.clear();
    FastLED.show();
    
    
    Serial.println("=== 计算器系统启动完成 ===");
    Serial.println("系统就绪，发送 'help' 查看命令");
    
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
    
    // 更新动画系统
    if (display) {
        display->tick();
    }
    
    // 小延迟避免过度占用CPU
    delay(1);
}

void initDisplay() {
    // 背光控制 - 确保在屏幕初始化后再开启
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW); // 先关闭背光
    
    Serial.println("  - 初始化显示总线...");
    // 升级到DMA SPI，80 MHz
    bus = new Arduino_ESP32SPIDMA(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI,
                                  /*miso*/ -1, /*host*/ SPI3_HOST, false);
    
    Serial.println("  - 初始化显示驱动...");
    gfx = new Arduino_NV3041A(bus,
                             LCD_RST,
                             2,             // rotation: 0~3
                             true,          // IPS 屏
                             DISPLAY_WIDTH, // 480
                             DISPLAY_HEIGHT,// 130
                             0,             // 水平偏移（col_offset）
                             0,
                             0,
                             140);          // 垂直偏移（row_offset）- 向上扩展5像素
    
    Serial.println("  - 启动显示硬件...");
    if (!gfx->begin(80 * 1000 * 1000UL)) {  // 80 MHz
        Serial.println("❌ 显示硬件启动失败！");
        LOG_E(TAG_MAIN, "显示硬件启动失败");
        return;
    }
    
    // 创建全屏Canvas缓冲区
    Serial.println("  - 创建Canvas缓冲区...");
    // 去除输出偏移，Canvas直接输出到(0,0)
    canvas = new Arduino_Canvas(DISPLAY_WIDTH, DISPLAY_HEIGHT, gfx);
    if (!canvas->begin(GFX_SKIP_OUTPUT_BEGIN)) {
        Serial.println("❌ Canvas初始化失败！回退到软件SPI");
        delete canvas;
        canvas = nullptr;
        delete bus;
        // 回退到软件SPI
        bus = new Arduino_SWSPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);
        delete gfx;
        gfx = new Arduino_NV3041A(bus, LCD_RST, 2, true, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, 0, 140);
        if (!gfx->begin()) {
            Serial.println("❌ 回退显示硬件启动也失败！");
            return;
        }
    } else {
        Serial.printf("✅ DMA Canvas创建成功: %dx%d, 内存占用: %d KB\n", 
                     DISPLAY_WIDTH, DISPLAY_HEIGHT, 
                     (DISPLAY_WIDTH * DISPLAY_HEIGHT * 2) / 1024);
    }
    
    // 延迟确保初始化完成
    delay(100);
    
    // 背光保持关闭，等待后续软件控制
    Serial.println("  - 背光硬件准备完成，等待软件控制");
    
    // 清屏
    if (canvas) {
        canvas->fillScreen(0x0000);
        canvas->flush();
    } else {
        gfx->fillScreen(0x0000);
    }
    
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
            
            // 移除强制刷新，避免在动画执行期间提前绘制目标文本导致闪烁/重影
            // 如果后续需要手动刷新，可在确认无活动动画时调用 displayAdapter->updateDisplay()
        }
        
        // 简单的按键反馈（LED亮起）
        if (key >= 1 && key <= NUM_LEDS) {
            leds[key-1] = CRGB::White;
            FastLED.show();
            delay(50);
            leds[key-1] = CRGB::Black;
            FastLED.show();
        }
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
            Serial.println();
            Serial.println("== 硬件控制 ==");
            Serial.println("backlight [0-100] - 设置背光亮度");
            Serial.println("test           - 执行系统测试");
            Serial.println("testframe      - Arduino GFX红色线框测试");
            Serial.println();
            Serial.println("== 计算器操作 ==");
            Serial.println("clear/c        - 清除显示");
            Serial.println("mode/m         - 切换计算模式");
            Serial.println();
            Serial.println("== 调试功能 ==");
            Serial.println("layout         - 显示按键布局");
            Serial.println("keymap         - 按键映射测试");
            Serial.println("log [level]    - 设置日志级别 (e/w/i/d/v)");
            Serial.println();
            Serial.println("== 配置状态 ==");
#ifdef DEBUG_MODE
            Serial.println("调试模式: 已启用");
#else
            Serial.println("调试模式: 已禁用");
#endif
            Serial.println("UI界面: CalcDisplay简化UI");
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
            // 简单的LED测试
            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = CRGB::Green;
            }
            FastLED.show();
            delay(1000);
            FastLED.clear();
            FastLED.show();
        }
        else if (command == "testframe") {
            Serial.println("绘制Arduino GFX红色线框测试...");
            if (gfx) {
                // 清屏为黑色
                gfx->fillScreen(0x0000);
                
                // 绘制外边框线条 - 红色 (RGB565: 0xF800)
                gfx->drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0xF800);
                
                // 绘制内部线框，间隔10像素
                gfx->drawRect(10, 10, DISPLAY_WIDTH-20, DISPLAY_HEIGHT-20, 0xF800);
                
                // 绘制中心十字线
                gfx->drawLine(DISPLAY_WIDTH/2, 0, DISPLAY_WIDTH/2, DISPLAY_HEIGHT, 0xF800);
                gfx->drawLine(0, DISPLAY_HEIGHT/2, DISPLAY_WIDTH, DISPLAY_HEIGHT/2, 0xF800);
                
                // 在四个角落绘制小方块
                gfx->fillRect(0, 0, 20, 20, 0xF800);                    // 左上角
                gfx->fillRect(DISPLAY_WIDTH-20, 0, 20, 20, 0xF800);     // 右上角
                gfx->fillRect(0, DISPLAY_HEIGHT-20, 20, 20, 0xF800);    // 左下角
                gfx->fillRect(DISPLAY_WIDTH-20, DISPLAY_HEIGHT-20, 20, 20, 0xF800); // 右下角
                
                // 在底部绘制测试线条
                for (int y = DISPLAY_HEIGHT-10; y < DISPLAY_HEIGHT; y++) {
                    gfx->drawLine(0, y, DISPLAY_WIDTH, y, 0xFFFF);  // 白色线条
                }
                
                // 显示坐标信息
                gfx->setTextColor(0x07E0);  // 绿色文字
                gfx->setTextSize(2);
                gfx->setCursor(30, 30);
                gfx->printf("Frame Test %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
                
                gfx->setCursor(30, 60);
                gfx->print("Bottom lines:");
                gfx->setCursor(30, 80);
                gfx->printf("Y=%d to Y=%d", DISPLAY_HEIGHT-10, DISPLAY_HEIGHT-1);
                
                Serial.println("✅ Arduino GFX红色线框已绘制");
                Serial.printf("外边框: (0,0) 到 (%d,%d)\n", DISPLAY_WIDTH-1, DISPLAY_HEIGHT-1);
                Serial.printf("底部测试线: Y=%d 到 Y=%d\n", DISPLAY_HEIGHT-10, DISPLAY_HEIGHT-1);
                Serial.println("检查显示屏是否有底部花屏现象");
            } else {
                Serial.println("❌ GFX对象为空");
            }
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
    
    // 更新计算器核心
    if (calculator) {
        calculator->update();
    }
}