/**
 * @file CalcDisplayAdapter.h
 * @brief CalcDisplay适配器，用于与CalculatorCore兼容
 */

#ifndef CALC_DISPLAY_ADAPTER_H
#define CALC_DISPLAY_ADAPTER_H

#include "CalculatorDisplay.h"
#include "calc_display.h"

// 前向声明
class CalculatorCore;

class CalcDisplayAdapter : public CalculatorDisplay {
private:
    CalcDisplay* _calcDisplay;
    CalculatorCore* _calculator;
    size_t _lastHistorySize;
    
public:
    CalcDisplayAdapter(CalcDisplay* calcDisplay) : _calcDisplay(calcDisplay), _calculator(nullptr), _lastHistorySize(0) {}
    
    void setCalculatorCore(CalculatorCore* calculator) { _calculator = calculator; }
    
    bool begin() override { return true; }
    
    void updateDisplay(const String& mainText, const String& expression, CalculatorState state) override {
        bool historyUpdated = false;
        
        // 更新历史记录显示
        if (_calculator) {
            const auto& history = _calculator->getHistory();
            
            // 格式化最新的两条历史记录
            String newLatest = "";
            String newOlder = "";
            
            if (history.size() >= 1) {
                const auto& latest = history[history.size() - 1];
                newLatest = latest.expression + "=" + String(latest.result, 2);
            }
            
            if (history.size() >= 2) {
                const auto& older = history[history.size() - 2];
                newOlder = older.expression + "=" + String(older.result, 2);
            }
            
            // 更新历史记录显示
            _calcDisplay->updateHistoryDirect(newLatest, newOlder);
            
            // 检查是否有新记录添加
            if (history.size() > _lastHistorySize) {
                _lastHistorySize = history.size();
                historyUpdated = true;
            }
        }
        
        // 更新表达式和结果
        _calcDisplay->updateExprDirect(expression.isEmpty() ? "" : expression);
        _calcDisplay->updateResultDirect(mainText);
        
        // 统一进行一次全屏刷新
        _calcDisplay->refresh();
        
        // 同时输出到串口
        Serial.println("=== 计算器显示 ===");
        if (!expression.isEmpty()) {
            Serial.println("表达式: " + expression);
        }
        Serial.println("结果: " + mainText);
        
        // 显示历史记录状态
        if (_calculator) {
            const auto& history = _calculator->getHistory();
            Serial.printf("历史记录数量: %d\n", history.size());
            if (history.size() >= 1) {
                const auto& latest = history[history.size() - 1];
                Serial.println("最新历史: " + latest.expression + "=" + String(latest.result, 2));
            }
            if (history.size() >= 2) {
                const auto& older = history[history.size() - 2];
                Serial.println("较旧历史: " + older.expression + "=" + String(older.result, 2));
            }
        }
        
        if (historyUpdated) {
            Serial.println("✓ 历史记录已更新");
        }
        Serial.println("================");
    }
    
    void showError(CalculatorError error, const String& message) override {
        _calcDisplay->updateResultDirect("Error: " + message);
        _calcDisplay->refresh();
        Serial.println("错误: " + message);
    }
    
    // 移除了showNotification - 基类中不存在此方法
    
    void clear() override {
        _calcDisplay->updateExprDirect("");
        _calcDisplay->updateResultDirect("0");
        _calcDisplay->refresh();
    }
    
    void showStatus(const String& message) override {
        Serial.println("状态: " + message);
    }
    
    void setTheme(const DisplayTheme& theme) override {
        // 简化UI暂不支持主题
    }
    
    void setNumberFormat(const NumberFormat& format) override {
        // 简化UI暂不支持数字格式化
    }
    
    void setUnitDisplay(const UnitDisplay& unitDisplay) override {
        // 简化UI暂不支持单位显示
    }
};

#endif