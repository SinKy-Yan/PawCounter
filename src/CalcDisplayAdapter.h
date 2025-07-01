/**
 * @file CalcDisplayAdapter.h
 * @brief CalcDisplay适配器，用于与CalculatorCore兼容
 */

#ifndef CALC_DISPLAY_ADAPTER_H
#define CALC_DISPLAY_ADAPTER_H

#include "CalculatorDisplay_base.h"
#include "calc_display.h"
#include "NumberFormatter.h"

// 前向声明
class CalculatorCore;

class CalcDisplayAdapter : public CalculatorDisplay {
private:
    CalcDisplay* _calcDisplay;
    CalculatorCore* _calculator;
    size_t _lastHistorySize;
    
    // 状态缓存用于差异检测
    String _prevMainText;
    String _prevExpression;
    CalculatorState _prevState;
    
public:
    CalcDisplayAdapter(CalcDisplay* calcDisplay) : _calcDisplay(calcDisplay), _calculator(nullptr), 
        _lastHistorySize(0), _prevMainText("0"), _prevExpression(""), _prevState(CalculatorState::INPUT_NUMBER) {}
    
    void setCalculatorCore(CalculatorCore* calculator) { _calculator = calculator; }
    
    bool begin() override { return true; }
    
    void updateDisplay(const String& mainText, const String& expression, CalculatorState state) override {
        // 检测变化并触发相应动画
        bool mainTextChanged = (mainText != _prevMainText);
        bool expressionChanged = (expression != _prevExpression);
        bool stateChanged = (state != _prevState);
        bool animationTriggered = false;
        
        // 根据变化类型触发动画
        if (mainTextChanged && state == CalculatorState::INPUT_NUMBER) {
            // A1/A2动画：输入数字时的字符滑入/滑出
            _calcDisplay->animateInputChange(_prevMainText, mainText);
            animationTriggered = true;
        } else if (expressionChanged && !_prevExpression.isEmpty() && !expression.isEmpty()) {
            // B动画：运算符输入时数字上移到表达式区
            if (state == CalculatorState::INPUT_OPERATOR) {
                _calcDisplay->animateMoveInputToExpr(_prevMainText, expression);
                animationTriggered = true;
            }
        }
        
        // 如果没有动画，按原逻辑处理
        if (!animationTriggered) {
        bool historyUpdated = false;
        
        // 更新历史记录显示
        if (_calculator) {
            const auto& history = _calculator->getHistory();
            
            // 检查是否有新记录添加
            if (history.size() > _lastHistorySize) {
                // 只有在历史记录实际增加时才更新显示
                _lastHistorySize = history.size();
                historyUpdated = true;
                
                // 格式化最新的两条历史记录
                String newLatest = "";
                String newOlder = "";
                
                if (history.size() >= 1) {
                    const auto& latest = history[history.size() - 1];
                    newLatest = latest.expression + "=" + NumberFormatter::format(latest.result);
                }
                
                if (history.size() >= 2) {
                    const auto& older = history[history.size() - 2];
                    newOlder = older.expression + "=" + NumberFormatter::format(older.result);
                }
                
                // 更新历史记录显示
                _calcDisplay->updateHistoryDirect(newLatest, newOlder);
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
                Serial.println("最新历史: " + latest.expression + "=" + NumberFormatter::format(latest.result));
            }
            if (history.size() >= 2) {
                const auto& older = history[history.size() - 2];
                Serial.println("较旧历史: " + older.expression + "=" + NumberFormatter::format(older.result));
            }
        }
        
        if (historyUpdated) {
            Serial.println("✓ 历史记录已更新");
        }
        Serial.println("================");
        }
        
        // 更新状态缓存
        _prevMainText = mainText;
        _prevExpression = expression;
        _prevState = state;
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