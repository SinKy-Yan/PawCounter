#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include "LVGLDisplay.h"
#include "config.h"
#include "databus/Arduino_ESP32SPIDMA.h"

// 全局变量
Arduino_DataBus *lvgl_bus = nullptr;
Arduino_GFX *lvgl_gfx = nullptr;
LVGLDisplay *lvgl_display = nullptr;

// 初始化LVGL显示系统
bool initLVGLDisplay() {
    Serial.println("初始化LVGL显示系统...");
    
    // 初始化SPI总线
    lvgl_bus = new Arduino_ESP32SPIDMA(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI,
                                      /*miso*/ -1, /*host*/ SPI3_HOST, false);
    
    // 初始化显示驱动
    lvgl_gfx = new Arduino_NV3041A(lvgl_bus,
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
    if (!lvgl_gfx->begin(80 * 1000 * 1000UL)) {  // 80 MHz
        Serial.println("显示硬件启动失败！");
        return false;
    }
    
    // 清屏
    lvgl_gfx->fillScreen(0x0000);
    
    // 初始化LVGL显示适配器
    lvgl_display = new LVGLDisplay(lvgl_gfx, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!lvgl_display->begin()) {
        Serial.println("LVGL显示系统初始化失败！");
        return false;
    }
    
    Serial.println("LVGL显示系统初始化成功！");
    return true;
}

// 创建Hello World UI
void createHelloWorldUI() {
    Serial.println("创建Hello World UI...");
    
    // 创建一个标签
    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello LVGL World!");
    
    // 设置标签样式
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    // 使用默认字体
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, LV_PART_MAIN);
    
    // 居中显示
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    
    // 创建一个简单的按钮
    lv_obj_t* btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 120, 40);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
    
    // 按钮标签
    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_center(btn_label);
    
    Serial.println("Hello World UI创建完成！");
}

// LVGL演示主函数
void lvglHelloWorldDemo() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== LVGL Hello World 演示 ===");
    
    // 初始化背光
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);
    
    // 初始化LVGL显示系统
    if (!initLVGLDisplay()) {
        Serial.println("LVGL初始化失败！");
        return;
    }
    
    // 创建Hello World UI
    createHelloWorldUI();
    
    Serial.println("演示启动成功！");
    
    // 主循环
    while (true) {
        if (lvgl_display) {
            lvgl_display->tick();
        }
        delay(1);
    }
}