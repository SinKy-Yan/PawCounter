#include "CalculatorApplication.h"

// 静态实例指针定义
CalculatorApplication* CalculatorApplication::_currentInstance = nullptr;
#include "CalculatorCore.h"
#include "LVGLDisplay.h"
#include "KeypadControl.h"
#include "SimpleHID.h"
#include "FontTester.h"
#include "CalculationEngine.h"
#include "Logger.h"
#include "config.h"
#include <FastLED.h>
#include <memory>
#include "BacklightControl.h"
#include "SleepManager.h"  // 添加SleepManager头文件

CalculatorApplication::CalculatorApplication() :
    currentState(ApplicationState::INITIALIZING),
    previousState(ApplicationState::INITIALIZING),
    lastStateChangeTime(0),
    lvglDisplay(nullptr),
    keyEventMutex(nullptr),
    displayMutex(nullptr),
    ledMutex(nullptr),
    buzzerMutex(nullptr),
    displayNeedsUpdate(false),
    lastDisplayUpdate(0),
    lastLEDUpdate(0),
    keyEventCount(0),
    displayUpdateCount(0),
    ledUpdateCount(0),
    buzzerEventCount(0),
    errorCount(0) {
    
    // 创建互斥锁
    keyEventMutex = xSemaphoreCreateMutex();
    displayMutex = xSemaphoreCreateMutex();
    ledMutex = xSemaphoreCreateMutex();
    buzzerMutex = xSemaphoreCreateMutex();
    
    // 初始化蜂鸣器效果
    currentBuzzerEffect = {0, 0, 0, false};
    
    // 预留容器空间
    activeLEDEffects.reserve(MAX_LED_EFFECTS);
    keyMappings.reserve(25);  // 22个按键 + 一些组合键
    errorLog.reserve(MAX_ERROR_LOG_SIZE);
}

CalculatorApplication::~CalculatorApplication() {
    shutdown();
    
    // 删除互斥锁
    if (keyEventMutex) {
        vSemaphoreDelete(keyEventMutex);
    }
    if (displayMutex) {
        vSemaphoreDelete(displayMutex);
    }
    if (ledMutex) {
        vSemaphoreDelete(ledMutex);
    }
    if (buzzerMutex) {
        vSemaphoreDelete(buzzerMutex);
    }
}

bool CalculatorApplication::initialize() {
    LOG_I("APP", "计算器应用初始化");
    
    try {
        // 初始化组件
        initializeComponents();
        
        // 设置按键映射
        setupKeyMappings();
        
        // 设置默认效果
        setupDefaultLEDEffects();
        setupDefaultBuzzerSettings();
        
        // 切换到计算器模式
        setState(ApplicationState::CALCULATOR_MODE);
        
        // 强制触发显示更新，确保初始UI正确显示
        displayNeedsUpdate = true;
        updateDisplay();
        
        LOG_I("APP", "计算器应用初始化完成");
        return true;
        
    } catch (const std::exception& e) {
        LOG_E("APP", "计算器应用初始化失败: %s", e.what());
        setState(ApplicationState::ERROR_MODE);
        return false;
    }
}

void CalculatorApplication::initializeComponents() {
    // 初始化计算引擎（创建一个独立的shared_ptr实例）
    auto engine = std::make_shared<CalculationEngine>();
    if (!engine->begin()) {
        throw std::runtime_error("计算引擎初始化失败");
    }
    
    // 获取LVGL显示对象的引用（不管理生命周期）
    // 从全局变量获取LVGL显示对象
    extern LVGLDisplay* lvgl_display;
    lvglDisplay = lvgl_display;
    if (!lvglDisplay) {
        LOG_W("APP", "LVGL显示对象未初始化，显示功能可能受限");
    }
    
    // 初始化按键控制
    keypadControl.reset(new KeypadControl());
    keypadControl->begin();  // KeypadControl::begin() 返回void
    
    // 设置按键事件回调函数（使用静态方法）
    keypadControl->setKeyEventCallback(CalculatorApplication::staticKeyEventCallback);
    _currentInstance = this;  // 设置当前实例指针供静态回调使用
    LOG_I("APP", "按键事件回调已设置");
    
    // 初始化计算器核心
    calculatorCore.reset(new CalculatorCore());
    if (!calculatorCore->begin()) {
        throw std::runtime_error("计算器核心初始化失败");
    }
    
    // 设置LVGL显示对象到计算器核心
    if (lvglDisplay) {
        calculatorCore->setLVGLDisplay(lvglDisplay);
    }
    
    // 设置计算引擎到计算器核心
    LOG_I("APP", "将计算引擎设置到计算器核心");
    calculatorCore->setCalculationEngine(engine);
    
    // 最后，将计算引擎赋值给成员变量
    calculationEngine = engine;
    
    // 初始化简单HID（可选）
    try {
        simpleHID.reset(new SimpleHID());
        if (!simpleHID->begin()) {
            LOG_W("APP", "简单HID初始化失败，继续运行");
            simpleHID.reset();
        }
    } catch (const std::exception& e) {
        LOG_W("APP", "简单HID初始化异常: %s", e.what());
        simpleHID.reset();
    }
    
    // 暂时跳过字体测试器初始化
    // 初始化字体测试器（可选）
    /*
    try {
        if (lvglDisplay) {
            fontTester.reset(new FontTester(lvglDisplay.get()));
            if (!fontTester->begin()) {
                LOG_W("APP", "字体测试器初始化失败");
                fontTester.reset();
            }
        }
    } catch (const std::exception& e) {
        LOG_W("APP", "字体测试器初始化异常: %s", e.what());
        fontTester.reset();
    }
    */
    
    LOG_I("APP", "应用组件初始化完成");
}

void CalculatorApplication::setupKeyMappings() {
    // 旧的按键映射系统已移除，现在统一使用KeyboardConfig系统
    // 这避免了双重映射系统的冲突问题
    
    // 清空旧映射（如果有的话）
    keyMappings.clear();
    
    LOG_I("APP", "按键映射设置完成，使用KeyboardConfig统一管理");
}

void CalculatorApplication::setupDefaultLEDEffects() {
    // 清除所有LED效果
    lockLEDs();
    activeLEDEffects.clear();
    unlockLEDs();
    
    LOG_I("APP", "LED效果设置完成");
}

void CalculatorApplication::setupDefaultBuzzerSettings() {
    lockBuzzer();
    currentBuzzerEffect.isActive = false;
    unlockBuzzer();
    
    LOG_I("APP", "蜂鸣器设置完成");
}

void CalculatorApplication::setState(ApplicationState state) {
    if (currentState != state) {
        LOG_I("APP", "应用状态变更: %d -> %d", static_cast<int>(currentState), static_cast<int>(state));
        
        previousState = currentState;
        currentState = state;
        lastStateChangeTime = millis();
        
        // 清理旧模式
        cleanupCurrentMode();
        
        // 初始化新模式
        initializeNewMode();
        
        // 标记显示需要更新
        displayNeedsUpdate = true;
    }
}

void CalculatorApplication::scanKeypadMatrix() {
    if (keypadControl) {
        keypadControl->update();
    }
}

bool CalculatorApplication::getKeyEvent(TaskKeyEvent* event) {
    if (!event) return false;
    
    lockKeyEvents();
    
    bool hasEvent = false;
    if (!keyEventQueue.empty()) {
        *event = keyEventQueue.front();
        keyEventQueue.pop();
        hasEvent = true;
        keyEventCount++;
    }
    
    unlockKeyEvents();
    
    return hasEvent;
}

void CalculatorApplication::handleKeyEvent(const TaskKeyEvent& event) {
    // 确保按键事件能唤醒系统
    SleepManager::instance().feed();
    
    // 根据事件类型处理按键
    switch (event.type) {
        case TaskKeyEventType::PRESS:
            processKeyPress(event.key);
            break;
            
        case TaskKeyEventType::RELEASE:
            processKeyRelease(event.key);
            break;
            
        case TaskKeyEventType::LONGPRESS:
            processKeyLongPress(event.key);
            break;
            
        case TaskKeyEventType::REPEAT:
            processKeyRepeat(event.key);
            break;
            
        case TaskKeyEventType::COMBO:
            processKeyCombo(const_cast<uint8_t*>(event.combo), event.comboCount);
            break;
    }
    
    // 发送HID事件（如果启用）
    if (simpleHID && simpleHID->isEnabled()) {
        bool isPress = (event.type == TaskKeyEventType::PRESS || event.type == TaskKeyEventType::LONGPRESS);
        simpleHID->handleKey(event.key, isPress);
    }
}

void CalculatorApplication::processKeyPress(uint8_t key) {
    LOG_D("APP", "处理按键按下: %d", key);
    
    // 触发LED效果
    uint32_t color = CRGB::White;  // 默认白色
    triggerLEDEffect(key - 1, color, 200, 100);  // key-1因为LED索引从0开始
    
    // 移除重复的蜂鸣器触发代码，因为KeypadControl已经处理了蜂鸣器反馈
    // 原代码：
    // uint16_t frequency = 2000;  // 默认频率
    // triggerBuzzer(frequency, 100);
    
    // 直接使用新的KeyboardConfig系统处理按键
    // 移除旧的映射系统，避免冲突
    if (calculatorCore) {
        try {
            calculatorCore->handleKeyInput(key, false);
        } catch (const std::exception& e) {
            handleApplicationError(String("按键处理错误: ") + e.what());
        }
    }
    
    // 标记显示需要更新
    displayNeedsUpdate = true;
}

void CalculatorApplication::processKeyRelease(uint8_t key) {
    LOG_D("APP", "处理按键释放: %d", key);
    // 按键释放通常不需要特殊处理
}

void CalculatorApplication::processKeyLongPress(uint8_t key) {
    LOG_D("APP", "处理长按: %d", key);
    
    // 长按可能有特殊功能
    switch (key) {
        case 16:  // 清除键长按 - 切换到设置模式
            if (currentState != ApplicationState::SETTINGS_MODE) {
                setState(ApplicationState::SETTINGS_MODE);
            } else {
                setState(ApplicationState::CALCULATOR_MODE);
            }
            break;
            
        case 17:  // 小数点键长按 - 切换字体测试模式
            if (currentState != ApplicationState::FONT_TEST_MODE) {
                setState(ApplicationState::FONT_TEST_MODE);
            } else {
                setState(ApplicationState::CALCULATOR_MODE);
            }
            break;
            
        default:
            // 其他键的长按处理（不调用processKeyPress以避免重复蜂鸣）
            if (calculatorCore) {
                try {
                    calculatorCore->handleKeyInput(key, true); // 传递true表示长按
                } catch (const std::exception& e) {
                    handleApplicationError(String("长按处理错误: ") + e.what());
                }
            }
            
            // 触发LED效果（但不触发蜂鸣器）
            uint32_t color = CRGB::White;
            triggerLEDEffect(key - 1, color, 300, 150);  // 长按LED效果时间略长
            
            // 标记显示需要更新
            displayNeedsUpdate = true;
            break;
    }
}

void CalculatorApplication::processKeyRepeat(uint8_t key) {
    LOG_D("APP", "处理按键重复: %d", key);
    
    // 重复事件处理（不调用processKeyPress以避免重复蜂鸣）
    if (calculatorCore) {
        try {
            calculatorCore->handleKeyInput(key, false);
        } catch (const std::exception& e) {
            handleApplicationError(String("重复按键处理错误: ") + e.what());
        }
    }
    
    // 触发LED效果（但不触发蜂鸣器）
    uint32_t color = CRGB::White;  // 默认白色
    triggerLEDEffect(key - 1, color, 100, 50);  // 重复按键LED效果时间略短
    
    // 标记显示需要更新
    displayNeedsUpdate = true;
}

void CalculatorApplication::processKeyCombo(uint8_t* keys, uint8_t count) {
    LOG_D("APP", "处理组合键，数量: %d", count);
    
    // 处理特定组合键
    if (count == 2) {
        // 双键组合
        if ((keys[0] == 1 && keys[1] == 2) || (keys[0] == 2 && keys[1] == 1)) {
            // 1+2组合 - 重启系统
            LOG_I("APP", "检测到重启组合键");
            ESP.restart();
        }
    }
}

void CalculatorApplication::staticKeyEventCallback(KeyEventType type, uint8_t key, uint8_t* combo, uint8_t count) {
    if (_currentInstance) {
        _currentInstance->onKeypadEvent(type, key, combo, count);
    }
}

void CalculatorApplication::onKeypadEvent(KeyEventType type, uint8_t key, uint8_t* combo, uint8_t count) {
    LOG_D("APP", "按键事件回调: type=%d, key=%d, count=%d", static_cast<int>(type), key, count);
    
    // 创建TaskKeyEvent事件
    TaskKeyEvent event;
    event.key = key;
    event.timestamp = millis();
    event.comboCount = (count > 5) ? 5 : count;  // 限制组合键数量不超过5个
    
    // 转换KeyEventType到TaskKeyEventType
    switch (type) {
        case KEY_EVENT_PRESS:
            event.type = TaskKeyEventType::PRESS;
            break;
        case KEY_EVENT_RELEASE:
            event.type = TaskKeyEventType::RELEASE;
            break;
        case KEY_EVENT_LONGPRESS:
            event.type = TaskKeyEventType::LONGPRESS;
            break;
        case KEY_EVENT_REPEAT:
            event.type = TaskKeyEventType::REPEAT;
            break;
        case KEY_EVENT_COMBO:
            event.type = TaskKeyEventType::COMBO;
            // 复制组合键数据
            if (combo && count > 0) {
                for (uint8_t i = 0; i < event.comboCount; i++) {
                    event.combo[i] = combo[i];
                }
            }
            break;
        default:
            LOG_W("APP", "未知的按键事件类型: %d", static_cast<int>(type));
            return;
    }
    
    // 将事件添加到队列中
    lockKeyEvents();
    
    // 检查队列是否已满
    if (keyEventQueue.size() >= MAX_KEY_EVENTS) {
        LOG_W("APP", "按键事件队列已满，丢弃旧事件");
        keyEventQueue.pop();  // 移除最旧的事件
    }
    
    // 添加新事件到队列
    keyEventQueue.push(event);
    
    unlockKeyEvents();
    
    LOG_D("APP", "按键事件已添加到队列，队列大小: %zu", keyEventQueue.size());
}

void CalculatorApplication::updateDisplay() {
    uint32_t currentTime = millis();
    
    if (displayNeedsUpdate || (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL)) {
        lockDisplay();
        
        try {
            // 根据当前状态更新显示
            switch (currentState) {
                case ApplicationState::CALCULATOR_MODE:
                    updateCalculatorDisplay();
                    break;
                    
                case ApplicationState::FONT_TEST_MODE:
                    updateFontTestDisplay();
                    break;
                    
                case ApplicationState::SETTINGS_MODE:
                    updateSettingsDisplay();
                    break;
                    
                case ApplicationState::ERROR_MODE:
                    updateErrorDisplay();
                    break;
                    
                default:
                    break;
            }
            
            displayNeedsUpdate = false;
            lastDisplayUpdate = currentTime;
            displayUpdateCount++;
            
        } catch (const std::exception& e) {
            handleApplicationError(String("显示更新错误: ") + e.what());
        }
        
        unlockDisplay();
    }
}

void CalculatorApplication::updateCalculatorDisplay() {
    if (calculatorCore) {
        calculatorCore->update();
    }
}

void CalculatorApplication::updateFontTestDisplay() {
    if (fontTester) {
        // 字体测试器有自己的更新逻辑
        // 这里可以添加额外的更新逻辑
    }
}

void CalculatorApplication::updateSettingsDisplay() {
    // TODO: 实现设置界面的显示更新
    if (lvglDisplay) {
        // 显示设置界面
        lvglDisplay->tick();
    }
}

void CalculatorApplication::updateErrorDisplay() {
    // TODO: 实现错误界面的显示更新
    if (lvglDisplay) {
        // 显示错误信息
        lvglDisplay->tick();
    }
}

void CalculatorApplication::updateLEDEffects() {
    uint32_t currentTime = millis();
    
    if (currentTime - lastLEDUpdate >= LED_UPDATE_INTERVAL) {
        lockLEDs();
        
        try {
            // 更新所有活动的LED效果
            for (auto it = activeLEDEffects.begin(); it != activeLEDEffects.end();) {
                updateSingleLED(*it);
                
                // 移除已完成的效果
                if (!it->isActive) {
                    it = activeLEDEffects.erase(it);
                } else {
                    ++it;
                }
            }
            
            // 更新FastLED
            FastLED.show();
            
            lastLEDUpdate = currentTime;
            ledUpdateCount++;
            
        } catch (const std::exception& e) {
            handleApplicationError(String("LED更新错误: ") + e.what());
        }
        
        unlockLEDs();
    }
}

void CalculatorApplication::updateSingleLED(LEDEffect& effect) {
    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - effect.startTime;
    
    if (elapsed >= effect.duration) {
        // 效果完成
        effect.isActive = false;
        setLEDColor(effect.ledIndex, 0);  // 关闭LED
        return;
    }
    
    // 计算当前颜色
    uint32_t currentColor;
    calculateLEDColor(effect, currentColor);
    
    // 设置LED颜色
    setLEDColor(effect.ledIndex, currentColor);
}

void CalculatorApplication::calculateLEDColor(const LEDEffect& effect, uint32_t& color) {
    uint32_t elapsed = millis() - effect.startTime;
    float progress = static_cast<float>(elapsed) / effect.duration;
    
    if (progress >= 1.0f) {
        color = 0;  // 效果结束，关闭LED
        return;
    }
    
    // 应用渐变效果
    float brightness = 1.0f;
    if (effect.fadeTime > 0) {
        float fadeProgress = static_cast<float>(elapsed) / effect.fadeTime;
        if (fadeProgress < 1.0f) {
            // 渐入阶段
            brightness = ApplicationUtils::easeOut(fadeProgress);
        } else {
            // 渐出阶段
            float fadeOutProgress = static_cast<float>(elapsed - effect.fadeTime) / (effect.duration - effect.fadeTime);
            brightness = 1.0f - ApplicationUtils::easeIn(fadeOutProgress);
        }
    }
    
    color = ApplicationUtils::adjustBrightness(effect.color, brightness);
}

void CalculatorApplication::setLEDColor(uint8_t ledIndex, uint32_t color) {
    if (ledIndex < NUM_LEDS) {
        leds[ledIndex] = CRGB(color);
    }
}

void CalculatorApplication::triggerLEDEffect(uint8_t ledIndex, uint32_t color, uint32_t duration, uint32_t fadeTime) {
    if (ledIndex >= NUM_LEDS) return;
    
    lockLEDs();
    
    // 检查是否已有相同LED的效果，如果有则替换
    for (auto& effect : activeLEDEffects) {
        if (effect.ledIndex == ledIndex) {
            effect.color = color;
            effect.duration = duration;
            effect.fadeTime = fadeTime;
            effect.startTime = millis();
            effect.isActive = true;
            unlockLEDs();
            return;
        }
    }
    
    // 添加新效果（如果空间足够）
    if (activeLEDEffects.size() < MAX_LED_EFFECTS) {
        LEDEffect effect = {
            .ledIndex = ledIndex,
            .color = color,
            .duration = duration,
            .fadeTime = fadeTime,
            .isActive = true,
            .startTime = millis()
        };
        activeLEDEffects.push_back(effect);
    }
    
    unlockLEDs();
}

void CalculatorApplication::updateBuzzer() {
    lockBuzzer();
    
    if (currentBuzzerEffect.isActive) {
        updateBuzzerEffect();
    }
    
    buzzerEventCount++;
    unlockBuzzer();
}

void CalculatorApplication::updateBuzzerEffect() {
    uint32_t elapsed = millis() - currentBuzzerEffect.startTime;
    
    if (elapsed >= currentBuzzerEffect.duration) {
        // 蜂鸣器效果完成
        currentBuzzerEffect.isActive = false;
        stopBuzzerTone();
    }
}

void CalculatorApplication::triggerBuzzer(uint16_t frequency, uint32_t duration) {
    lockBuzzer();
    
    currentBuzzerEffect.frequency = frequency;
    currentBuzzerEffect.duration = duration;
    currentBuzzerEffect.startTime = millis();
    currentBuzzerEffect.isActive = true;
    
    startBuzzerTone(frequency);
    
    unlockBuzzer();
}

void CalculatorApplication::startBuzzerTone(uint16_t frequency) {
    if (keypadControl) {
        keypadControl->startBuzzer(frequency, currentBuzzerEffect.duration);
    }
}

void CalculatorApplication::stopBuzzerTone() {
    if (keypadControl) {
        keypadControl->stopBuzzer();  // 直接调用停止蜂鸣器方法
    }
}

void CalculatorApplication::handleNumberInput(uint8_t digit) {
    LOG_D("APP", "数字输入: %d", digit);
    
    if (calculatorCore) {
        calculatorCore->handleKeyInput(digit, false);
    }
}

void CalculatorApplication::handleOperatorInput(char op) {
    LOG_D("APP", "运算符输入: %c", op);
    
    // 将运算符转换为对应的按键码
    uint8_t keyCode = 0;
    switch (op) {
        case '+': keyCode = 11; break;
        case '-': keyCode = 12; break;
        case '*': keyCode = 13; break;
        case '/': keyCode = 14; break;
    }
    
    if (calculatorCore && keyCode > 0) {
        calculatorCore->handleKeyInput(keyCode, false);
    }
}

void CalculatorApplication::executeCalculation() {
    LOG_D("APP", "执行计算");
    
    if (calculatorCore) {
        calculatorCore->handleKeyInput(15, false);  // 等号键
    }
}

void CalculatorApplication::clearCalculator() {
    LOG_D("APP", "清除计算器");
    
    if (calculatorCore) {
        calculatorCore->clearAll();
    }
}

void CalculatorApplication::handleApplicationError(const String& error) {
    errorCount++;
    
    if (errorLog.size() >= MAX_ERROR_LOG_SIZE) {
        errorLog.erase(errorLog.begin());
    }
    
    String errorEntry = String(millis()) + ": " + error;
    errorLog.push_back(errorEntry);
    
    LOG_E("APP", "应用错误: %s", error.c_str());
}

void CalculatorApplication::cleanupCurrentMode() {
    // 根据当前模式进行清理
    switch (currentState) {
        case ApplicationState::FONT_TEST_MODE:
            // 清理字体测试模式
            break;
            
        case ApplicationState::SETTINGS_MODE:
            // 清理设置模式
            break;
            
        default:
            break;
    }
}

void CalculatorApplication::initializeNewMode() {
    // 根据新模式进行初始化
    switch (currentState) {
        case ApplicationState::CALCULATOR_MODE:
            if (calculatorCore) {
                calculatorCore->initLVGLUI();
            }
            break;
            
        case ApplicationState::FONT_TEST_MODE:
            if (fontTester) {
                fontTester->showFontTest();
            }
            break;
            
        case ApplicationState::SETTINGS_MODE:
            // 初始化设置界面
            break;
            
        case ApplicationState::ERROR_MODE:
            // 初始化错误界面
            break;
            
        default:
            break;
    }
}

// 线程安全方法实现
void CalculatorApplication::lockKeyEvents() {
    if (keyEventMutex) {
        xSemaphoreTake(keyEventMutex, portMAX_DELAY);
    }
}

void CalculatorApplication::unlockKeyEvents() {
    if (keyEventMutex) {
        xSemaphoreGive(keyEventMutex);
    }
}

void CalculatorApplication::lockDisplay() {
    if (displayMutex) {
        xSemaphoreTake(displayMutex, portMAX_DELAY);
    }
}

void CalculatorApplication::unlockDisplay() {
    if (displayMutex) {
        xSemaphoreGive(displayMutex);
    }
}

void CalculatorApplication::lockLEDs() {
    if (ledMutex) {
        xSemaphoreTake(ledMutex, portMAX_DELAY);
    }
}

void CalculatorApplication::unlockLEDs() {
    if (ledMutex) {
        xSemaphoreGive(ledMutex);
    }
}

void CalculatorApplication::lockBuzzer() {
    if (buzzerMutex) {
        xSemaphoreTake(buzzerMutex, portMAX_DELAY);
    }
}

void CalculatorApplication::unlockBuzzer() {
    if (buzzerMutex) {
        xSemaphoreGive(buzzerMutex);
    }
}

void CalculatorApplication::shutdown() {
    LOG_I("APP", "计算器应用关闭");
    
    setState(ApplicationState::ERROR_MODE);
    
    // 清理所有效果
    clearAllLEDs();
    stopBuzzer();
    
    // 清理组件
    fontTester.reset();
    simpleHID.reset();
    calculatorCore.reset();
    keypadControl.reset();
    calculationEngine.reset();
    
    // 清理容器
    activeLEDEffects.clear();
    keyMappings.clear();
    errorLog.clear();
    
    // 清理队列
    while (!keyEventQueue.empty()) {
        keyEventQueue.pop();
    }
    while (!displayUpdateQueue.empty()) {
        displayUpdateQueue.pop();
    }
}

void CalculatorApplication::clearAllLEDs() {
    lockLEDs();
    activeLEDEffects.clear();
    FastLED.clear();
    FastLED.show();
    unlockLEDs();
}

void CalculatorApplication::stopBuzzer() {
    lockBuzzer();
    currentBuzzerEffect.isActive = false;
    stopBuzzerTone();
    unlockBuzzer();
}

// ApplicationUtils命名空间实现
namespace ApplicationUtils {
    String getKeyName(uint8_t key) {
        // 简单的按键名称映射
        if (key >= 1 && key <= 9) return String(key);
        if (key == 10) return "0";
        if (key == 11) return "+";
        if (key == 12) return "-";
        if (key == 13) return "*";
        if (key == 14) return "/";
        if (key == 15) return "=";
        if (key == 16) return "C";
        if (key == 17) return ".";
        return "?";
    }
    
    uint8_t getKeyFromName(const String& name) {
        if (name.length() == 1) {
            char c = name.charAt(0);
            if (c >= '1' && c <= '9') return c - '0';
            if (c == '0') return 10;
            if (c == '+') return 11;
            if (c == '-') return 12;
            if (c == '*') return 13;
            if (c == '/') return 14;
            if (c == '=') return 15;
            if (c == 'C' || c == 'c') return 16;
            if (c == '.') return 17;
        }
        return 0;
    }
    
    uint32_t blendColors(uint32_t color1, uint32_t color2, float ratio) {
        uint8_t r1 = (color1 >> 16) & 0xFF;
        uint8_t g1 = (color1 >> 8) & 0xFF;
        uint8_t b1 = color1 & 0xFF;
        
        uint8_t r2 = (color2 >> 16) & 0xFF;
        uint8_t g2 = (color2 >> 8) & 0xFF;
        uint8_t b2 = color2 & 0xFF;
        
        uint8_t r = r1 + (r2 - r1) * ratio;
        uint8_t g = g1 + (g2 - g1) * ratio;
        uint8_t b = b1 + (b2 - b1) * ratio;
        
        return (r << 16) | (g << 8) | b;
    }
    
    uint32_t adjustBrightness(uint32_t color, float brightness) {
        if (brightness < 0) brightness = 0;
        if (brightness > 1) brightness = 1;
        
        uint8_t r = ((color >> 16) & 0xFF) * brightness;
        uint8_t g = ((color >> 8) & 0xFF) * brightness;
        uint8_t b = (color & 0xFF) * brightness;
        
        return (r << 16) | (g << 8) | b;
    }
    
    uint32_t getElapsedTime(uint32_t startTime) {
        return millis() - startTime;
    }
    
    float getProgressRatio(uint32_t startTime, uint32_t duration) {
        uint32_t elapsed = getElapsedTime(startTime);
        if (duration == 0) return 1.0f;
        return static_cast<float>(elapsed) / duration;
    }
    
    float easeInOut(float t) {
        if (t < 0.5f) {
            return 2 * t * t;
        } else {
            return -1 + (4 - 2 * t) * t;
        }
    }
    
    float easeIn(float t) {
        return t * t;
    }
    
    float easeOut(float t) {
        return t * (2 - t);
    }
}

void CalculatorApplication::handleSpecialInput(const String& special) {
    LOG_D("APP", "特殊输入: %s", special.c_str());
    
    if (calculatorCore) {
        if (special == ".") {
            // 处理小数点输入，调用handleKeyInput或相应方法
            calculatorCore->handleKeyInput(21); // 假设21是小数点键
        } else if (special == "=") {
            // 处理等号，触发计算
            calculatorCore->handleKeyInput(22); // 假设22是等号键
        } else if (special == "C") {
            calculatorCore->clearAll();
        } else if (special == "CE") {
            calculatorCore->clearEntry();
        } else if (special == "+/-") {
            // 处理正负号切换
            calculatorCore->handleKeyInput(20); // 假设20是正负号键
        }
        // 更新显示
        calculatorCore->updateDisplay();
    }
}

void CalculatorApplication::handleDisplayUpdate(const DisplayUpdate& update) {
    LOG_D("APP", "显示更新: 类型=%d", static_cast<int>(update.type));
    
    if (!lvglDisplay) return;
    
    switch (update.type) {
        case DisplayUpdate::EXPRESSION_UPDATE:
            // 更新表达式显示（具体实现取决于LVGLDisplay的接口）
            LOG_D("APP", "更新表达式: %s", update.data.c_str());
            break;
        case DisplayUpdate::RESULT_UPDATE:
            // 更新结果显示
            LOG_D("APP", "更新结果: %s", update.data.c_str());
            break;
        case DisplayUpdate::HISTORY_UPDATE:
            // 更新历史记录显示
            LOG_D("APP", "更新历史: %s", update.data.c_str());
            break;
        case DisplayUpdate::FULL_REFRESH:
            // 全屏刷新
            LOG_D("APP", "全屏刷新");
            break;
    }
    
    if (update.forceUpdate) {
        LOG_D("APP", "强制重绘");
        // 强制重绘（具体实现取决于LVGLDisplay的接口）
    }
}