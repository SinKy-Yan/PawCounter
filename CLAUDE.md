# CLAUDE.md

此文件为Claude Code (claude.ai/code)在处理此代码库时提供指导。

## 项目概述

这是一个基于ESP32-S3的计算器项目，使用PlatformIO开发。硬件特性包括：
- ESP32-S3 DevKit-M-1开发板
- 480x128 LCD显示屏，带背光控制
- 22键物理键盘，每个按键都有独立LED反馈
- 电池管理系统（MAX17048电量计 + TP4056充电器）
- 蜂鸣器音频反馈
- WS2812 RGB LED用于按键照明

## 构建命令

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

## 架构概览

### 核心组件

项目采用模块化架构，包含以下主要组件：

1. **硬件抽象层**
   - `config.h`: 硬件配置和引脚定义的中心
   - `Initialization.h/.cpp`: 显示屏、LED、动画的硬件初始化例程

2. **输入系统**
   - `KeypadControl`: 高级键盘管理，支持防抖、长按检测、自动重复、LED反馈和蜂鸣器
   - 使用移位寄存器扫描22个物理按键
   - 支持组合键和多种事件类型（按下/释放/长按/重复）

3. **显示系统**  
   - 使用Arduino_GFX库进行LCD控制
   - 480x128像素显示屏，支持自定义字体（`FreeSansBold10pt7b.h`）
   - `BackLightControl`: 基于PWM的背光控制，支持平滑渐变

4. **电源管理**
   - `BatteryManager`: MAX17048电量计集成，用于电压/电量百分比监控
   - TP4056充电电路状态监控
   - 低电量检测和警告

5. **应用逻辑**
   - `Calculator`: 主计算器引擎，包含显示缓冲区管理、算术运算和按键映射
   - 处理数字输入、运算符、小数点和结果计算

### 关键设计模式

- **单例模式**: `BacklightControl`使用单例模式便于全局访问
- **事件驱动架构**: `KeypadControl`使用回调系统处理按键事件
- **硬件抽象**: 所有引脚定义集中在`config.h`中
- **状态管理**: 每个组件维护自己的状态并具有更新循环

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

## 键盘扫描调试

项目包含详细的键盘扫描调试功能：
- 串口输出按键事件日志（格式：`[KEY] Key X - EVENT`）
- 原始扫描状态输出（`[SCAN]`标签）
- 按键状态检查详情（`[CHECK]`和`[BIT]`标签）
- 发送字符't'到串口可触发完整的24位状态测试

## 故障排除

如果遇到按键响应问题：
1. 检查串口监视器中的调试输出
2. 使用't'命令测试所有位状态
3. 验证`KEY_POSITIONS`数组与实际硬件连接是否匹配
4. 确保主循环中没有长时间阻塞操作