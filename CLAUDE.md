# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

这是一个基于ESP32-S3的计算器项目，使用PlatformIO开发。

### 硬件特性
- ESP32-S3 DevKit-M-1开发板  
- 480x128 LCD显示屏，带背光控制
- 22键物理键盘，每个按键都有独立LED反馈
- 电池管理系统（MAX17048电量计 + TP4056充电器）
- 蜂鸣器音频反馈
- WS2812 RGB LED用于按键照明

### 关键配置开关
在 `config.h` 中可配置的功能：
```cpp
#define DEBUG_MODE                    // 启用调试模式
// #define ENABLE_BATTERY_MANAGER     // 启用电池管理（调试时可注释）
// #define ENABLE_LVGL_UI             // 启用LVGL界面（注释掉使用Arduino_GFX）
```

## 构建命令

根据开发环境选择对应的命令：

**Windows (WSL):**
```bash
# 编译项目
/mnt/c/.platformio/penv/Scripts/pio.exe run

# 上传到设备
/mnt/c/.platformio/penv/Scripts/pio.exe run --target upload

# 监控串口输出
/mnt/c/.platformio/penv/Scripts/pio.exe device monitor

# 清理构建文件
/mnt/c/.platformio/penv/Scripts/pio.exe run --target clean

# 一键编译并上传
/mnt/c/.platformio/penv/Scripts/pio.exe run --target upload --target monitor
```

**macOS/Linux:**
```bash
# 编译项目 (使用完整路径)
/Users/sinky/.platformio/penv/bin/pio run

# OTA功能已移除

# 上传到设备
/Users/sinky/.platformio/penv/bin/pio run --target upload

# OTA无线上传功能已移除

# 监控串口输出
/Users/sinky/.platformio/penv/bin/pio device monitor

# 清理构建文件
/Users/sinky/.platformio/penv/bin/pio run --target clean

# 查看Arduino框架版本
/Users/sinky/.platformio/penv/bin/pio pkg show framework-arduinoespressif32

# 查看ESP32平台版本
/Users/sinky/.platformio/penv/bin/pio platform show espressif32
```

**当前环境版本信息:**
- ESP32 Platform: 6.11.0 (2025年5月发布)
- Arduino ESP32 Framework: 3.20017.241212 (2024年12月发布)

## 架构概览

### 核心组件

项目采用模块化架构，主要组件包括：

1. **硬件抽象层**
   - `config.h`: 硬件配置和引脚定义中心
   - 统一的硬件初始化流程（12步初始化）

2. **输入系统** 
   - `KeypadControl`: 高级键盘管理，支持防抖、长按检测、LED反馈
   - 移位寄存器扫描22个物理按键
   - 支持组合键和多种事件类型

3. **显示系统**
   - 双UI系统：Arduino_GFX（轻量级）或 LVGL（现代UI）
   - `BackLightControl`: PWM背光控制，支持平滑渐变
   - 480x128像素显示，支持180度软件旋转

4. **电源管理**
   - `BatteryManager`: MAX17048电量计 + TP4056充电管理
   - 低电量检测和警告（可配置禁用）

5. **计算器核心**
   - `CalculatorCore`: 可扩展的计算器架构
   - `CalculationEngine`: 高精度计算引擎
   - 支持基本、科学、财务计算模式
   - 完整的历史记录和错误处理

6. **日志系统**
   - `Logger`: 统一日志管理，支持多级别（ERROR到VERBOSE）
   - 模块化标签，彩色输出
   - 串口命令动态控制日志级别

### 关键设计模式

- **单例模式**: `BacklightControl`和`Logger`使用单例模式便于全局访问
- **事件驱动架构**: `KeypadControl`使用回调系统处理按键事件
- **硬件抽象**: 所有引脚定义集中在`config.h`中
- **状态管理**: 每个组件维护自己的状态并具有更新循环
- **模块化日志**: 每个组件使用专用的日志标签，便于调试和监控

### 硬件集成

系统集成了多个硬件子系统：
- **SPI显示**: LCD的自定义并行接口
- **移位寄存器扫描**: 键盘矩阵的串行输入
- **I2C电池监控**: MAX17048通信
- **PWM控制器**: 用于背光、蜂鸣器和LED效果的LEDC
- **RMT协议**: 通过FastLED控制WS2812 LED灯带

### 按键映射

物理按键通过`LED_TO_KEY_MAP`和`KEY_MATRIX`数组进行映射。计算器支持：
- 标准数字输入（0-9）
- 基本运算符（+、-、*、/）
- 特殊功能（清除、等号、小数点）
- 电源和蓝牙按键（预留扩展）

## 开发注意事项

- 所有时间敏感操作使用`millis()`进行非阻塞执行
- 硬件初始化必须遵循特定顺序（显示屏 → LED → 键盘 → 电池）
- 主循环需要定期调用各组件的`update()`方法以确保正常运行
- 在config.h中定义`DEBUG_MODE`时可获得调试输出
- 使用新的ESP32 LEDC API：`ledcAttach()`、`ledcAttachChannel()`、`ledcWrite()`，而不是已弃用的`ledcSetup()`和`ledcAttachPin()`

## 常用串口命令

系统提供了丰富的串口命令用于调试和控制：

### 基础命令
- `help` / `h` - 显示完整命令帮助
- `status` / `s` - 显示系统运行状态  
- `clear` / `c` - 清除显示
- `test` - 执行系统测试

### 硬件控制
- `backlight [0-100]` - 设置背光亮度
- `battery` / `b` - 显示电池状态（需启用电池管理）

### 计算器功能
- `mode` / `m` - 切换计算模式（基本↔财务）
- `financial` - 财务模式演示

### 调试功能
- `layout` - 显示按键布局图
- `keymap` - 按键映射测试
- `log [e/w/i/d/v]` - 设置日志级别
- `rotate [0-1]` - 测试显示旋转

## 日志系统

项目集成了完整的日志管理系统，支持多级别日志输出：

### 日志级别
- `ERROR` - 错误信息，系统异常
- `WARN` - 警告信息，潜在问题  
- `INFO` - 重要信息，系统状态
- `DEBUG` - 调试信息，详细流程
- `VERBOSE` - 详细信息，完整数据

### 使用示例
```cpp
// 通用日志宏
LOG_E(TAG_KEYPAD, "Error message");
LOG_I(TAG_KEYPAD, "Info message");

// 模块专用宏
KEYPAD_LOG_I("Key %d pressed", keyNum);
DISPLAY_LOG_D("Backlight set to %d%%", brightness);
BATTERY_LOG_W("Low battery: %d%%", percentage);
```

### 运行时控制
通过串口命令动态调整日志级别：
- `log e` - 设置为ERROR级别
- `log w` - 设置为WARN级别
- `log i` - 设置为INFO级别
- `log d` - 设置为DEBUG级别
- `log v` - 设置为VERBOSE级别

## 按键布局

22键物理键盘布局：
```
┌───────┬───────┬───────┬───────┬───────┐
│Key 1  │Key 6  │Key 10 │Key 15 │Key 19 │
│ON/OFF │  BT   │ PCT   │  C    │ DEL   │
├───────┼───────┼───────┼───────┼───────┤
│Key 2  │Key 7  │Key 11 │Key 16 │Key 20 │
│  7    │  8    │  9    │ MUL   │ +/-   │
├───────┼───────┼───────┼───────┼───────┤
│Key 3  │Key 8  │Key 12 │Key 17 │Key 21 │
│  4    │  5    │  6    │ SUB   │ DIV   │
├───────┼───────┼───────┼───────┼───────┤
│Key 4  │Key 9  │Key 13 │Key 18 │Key 22 │
│  1    │  2    │  3    │ ADD   │ EQ    │
├───────┼───────┼───────┴───────┴───────┤
│Key 5  │Key 14 │                       │
│  0    │  .    │                       │
└───────┴───────┴───────────────────────┘
```

## 故障排除

### 按键问题
1. 使用 `layout` 命令查看按键布局
2. 使用 `keymap` 命令进入测试模式验证按键功能
3. 检查串口日志中的详细计算过程
4. 使用 `status` 命令查看当前状态信息

### 显示问题
- 如果显示倒置，项目已实现180度软件旋转修复
- 使用 `rotate [0-1]` 命令测试不同角度
- 检查背光设置：`backlight [0-100]`

### 电池管理问题
- 调试时可在 `config.h` 中注释 `ENABLE_BATTERY_MANAGER` 
- 使用 `battery` 命令查看电池状态

## 计算器功能特性

### 双UI系统支持
- **Arduino_GFX**: 轻量级UI，适合基础功能
- **LVGL**: 现代化UI框架，支持复杂动画和主题
- 通过 `ENABLE_LVGL_UI` 宏切换

### 计算模式
- **基本模式**: 四则运算、百分比、平方根
- **财务模式**: 货币格式、单位显示、金额分解
- **科学模式**: 三角函数、对数、指数（预留）

### 财务模式特色
财务模式提供中文单位显示：
```
12345.67 显示为:
- LCD: ¥12,345.67 
- 串口: 
  总金额: ¥12,345.67
  1万 2千 3百 4十 5个 0.67元
```

### 扩展开发指南

#### 添加新计算模式
1. 继承`CalculatorMode`基类
2. 实现必要的虚函数
3. 定义专用的按键映射
4. 注册到计算器核心

#### 添加自定义数学函数
1. 继承`CalculationFunction`基类
2. 实现`calculate()`方法
3. 通过`CalculationEngine::addFunction()`注册

## 性能和内存信息

### 编译结果（参考）
- **RAM使用**: ~23.6% (77,496/327,680 bytes) - 使用LVGL时
- **Flash使用**: ~34.6% (726,368/2,097,152 bytes) - 包含所有功能
- **编译状态**: 成功，无错误或警告

### 初始化流程（12步）
系统采用有序初始化确保硬件稳定：
1. 串口通信初始化
2. 日志系统初始化
3. 显示系统初始化
4. LED系统初始化
5. 背光控制初始化
6. 反馈系统初始化
7. 键盘系统初始化
8. 电池管理初始化（可选）
9. 计算引擎初始化
10. 显示管理器初始化
11. 计算器核心初始化
12. 启动效果播放

