# 代码清理行动计划

> 本计划基于 `CODE_CLEANUP_REPORT.md` 以及对现有源码的依赖扫描结果，列出可立即执行的删除/迁移步骤与注意事项，供代码自动化模型（或人工）按序实施。

---

## 0. 概览

| 类别 | 目标 | 风险评估 |
| ---- | ---- | -------- |
| 备份文件 (`*.bak`) | 无任何引用 | ❌ 零风险 |
| 旧版 Calculator 实现 | 无外部 `#include` / 实例化 | ⚠️ 极低 |
| 复杂显示子系统 | 仅抽象基类仍被使用 | ⚠️ 低 |
| BatteryManager 模块 | 编译链路通过宏隔离 | ⚠️ 取决于硬件需求 |
| TODO / 重复宏 | 文字级修改 | ❌ 零风险 |

---

## 1. 分阶段执行列表

> **建议在独立分支（如 `feature/cleanup`）进行，每完成一阶段即 `pio run` 验证并提交。**

### 1.1 移除备份文件
- [ ] 删除 `src/CalculatorModes.h.bak`
- [ ] 删除 `src/CalculatorModes.cpp.bak`

### 1.2 移除旧版 `Calculator.*`
- [ ] 删除 `src/Calculator.h`
- [ ] 删除 `src/Calculator.cpp`
- [ ] *无其他改动；验证编译通过*

### 1.3 精简显示子系统
1. 拆分抽象基类：
   - [ ] 在 `include/` 创建 `CalculatorDisplay_base.h`，复制 `CalculatorDisplay` 纯虚接口部分。
2. 适配器改头文件：
   - [ ] `CalcDisplayAdapter.h` 改为 `#include "CalculatorDisplay_base.h"`。
3. 删除未用实现：
   - [ ] 删除 `LCDDisplay`, `SerialDisplay`, `DualDisplay` 相关代码段（位于 `CalculatorDisplay.{h,cpp}`）。
4. 若编译器提示缺失符号，再次确认依赖并调整。

### 1.4 BatteryManager（可选）
- 若确认 **永久无电池**：
  1. [ ] 删除 `src/BatteryManager.{h,cpp}`
  2. [ ] 删除 `#include "BatteryManager.h"` 以及 `batteryManager` 变量/调用。
  3. [ ] 清理 `#ifdef ENABLE_BATTERY_MANAGER` 分支。
- 若保留备选：跳过。

### 1.5 零散清理
- [ ] grep `TODO` 或过期注释，删除已完成条目。
- [ ] 合并 `config.h` 多余宏（LCD/LED 引脚等）。

---

## 2. 编译 & 体积监控

1. 每步执行 `pio run` ；若项目有 CI，可启用 size-check 插件，追踪二进制大小变化。
2. 最终预计减少源码 **≈3k 行**，固件体积 **15–20 KB**。

---

## 3. 风险与回滚

- **抽象基类拆分**：务必统一 `#include` 路径，避免循环依赖。
- **BatteryManager**：删除后需同步移除日志/反馈调用，否则链接失败。
- 保留 `archive/` 目录或 Git 历史，随时回滚。

---

## 4. 文档同步

- [ ] 更新 `CLAUDE.md`、README 中的模块说明与硬件描述，删除已废弃内容。

---

> 执行完以上步骤后，可提交 PR 并请求评审。 