#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// LCD 引脚定义
#define LCD_RST   48
#define LCD_CS    45
#define LCD_DC    38
#define LCD_SCK   39
#define LCD_BL    42
#define LCD_MOSI  41

#define DISPLAY_WIDTH 480
#define DISPLAY_HEIGHT 128

Arduino_DataBus *bus = nullptr;
Arduino_GFX *gfx = nullptr;

void setup() {
    Serial.begin(115200);
    Serial.println("=== 计算器屏幕测试 ===");
    
    // 背光控制 - 确保在屏幕初始化后再开启
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW); // 先关闭背光
    
    Serial.println("1. 初始化显示总线...");
    bus = new Arduino_HWSPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);
    
    Serial.println("2. 初始化显示驱动...");
    gfx = new Arduino_NV3041A(bus, LCD_RST, 0, false, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, 0, 0);
    
    Serial.println("3. 启动显示硬件...");
    gfx->begin();
    
    // 延迟确保初始化完成
    delay(100);
    
    // 开启背光
    digitalWrite(LCD_BL, HIGH);
    Serial.println("4. 背光已开启");
    
    // 绘制彩虹条纹验证线序
    Serial.println("5. 绘制彩虹条纹验证线序...");
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        uint16_t color = gfx->color565(
            (x * 255) / DISPLAY_WIDTH,        // 红色渐变
            ((x * 2) % 255),                  // 绿色变化
            255 - ((x * 255) / DISPLAY_WIDTH) // 蓝色反向渐变
        );
        gfx->drawFastVLine(x, 0, DISPLAY_HEIGHT, color);
    }
    
    Serial.println("彩虹图已显示");
}

void loop() {
    // 循环闪烁测试
    static unsigned long lastTime = 0;
    static bool toggle = false;
    
    if (millis() - lastTime > 1000) {
        lastTime = millis();
        toggle = !toggle;
        
        if (gfx) {
            if (toggle) {
                gfx->fillScreen(0x0000); // 黑色
                gfx->setTextColor(0xFFFF);
                gfx->setCursor(10, 60);
                gfx->print("LCD OK - BLACK");
            } else {
                gfx->fillScreen(0x001F); // 蓝色
                gfx->setTextColor(0xFFFF);
                gfx->setCursor(10, 60);
                gfx->print("LCD OK - BLUE");
            }
        }
    }
}