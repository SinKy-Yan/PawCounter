# 代码冗余与优化合并报告

本报告综合了先前的 `CODE_ANALYSIS.md` 与 `CODE_CLEANUP_REPORT.md` 两份文档内容，针对 `src/` 目录及相关资源做统一梳理，列出当前工程中的冗余/废弃代码及优化方向。

---
## 目录
1. 重复配置文件
2. 备份文件（*.bak）
3. 旧版 `Calculator` 实现
4. 复杂显示子系统（LCD/串口/DualDisplay）
5. `KeypadControl` 冗余功能
6. `CalculatorCore` 旧按键映射
7. `main.cpp` 直接硬件初始化
8. `BatteryManager` 可选模块
9. 未使用字体文件
10. 其它零散冗余
11. 简化整合建议
12. 后续清理建议

---
### 1. 重复配置文件
| 文件 | 问题 | 建议 |
| ---- | ---- | ---- |
| `lv_conf.h`（根目录 & `src/`） | 两处配置可能导致宏定义冲突、构建参数混淆 | 仅保留一份（推荐根目录），并在 `platformio.ini` 的 `build_flags` 指向该路径 |

### 2. 备份文件（*.bak）
| 文件 | 说明 | 建议 |
| ---- | ---- | ---- |
| `src/CalculatorModes.h.bak`<br>`src/CalculatorModes.cpp.bak` | 旧模式枚举/逻辑备份，已被 `KeyboardConfig` 体系取代 | 若无历史需求，可直接删除 |

### 3. 旧版 `Calculator` 实现
| 文件 | 说明 | 现状 | 建议 |
| ---- | ---- | ---- | ---- |
| `src/Calculator.h`<br>`src/Calculator.cpp` | 基于 `Arduino_GFX` 的早期单体实现 | 主程序已使用 `CalculatorCore + CalculationEngine` | 整体删除或移入 `archive/` |

### 4. 复杂显示子系统（未使用）
| 组件/文件 | 说明 | 现状 | 建议 |
| ---- | ---- | ---- | ---- |
| `LCDDisplay`, `SerialDisplay`, `DualDisplay`（`CalculatorDisplay.{h,cpp}`） | 支持主题/单位/历史滚动等高级 UI | 项目中无 `new` 实例，当前仅 `CalcDisplayAdapter` 继承基类 | 若不回退复杂 UI：<br>• 保留抽象基类，删除三具体实现<br>• 或整体迁至 `extras/` |
| `src/CalculatorDisplay.cpp` | 大量串口调试输出 | 仅为上表类服务 | 与实现同步处理 |

### 5. `KeypadControl` 冗余功能
| 问题 | 现状 | 建议 |
| ---- | ---- | ---- |
| 直接控制 LED/Buzzer (`handleLEDEffect`, `startBuzzer` 等) | 反馈逻辑已交由 `LEDEffectManager` & `BuzzerSoundManager` 统一 | 移除直接控制代码，仅保留按键扫描/事件分发，通过回调交给 `FeedbackManager` |

### 6. `CalculatorCore` 旧按键映射
| 问题 | 说明 | 建议 |
| ---- | ---- | ---- |
| `_keyMappings` 静态数组仍存在 | 已被 `KeyboardConfigManager` 的双层映射取代 | 删除数组及相关引用，使全部按键查询统一走 `KeyboardConfigManager` |

### 7. `main.cpp` 直接硬件初始化
| 问题 | 建议 |
| ---- | ---- |
| `initDisplay()/initLEDs()` 在 `setup()` 内手动拉脚 | 将显示/LED 初始化封装进 `LCDDisplay::begin()`、`LEDEffectManager::begin()` 等管理器；`setup()` 仅调用各 `begin()` |

### 8. `BatteryManager` 可选模块
| 文件 | 状态 | 建议 |
| ---- | ---- | ---- |
| `src/BatteryManager.{h,cpp}` | 默认为禁用 (`ENABLE_BATTERY_MANAGER` 注释) | 若硬件无电池，考虑移除模块及 `main.cpp` 分支 |

### 9. 未使用字体文件
| 文件 | 状态 | 建议 |
| ---- | ---- | ---- |
| `FreeSansBold10pt7b.h` | 未被 `LVGL` / `LCDDisplay` 引用 | 若确认无用，可删除以减小固件体积 |

### 10. 其它零散冗余
| 类型 | 示例 | 说明 / 建议 |
| ---- | ---- | ---- |
| 旧注释 / TODO | `KeypadControl.cpp` 关于"蜂鸣器已移至 BuzzerSoundManager" | 清理陈旧 TODO，保持注释准确 |
| 重复/无效宏 | `config.h` 中 LCD/LEDC 定义若与实硬件不符 | 合并或删除，防止误用 |

### 11. 简化整合建议
1. **单例收敛**：可将 `BacklightControl`, `BatteryManager` 等小型单例并入 `HardwareManager`，减少全局对象。
2. **模块层级**：保持 `Hardware -> Manager -> Core -> UI` 层次，避免交叉依赖。
3. **资源监控**：构建时使用 `size --format=berkeley` 或 `platformio run --target size` 监控体积。

### 12. 后续清理计划
1. 按上述顺序分批删除/重构，确保每步编译通过。
2. 在 CI 中加入尺寸阈值报警。
3. 引入 `cppcheck`/`clang-tidy`，持续检测未引用符号与潜在问题。
4. 每次删除模块后同步更新文档与 `CHANGELOG`。

> 预计清理后可减少源码行数 **≈3k+**，固件体积 **↓15–20 KB**（具体取决于编译优化）。
