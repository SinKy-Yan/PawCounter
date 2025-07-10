#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino_GFX_Library.h>

// 项目头文件
#include "config.h"
#include "Logger.h"
#include "TaskManager.h"
#include "SystemController.h"
#include "CalculatorApplication.h"
#include "LVGLDisplay.h"
#include "BackLightControl.h"
#include <FastLED.h>

// 全局对象
LVGLDisplay* lvgl_display = nullptr;
TaskManager taskManager;

// LED数组定义（在config.h中声明为extern）
CRGB leds[NUM_LEDS];

// 函数声明
void initLVGLDisplay();
void initLEDs();
void handleSystemInit();

void setup() {
    Serial.begin(115200);
    delay(1000); // 等待串口稳定
    
    Serial.println("=== ESP32-S3 PawCounter FreeRTOS版本启动 ===");
    Serial.println("架构: FreeRTOS多任务系统");
    Serial.println("版本: 重构版 v2.0");
    Serial.println();
    
    // 执行系统初始化
    handleSystemInit();
    
    // 初始化任务管理器硬件
    Serial.println("初始化任务管理器...");
    if (!taskManager.initializeHardware()) {
        Serial.println("❌ 任务管理器初始化失败");
        return;
    }
    Serial.println("✅ 任务管理器初始化完成");
    
    // 创建FreeRTOS任务
    Serial.println("创建FreeRTOS任务...");
    taskManager.createTasks();
    Serial.println("✅ 所有任务创建完成");
    
    // 显示系统信息
    Serial.println("\n=== 系统信息 ===");
    Serial.printf("CPU频率: %u MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("堆内存: %u 字节\n", ESP.getFreeHeap());
    Serial.printf("PSRAM: %u 字节\n", ESP.getFreePsram());
    Serial.println("================");
    
    Serial.println("\n=== FreeRTOS任务系统启动 ===");
    Serial.println("按键任务: 优先级3, 10ms周期, Core1");
    Serial.println("显示任务: 优先级2, 20ms周期, Core1");
    Serial.println("系统任务: 优先级1, 1000ms周期, Core0");
    Serial.println("==============================");
    
    Serial.println("\n系统就绪，发送串口命令进行控制");
    Serial.println("输入 'help' 查看可用命令");
    
    // 启动FreeRTOS任务系统
    // 在Arduino ESP32框架中，调度器已经运行，只需要确认任务创建成功
    taskManager.startScheduler();
}

void loop() {
    // 在Arduino ESP32框架中，loop()仍然会被调用
    // 但主要逻辑已经在FreeRTOS任务中运行
    
    // 保持loop()为空，让FreeRTOS任务处理所有逻辑
    // 这样可以确保任务优先级正确工作
    vTaskDelay(pdMS_TO_TICKS(1000));  // 让其他任务运行
}

void handleSystemInit() {
    // 1. 初始化硬件LED系统
    Serial.println("1. 初始化LED系统...");
    initLEDs();
    
    // 2. 初始化LVGL显示系统
    Serial.println("2. 初始化LVGL显示系统...");
    initLVGLDisplay();
    
    // 3. 初始化背光控制
    Serial.println("3. 初始化背光控制...");
    BacklightControl::getInstance().begin();
    delay(100);  // 确保背光控制器初始化完成
    BacklightControl::getInstance().setBacklight(100, 1000);  // 启动时100%亮度
    Serial.println("  - 背光已设置为100%");
    
    Serial.println("✅ 硬件初始化完成");
}

void initLVGLDisplay() {
    // 背光控制 - 初始化但不立即开启
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW); // 先关闭背光，等待BacklightControl接管
    
    Serial.println("  - 初始化SPI总线和显示驱动...");
    
    // 初始化SPI总线
    Arduino_DataBus *bus = new Arduino_ESP32SPIDMA(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI,
                                      /*miso*/ -1, /*host*/ SPI3_HOST, false);
    
    // 初始化显示驱动
    Arduino_GFX *gfx = new Arduino_NV3041A(bus,
                                   LCD_RST,
                                   2,             // rotation: 0~3
                                   true,          // IPS 屏
                                   DISPLAY_WIDTH, // 480
                                   DISPLAY_HEIGHT,// 135
                                   0,             // 水平偏移
                                   0,
                                   0,
                                   140);          // 垂直偏移
    
    // 启动显示硬件
    if (!gfx->begin(80 * 1000 * 1000UL)) {  // 80 MHz
        Serial.println("❌ 显示硬件启动失败！");
        return;
    }
    
    // 清屏
    gfx->fillScreen(0x0000);
    
    // 初始化LVGL显示适配器
    lvgl_display = new LVGLDisplay(gfx, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!lvgl_display->begin()) {
        Serial.println("❌ LVGL显示系统初始化失败！");
        return;
    }
    
    // 创建测试显示内容
    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "PawCounter\nFreeRTOS v2.0\nSystem Ready");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);
    
    // 强制刷新显示
    lv_refr_now(lv_disp_get_default());
    
    Serial.println("✅ LVGL显示系统初始化完成");
}

void initLEDs() {
    Serial.println("  - 配置FastLED...");
    FastLED.addLeds<WS2812, RGB_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    
    Serial.println("  - 清除所有LED...");
    FastLED.clear();
    FastLED.show();
    
    // 启动LED测试序列 - 简化版本
    Serial.println("  - 执行LED启动序列...");
    
    // 快速紫色闪烁
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Purple;
    }
    FastLED.show();
    delay(200);
    
    FastLED.clear();
    FastLED.show();
    delay(100);
    
    // 再次短暂闪烁表示准备就绪
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Green;
    }
    FastLED.show();
    delay(150);
    
    FastLED.clear();
    FastLED.show();
    
    Serial.println("✅ LED系统初始化完成");
}