# OTA 更新集成指导（面向代码模型）

> 本指南面向 **代码自动修改模型**，描述在现有 ESP32-S3 计算器项目中集成 OTA 固件升级所需的 _具体文件操作_、_增删代码片段_ 与 _PlatformIO 配置_。请严格按序号执行，每一步后均应重新编译 (`pio run`) 以确保无误。

---

## 1. 修改 `platformio.ini`

1. 在对应环境段（如 `[env:esp32s3]`）追加：
   ```ini
   upload_protocol = espota
   build_flags = 
       -DOTA_ENABLED
   ```
2. 若固件体积 > 1 MB，请切换双分区表，例如：
   ```ini
   board_build.partitions = default_2MB.csv  ; 或 huge_app.csv
   ```

---

## 2. 新增 OTA 功能代码

### 2.1 在 `include/` 目录新建 `OTAUpdater.h`
```cpp
#pragma once
#ifdef OTA_ENABLED
#include <ArduinoOTA.h>
#include <WiFi.h>

namespace OTA {
void begin(const char* ssid, const char* pwd);
void handle();
} // namespace OTA
#endif // OTA_ENABLED
```

### 2.2 在 `src/` 目录新建 `OTAUpdater.cpp`
```cpp
#include "OTAUpdater.h"
#ifdef OTA_ENABLED
#include "FeedbackManager.h"  // 提供视觉/音效反馈，可选

void OTA::begin(const char* ssid, const char* pwd) {
    WiFi.begin(ssid, pwd);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    ArduinoOTA.setHostname("esp32-calc");
    ArduinoOTA.setPassword("calc@2024"); // 可替换或删除

    ArduinoOTA.onStart([]() {
        FeedbackManager::getInstance().triggerSystemFeedback(SCENE_SYSTEM_STARTUP);
    });
    ArduinoOTA.onEnd([]() {
        FeedbackManager::getInstance().triggerSystemFeedback(SCENE_SYSTEM_SHUTDOWN);
    });
    ArduinoOTA.onError([](ota_error_t e) {
        Serial.printf("OTA Error %d\n", e);
    });
    ArduinoOTA.begin();
}

void OTA::handle() { ArduinoOTA.handle(); }
#endif // OTA_ENABLED
```

---

## 3. 修改 `main.cpp`

1. 头部添加：
   ```cpp
   #ifdef OTA_ENABLED
   #include "OTAUpdater.h"
   #endif
   ```
2. 在 `setup()` 中（通常日志系统初始化之后）调用：
   ```cpp
   #ifdef OTA_ENABLED
   OTA::begin("your_ssid", "your_password");
   #endif
   ```
3. 在 `loop()` 主循环最顶部插入：
   ```cpp
   #ifdef OTA_ENABLED
   OTA::handle();
   #endif
   ```

---

## 4. Wi-Fi 凭据管理（可选）

若不想硬编码，可：
1. 在 `config.h` 新增 `WIFI_SSID` / `WIFI_PASS` 宏；或
2. 使用 NVS / SPIFFS 保存并通过串口命令配置。

---

## 5. 串口命令扩展（可选）

在 `handleSerialCommands()` 添加：
```cpp
else if (command == "ota" || command == "update") {
    Serial.println("手动触发 OTA 更新");
    ArduinoOTA.handle();
}
```

---

## 6. 编译 & 测试步骤

1. USB 有线首次烧录：
   ```bash
   pio run --target upload
   ```
2. 与电脑处于同一局域网后执行 OTA 上传：
   ```bash
   pio run --target upload --upload-port esp32-calc.local
   ```
3. 观察串口输出，验证 `ArduinoOTA` 初始化成功、升级过程无错误。

---

## 7. 低电量保护（进阶，可选）

若启用 `BatteryManager`，可在 `OTA::begin()` 中添加：
```cpp
if (BatteryManager::getInstance().isLowBattery()) {
    Serial.println("电量过低，取消 OTA");
    return;
}
```

---

> 完成所有步骤并验证成功后，请提交 PR。若 OTA 功能需默认禁用，仅保留宏 `OTA_ENABLED` 的注释即可。 