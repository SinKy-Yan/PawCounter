#ifndef CALCULATORAPPLICATION_H
#define CALCULATORAPPLICATION_H

#include <Arduino.h>
#include <memory>
#include <queue>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "TaskManager.h"  // 为了使用TaskKeyEvent和DisplayUpdate结构
#include "KeypadControl.h"  // 为了使用KeyEventType枚举

// 前向声明
class CalculatorCore;
class LVGLDisplay;
class SimpleHID;
class FontTester;
class CalculationEngine;
class UIManager;
class UIOptimizer;

// 应用状态枚举
enum class ApplicationState {
    INITIALIZING,
    CALCULATOR_MODE,
    FONT_TEST_MODE,
    SETTINGS_MODE,
    ERROR_MODE
};

// 按键映射结构
struct KeyMapping {
    uint8_t physicalKey;
    uint8_t logicalKey;
    String keyName;
    std::function<void()> action;
};

// LED效果配置
struct LEDEffect {
    uint8_t ledIndex;
    uint32_t color;
    uint32_t duration;
    uint32_t fadeTime;
    bool isActive;
    uint32_t startTime;
};

// 蜂鸣器效果配置
struct BuzzerEffect {
    uint16_t frequency;
    uint32_t duration;
    uint32_t startTime;
    bool isActive;
};

// 计算器应用类
class CalculatorApplication {
public:
    CalculatorApplication();
    ~CalculatorApplication();
    
    // 初始化和管理
    bool initialize();
    void shutdown();
    
    // 状态管理
    ApplicationState getState() const { return currentState; }
    void setState(ApplicationState state);
    
    // 按键相关方法
    void scanKeypadMatrix();
    bool getKeyEvent(TaskKeyEvent* event);
    void handleKeyEvent(const TaskKeyEvent& event);
    
    // 显示相关方法
    void updateDisplay();
    void handleDisplayUpdate(const DisplayUpdate& update);
    void refreshDisplay();
    
    // LED效果相关方法
    void updateLEDEffects();
    void triggerLEDEffect(uint8_t ledIndex, uint32_t color, uint32_t duration, uint32_t fadeTime);
    void clearAllLEDs();
    
    // 蜂鸣器相关方法
    void updateBuzzer();
    void triggerBuzzer(uint16_t frequency, uint32_t duration);
    void stopBuzzer();
    
    // 计算器功能
    void processCalculatorInput(uint8_t key, bool isLongPress);
    void executeCalculation();
    void clearCalculator();
    
    // 应用模式切换
    void switchToCalculatorMode();
    void switchToFontTestMode();
    void switchToSettingsMode();
    void switchToUIPage(uint8_t pageIndex);
    
    // 错误处理
    void handleApplicationError(const String& error);
    
    // 组件访问接口
    CalculatorCore* getCalculatorCore() const { return calculatorCore.get(); }
    LVGLDisplay* getLVGLDisplay() const { return lvglDisplay; }
    KeypadControl* getKeypadControl() const { return keypadControl.get(); }
    
private:
    // 核心组件
    std::unique_ptr<CalculatorCore> calculatorCore;
    LVGLDisplay* lvglDisplay;  // 不管理生命周期，只是引用全局对象
    std::unique_ptr<KeypadControl> keypadControl;
    std::unique_ptr<SimpleHID> simpleHID;
    std::unique_ptr<FontTester> fontTester;
    std::shared_ptr<CalculationEngine> calculationEngine;  // 修改为shared_ptr
    std::unique_ptr<UIManager> uiManager;       // UI管理器
    std::unique_ptr<UIOptimizer> uiOptimizer;   // UI优化器
    
    // 应用状态
    ApplicationState currentState;
    ApplicationState previousState;
    uint32_t lastStateChangeTime;
    
    // 按键处理
    std::queue<TaskKeyEvent> keyEventQueue;
    std::vector<KeyMapping> keyMappings;
    SemaphoreHandle_t keyEventMutex;
    
    // 显示管理
    std::queue<DisplayUpdate> displayUpdateQueue;
    SemaphoreHandle_t displayMutex;
    bool displayNeedsUpdate;
    uint32_t lastDisplayUpdate;
    
    // LED效果管理
    std::vector<LEDEffect> activeLEDEffects;
    SemaphoreHandle_t ledMutex;
    uint32_t lastLEDUpdate;
    
    // 蜂鸣器管理
    BuzzerEffect currentBuzzerEffect;
    SemaphoreHandle_t buzzerMutex;
    
    // 性能统计
    uint32_t keyEventCount;
    uint32_t displayUpdateCount;
    uint32_t ledUpdateCount;
    uint32_t buzzerEventCount;
    
    // 错误处理
    std::vector<String> errorLog;
    uint32_t errorCount;
    
    // 静态实例指针（用于静态回调函数）
    static CalculatorApplication* _currentInstance;
    
    // 内部方法
    void initializeComponents();
    void setupKeyMappings();
    void setupDefaultLEDEffects();
    void setupDefaultBuzzerSettings();
    
    // 按键处理内部方法
    void onKeypadEvent(KeyEventType type, uint8_t key, uint8_t* combo, uint8_t count);
    static void staticKeyEventCallback(KeyEventType type, uint8_t key, uint8_t* combo, uint8_t count);
    void processKeyPress(uint8_t key);
    void processKeyRelease(uint8_t key);
    void processKeyLongPress(uint8_t key);
    void processKeyRepeat(uint8_t key);
    void processKeyCombo(uint8_t* keys, uint8_t count);
    
    // 显示处理内部方法
    void updateCalculatorDisplay();
    void updateFontTestDisplay();
    void updateSettingsDisplay();
    void updateErrorDisplay();
    
    // LED效果内部方法
    void updateSingleLED(LEDEffect& effect);
    void calculateLEDColor(const LEDEffect& effect, uint32_t& color);
    void setLEDColor(uint8_t ledIndex, uint32_t color);
    
    // 蜂鸣器内部方法
    void updateBuzzerEffect();
    void startBuzzerTone(uint16_t frequency);
    void stopBuzzerTone();
    
    // 计算器逻辑内部方法
    void handleNumberInput(uint8_t digit);
    void handleOperatorInput(char op);
    void handleFunctionInput(const String& function);
    void handleSpecialInput(const String& special);
    
    // 模式切换内部方法
    void cleanupCurrentMode();
    void initializeNewMode();
    
    // 线程安全方法
    void lockKeyEvents();
    void unlockKeyEvents();
    void lockDisplay();
    void unlockDisplay();
    void lockLEDs();
    void unlockLEDs();
    void lockBuzzer();
    void unlockBuzzer();
    
    // 性能监控
    void updatePerformanceStats();
    void logPerformanceStats();
    
    // 常量定义
    static constexpr uint32_t DISPLAY_UPDATE_INTERVAL = 20;  // 20ms, 50fps
    static constexpr uint32_t LED_UPDATE_INTERVAL = 10;      // 10ms, 100fps
    static constexpr uint32_t BUZZER_UPDATE_INTERVAL = 5;    // 5ms, 200fps
    static constexpr uint32_t PERFORMANCE_LOG_INTERVAL = 10000; // 10秒
    static constexpr uint32_t MAX_ERROR_LOG_SIZE = 20;       // 最多20条错误日志
    static constexpr uint32_t MAX_LED_EFFECTS = 22;          // 最多22个LED效果
    static constexpr uint32_t MAX_KEY_EVENTS = 50;           // 最多50个按键事件
    static constexpr uint32_t MAX_DISPLAY_UPDATES = 20;      // 最多20个显示更新
};

// 应用相关的实用工具函数
namespace ApplicationUtils {
    // 按键映射工具
    String getKeyName(uint8_t key);
    uint8_t getKeyFromName(const String& name);
    
    // 颜色工具
    uint32_t blendColors(uint32_t color1, uint32_t color2, float ratio);
    uint32_t adjustBrightness(uint32_t color, float brightness);
    
    // 时间工具
    uint32_t getElapsedTime(uint32_t startTime);
    float getProgressRatio(uint32_t startTime, uint32_t duration);
    
    // 效果工具
    float easeInOut(float t);
    float easeIn(float t);
    float easeOut(float t);
}

#endif // CALCULATORAPPLICATION_H