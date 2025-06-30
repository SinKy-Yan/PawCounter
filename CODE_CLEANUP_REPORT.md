## 代码冗余分析报告

> 本报告基于对 `src/` 目录全部源码的静态检索与依赖关系追踪（`#include`、对象实例化、函数调用等）结果，列出了当前工程中**明显**不再被主程序引用或由更新模块取代的实现。建议在后续清理中逐步移除或重构，以缩减固件体积、降低维护成本。

### 1. 备份文件（*.bak）
| 文件 | 说明 | 建议 |
| ---- | ---- | ---- |
| `src/CalculatorModes.h.bak`<br>`src/CalculatorModes.cpp.bak` | 早期"模式枚举 + 逻辑"备份，已完全被 `KeyboardConfig / CalculatorCore` 体系替代，工程内无任何 `#include` / 引用 | 若无历史兼容需求，可直接删除 |

### 2. 旧版 Calculator 实现
| 文件 | 说明 | 现状 | 建议 |
| ---- | ---- | ---- | ---- |
| `src/Calculator.h`<br>`src/Calculator.cpp` | 基于 `Arduino_GFX` 的单文件简易计算器实现（含 `onKeyInput`, `performCalculation` 等） | 主程序 (`main.cpp`) 已改用 `CalculatorCore + CalculationEngine`；除自文件相互包含外无其他引用 | 代码功能已由新架构覆盖，可整体删除或迁移到 `archive/` 目录 |

### 3. 复杂显示子系统（未使用）
| 组件 | 说明 | 现状 | 建议 |
| ---- | ---- | ---- | ---- |
| `LCDDisplay`, `SerialDisplay`, `DualDisplay`（定义于 `CalculatorDisplay.{h,cpp}`） | 提供多区域排版、主题/单位/历史滚动等完整 UI 方案 | 工程内无任何 `new xxxDisplay` 实例；当前仅 `CalcDisplayAdapter` 继承其抽象基类 `CalculatorDisplay` 并桥接到 `CalcDisplay` | 若未来不打算回归复杂 UI，可：<br>1) 将 `CalculatorDisplay` 抽象基类拆分到独立头文件<br>2) 移除三种具体显示器实现，或移动到 `extras/` |
| `src/CalculatorDisplay.cpp` 内部大量纯串口调试打印 | 体积可观（>600 行），且仅被上表类使用 | 同上 |

### 4. BatteryManager (可选模块)
| 文件 | 说明 | 状态 | 建议 |
| ---- | ---- | ---- | ---- |
| `src/BatteryManager.{h,cpp}` | MAX17048 电量计 + TP4056 状态检测 | 在 `config.h` 中默认 **未启用** (`ENABLE_BATTERY_MANAGER` 注释)，主循环仅在宏开启时调用 `.begin()` | 若硬件长期无电池方案，可移除整个模块，并删除 `main.cpp` 中相关代码分支 |

### 5. 其它零散冗余
| 类型 | 示例 | 说明 / 处理建议 |
| ---- | ---- | ---- |
| 旧注释 / TODO | `KeypadControl.cpp` 中关于"蜂鸣器已移至 BuzzerSoundManager" | 确认功能已实现后去除陈旧 TODO，保持代码清晰 |
| 重复宏 | `config.h` 中 LCD、LEDC 宏与真实硬件若不符 | 合并或删除无效定义，防止日后误用 |

---
#### 后续清理建议
1. **增量删减**：按优先级依次删除上表代码，每次删完重新编译验证。
2. **CI 编译体积监控**：加入 size-check，直观观察二进制大小变化。
3. **启用静态分析工具**：如 `cppcheck`、`clang-tidy`，持续发现未引用符号。
4. **文档同步**：每次删除模块后同步更新 README/设计文档，确保历史信息可追溯。

> 清理完成后预估可减少源码行数约 **3 k+**，固件体积减少 **15-20 KB**（取决于编译优化选项）。 