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
// #include "CalcDisplayAdapter.h" - 已移除适配器层
#include "CalculationEngine.h"
#include "KeyboardConfig.h"   // 新增：用于打印键盘配置


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
// CalcDisplayAdapter已被移除，直接使用CalcDisplay
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
    // CalcDisplayAdapter已被移除，直接使用CalcDisplay
    LOG_I(TAG_MAIN, "显示管理器初始化完成");
    
    // 8. 创建计算器核心
    Serial.println("8. 初始化计算器核心...");
    calculator = std::make_shared<CalculatorCore>();
    calculator->setDisplay(display.get());
    calculator->setCalculationEngine(engine);
    
    // CalcDisplayAdapter已被移除，直接使用CalcDisplay
    
    if (!calculator->begin()) {
        LOG_E(TAG_MAIN, "计算器核心初始化失败");
        Serial.println("❌ 计算器核心初始化失败");
        return;
    }
    LOG_I(TAG_MAIN, "计算器核心初始化完成");
    
    // 9. 初始化计算器界面（直接进入计算器）
    if (calculator && display) {
        // 清除所有内容，设置初始状态
        calculator->clearAll();
        
        // 立即刷新显示，显示计算器界面（显示"0"）
        display->updateExprDirect("");
        display->updateResultDirect(calculator->getCurrentDisplay());
        display->refresh();
        
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
    const char* eventStr;
    switch (type) {
        case KEY_EVENT_PRESS:       eventStr = "按下"; break;
        case KEY_EVENT_RELEASE:     eventStr = "释放"; break;
        case KEY_EVENT_LONGPRESS:   eventStr = "长按"; break;
        case KEY_EVENT_REPEAT:      eventStr = "重复"; break;
        case KEY_EVENT_COMBO:       eventStr = "组合"; break;
        default:                    eventStr = "未知"; break;
    }
    
    // 完整的日志格式
    LOG_D(TAG_MAIN, "按键事件: Key=%d, Event=%s", key, eventStr);

    // 简化串口输出
    Serial.printf("按键事件: Key=%d, Event=%s\n", key, eventStr);
    
    // 仅在按下/长按时处理输入，忽略释放等其他事件
    if (calculator) {
        if (type == KEY_EVENT_PRESS) {
            calculator->handleKeyInput(key, /*isLongPress*/ false);
        } else if (type == KEY_EVENT_LONGPRESS) {
            calculator->handleKeyInput(key, /*isLongPress*/ true);
        }
    }
}

void handleSerialCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd.equalsIgnoreCase("help")) {
            Serial.println("可用命令:");
            Serial.println("  help          - 显示此帮助信息");
            Serial.println("  status        - 显示系统状态");
            Serial.println("  led <idx> <r> <g> <b> - 设置单个LED颜色");
            Serial.println("  led all <r> <g> <b> - 设置所有LED颜色");
            Serial.println("  led off       - 关闭所有LED");
            Serial.println("  brightness <0-255> - 设置LED亮度");
            Serial.println("  reboot        - 重启设备");
            Serial.println("  log_level <lvl> - 设置日志级别 (0:无, 1:错误, 2:警告, 3:信息, 4:调试, 5:详细)");
            Serial.println("  mem           - 显示内存使用情况");
            Serial.println("  tasks         - 显示正在运行的任务");
            Serial.println("  layout        - 显示键盘布局");
            Serial.println("  config        - 显示当前加载的配置");
        } else if (cmd.equalsIgnoreCase("status")) {
            Serial.println("系统状态:");
            Serial.printf(" - 可用堆内存: %d 字节\n", ESP.getFreeHeap());
            Serial.printf(" - CPU 频率: %d MHz\n", getCpuFrequencyMhz());
            Serial.printf(" - 运行时间: %lu 毫秒\n", millis());
            Serial.printf(" - 背光亮度: %d%%\n", BacklightControl::getInstance().getCurrentBrightness() * 100 / 255);
            if(calculator) {
                Serial.printf(" - 计算器显示: %s\n", calculator->getCurrentDisplay().c_str());
            }
        } else if (cmd.startsWith("led")) {
            int parts[5];
            if (cmd.indexOf("all") > 0) {
                 if (sscanf(cmd.c_str(), "led all %d %d %d", &parts[2], &parts[3], &parts[4]) == 3) {
                     for (int i = 0; i < NUM_LEDS; i++) {
                         leds[i].setRGB(parts[2], parts[3], parts[4]);
                     }
                     FastLED.show();
                     Serial.println("所有LED已更新");
                 } else {
                     Serial.println("无效的 'led all' 命令格式. 使用: led all <r> <g> <b>");
                 }
            } else if (cmd.indexOf("off") > 0) {
                FastLED.clear();
                FastLED.show();
                Serial.println("所有LED已关闭");
            } else {
                if (sscanf(cmd.c_str(), "led %d %d %d %d", &parts[1], &parts[2], &parts[3], &parts[4]) == 4) {
                    if (parts[1] >= 0 && parts[1] < NUM_LEDS) {
                        leds[parts[1]].setRGB(parts[2], parts[3], parts[4]);
                        FastLED.show();
                        Serial.printf("LED %d 已更新\n", parts[1]);
                    } else {
                        Serial.println("无效的LED索引");
                    }
                } else {
                    Serial.println("无效的 'led' 命令格式. 使用: led <index> <r> <g> <b>");
                }
            }
        } else if (cmd.startsWith("brightness")) {
            int brightness;
            if (sscanf(cmd.c_str(), "brightness %d", &brightness) == 1) {
                if (brightness >= 0 && brightness <= 255) {
                    FastLED.setBrightness(brightness);
                    FastLED.show();
                    Serial.printf("LED亮度已设置为 %d\n", brightness);
                } else {
                    Serial.println("亮度值必须在 0-255 之间");
                }
            } else {
                Serial.println("无效的 'brightness' 命令格式. 使用: brightness <0-255>");
            }
        } else if (cmd.equalsIgnoreCase("reboot")) {
            Serial.println("正在重启...");
            ESP.restart();
        } else if (cmd.startsWith("log_level")) {
            int level;
            if (sscanf(cmd.c_str(), "log_level %d", &level) == 1) {
                if (level >= LOG_LEVEL_NONE && level <= LOG_LEVEL_VERBOSE) {
                    Logger::getInstance().setLevel((log_level_t)level);
                    Serial.printf("日志级别已设置为 %d\n", level);
                } else {
                    Serial.println("无效的日志级别");
                }
            } else {
                 Serial.println("无效的 'log_level' 命令格式. 使用: log_level <0-5>");
            }
        } else if (cmd.equalsIgnoreCase("mem")) {
            Serial.println("内存使用情况:");
            Serial.printf(" - 总堆大小: %d\n", ESP.getHeapSize());
            Serial.printf(" - 可用堆大小: %d\n", ESP.getFreeHeap());
            Serial.printf(" - 最小剩余堆: %d\n", ESP.getMinFreeHeap());
            Serial.printf(" - 最大分配块: %d\n", ESP.getMaxAllocHeap());
        } else if (cmd.equalsIgnoreCase("tasks")) {
            Serial.println("任务列表功能暂时不可用（vTaskList未启用）");
        } else if (cmd.equalsIgnoreCase("layout")) {
            keyboardConfig.printConfig();
        } else if (cmd.equalsIgnoreCase("config")) {
            keyboardConfig.printConfig();
        }
        else {
            Serial.printf("未知命令: '%s'\n", cmd.c_str());
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