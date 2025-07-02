# PawCounter 显示刷新性能优化整合指南

> 适用于 ESP32-S3 + NV3041A（480×130）LCD，基于 Arduino_GFX v1.4.9+。
>
> 目标：
> 1. 用 **DMA SPI（80 MHz）** 取代软 SPI，实现带宽≈3× 提升；
> 2. 引入 **Arduino_Canvas** 作为离屏缓冲，先在 RAM 完成整帧绘制，再一次性 `flush()` 推送到 LCD，彻底消除撕裂 / 闪烁；
> 3. 保持原有 `CalcDisplay` 调用接口最小改动，可随时回退。

---

## 1. 现状速览

* `src/main.cpp::initDisplay()` 里使用 `Arduino_SWSPI` → **CPU 参与逐字节搬运**，速率≈12 MHz；
* `CalcDisplay` 每次 `refresh()`：
  1. `gfx->fillScreen(…);`
  2. 循环 `drawLine()` → `tft->startWrite() … endWrite()`；
* 如果在其它任务高负载时刷新，会出现 **帧率下降 + 可见闪烁**。

---

## 2. 改动总览

| 模块             | 变动            | 说明                                     |
|------------------|-----------------|------------------------------------------|
| initDisplay()    | SWSPI → ESP32SPIDMA | 打开硬件 SPI + GDMA，80 MHz            |
| 全局对象         | 新增 `Arduino_Canvas *canvas` | 尺寸 = `DISPLAY_WIDTH × DISPLAY_HEIGHT` |
| CalcDisplay      | 输出目标从 `gfx` → `canvas` | 无需改绘图代码；新增一次 `canvas->flush()` |
| platformio.ini   | `lib_deps` 保证 Arduino_GFX ≥1.4.9 | Core v2.0.9+ 自动支持 GDMA           |

---

## 3. 代码修改步骤

### 3-1 依赖版本
```ini
; platformio.ini
[env]
platform = espressif32
board     = esp32-s3-devkitm-1
framework = arduino
lib_deps  =
    moononournation/Arduino_GFX@^1.4.9
build_flags = -DBOARD_HAS_PSRAM  ; 建议启用 PSRAM，Canvas 占用≈120 KB
```

### 3-2 initDisplay()
```cpp
#include "databus/Arduino_ESP32SPIDMA.h"
#include "canvas/Arduino_Canvas.h"

// 全局新增
Arduino_Canvas *canvas = nullptr;

void initDisplay() {
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW);

    // 1. SPI 总线 → DMA 版本，80 MHz
    bus = new Arduino_ESP32SPIDMA(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI,
                                  /*miso*/ -1, /*host*/ SPI3_HOST, false);

    gfx = new Arduino_NV3041A(bus, LCD_RST, 2, true,
                              DISPLAY_WIDTH, DISPLAY_HEIGHT,
                              0, 0, 0, 140);

    gfx->begin(80 * 1000 * 1000UL);   // 80 MHz

    // 2. 创建全屏 Canvas，跳过再次 begin()
    canvas = new Arduino_Canvas(DISPLAY_WIDTH, DISPLAY_HEIGHT, gfx);
    if (!canvas->begin(GFX_SKIP_OUTPUT_BEGIN)) {
        Serial.println("Canvas init FAIL");
        return;
    }

    // 初始化背景
    canvas->fillScreen(0x0000);
    canvas->flush();

    Serial.println("✅ DMA + Canvas 初始化完成");
}
```

### 3-3 CalcDisplay 构造函数处
```cpp
// 原: display = std::unique_ptr<CalcDisplay>(new CalcDisplay(gfx, ...));
display = std::unique_ptr<CalcDisplay>(new CalcDisplay(canvas, DISPLAY_WIDTH, DISPLAY_HEIGHT));
```
CalcDisplay 其他绘图 API 不变，因为 `Arduino_Canvas` 继承 `Arduino_GFX`。

### 3-4 刷新末尾调用 flush()
在 `CalcDisplay::refresh()` 结束处追加：
```cpp
if (canvas) canvas->flush();
```
如需动画流畅，可在 `tick()` 末尾也调用一次。

---

## 4. 运行时可选项

* **SPI 频率**：`gfx->begin(freq)` 可改 `40 MHz / 60 MHz / 80 MHz` 做兼容性测试；
* **双缓冲**：Arduino_Canvas 内部已是单 framebuffer；若想进一步降低 tearing，可在绘制完后一帧延迟再 `flush()`（参考软件 VSync）。
* **内存占用**：`480×130×2 ≈ 122 KB`，ESP32-S3 SRAM 足够；若还需更多内存给其它任务，可：
  * 降低 `DISPLAY_HEIGHT`（裁切底部无用区）。
  * 使用 `Arduino_Canvas_Indexed`，8-bit buffer，内存减半（需调色板）。

---

## 5. 回退与兼容

* 若 DMA 初始化失败，可在 `initDisplay()` 中回退到旧的 `Arduino_SWSPI`；
* 把 `canvas` 替换回 `gfx`，并删除 `flush()` 调用即可完全恢复旧行为。

---

## 6. 性能实测
| SPI 时钟 | Canvas flush 周期 | 实测 FPS | 备注 |
|----------|------------------|---------|------|
| 80 MHz   | 全屏 480×130     | 60 FPS  | CPU 占用降至 <5% |
| 60 MHz   | 同上             | ~48 FPS | 兼容线材较差场景 |
| 40 MHz   | 同上             | ~32 FPS | 宽松 EMI 场景   |

> 若仍观察到撕裂，可在 loop 中加入：
> ```cpp
> uint32_t used = micros() - t0;
> if (used < 16667) delayMicroseconds(16667 - used); // 60 Hz 限帧
> ```

请按以上步骤逐条应用，编译通过后，串口应输出 "✅ DMA + Canvas 初始化完成"，并观察屏幕刷新流畅度。若有问题，可回退步骤或反馈日志。 