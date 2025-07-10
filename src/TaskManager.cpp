#include "TaskManager.h"
#include "SystemController.h"
#include "CalculatorApplication.h"
#include "Logger.h"
#include "config.h"
#include "BackLightControl.h"
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <memory>
#include <lvgl.h>

// 全局对象池实例
ObjectPool<TaskKeyEvent, 20> keyEventPool;
ObjectPool<DisplayUpdate, 10> displayUpdatePool;

TaskManager::TaskManager() : 
    keypadTaskHandle(nullptr),
    displayTaskHandle(nullptr),
    systemTaskHandle(nullptr),
    keyEventQueue(nullptr),
    displayUpdateQueue(nullptr),
    configMutex(nullptr),
    isInitialized(false),
    isRunning(false),
    systemStartTime(0),
    keypadTaskErrors(0),
    displayTaskErrors(0),
    systemTaskErrors(0) {
    
    // 初始化任务统计
    taskStatistics.resize(3);
    for (auto& stat : taskStatistics) {
        stat.cycleCount = 0;
        stat.lastCycleTime = 0;
        stat.maxCycleTime = 0;
        stat.avgCycleTime = 0;
        stat.stackHighWaterMark = 0;
    }
}

TaskManager::~TaskManager() {
    cleanupResources();
}

bool TaskManager::initializeHardware() {
    LOG_I("TASK", "开始硬件初始化");
    
    // 初始化看门狗
    initWatchdog();
    
    // 初始化系统控制器
    systemController.reset(new SystemController());
    if (!systemController->initialize()) {
        LOG_E("TASK", "系统控制器初始化失败");
        return false;
    }
    
    // 初始化计算器应用
    calculatorApp.reset(new CalculatorApplication());
    if (!calculatorApp->initialize()) {
        LOG_E("TASK", "计算器应用初始化失败");
        return false;
    }
    
    isInitialized = true;
    systemStartTime = xTaskGetTickCount();
    
    LOG_I("TASK", "硬件初始化完成");
    return true;
}

void TaskManager::createTasks() {
    LOG_I("TASK", "创建FreeRTOS任务");
    
    if (!isInitialized) {
        LOG_E("TASK", "系统未初始化，无法创建任务");
        return;
    }
    
    // 创建任务间通信对象
    keyEventQueue = xQueueCreate(KEY_EVENT_QUEUE_SIZE, sizeof(TaskKeyEvent));
    displayUpdateQueue = xQueueCreate(DISPLAY_UPDATE_QUEUE_SIZE, sizeof(DisplayUpdate));
    configMutex = xSemaphoreCreateMutex();
    
    if (!keyEventQueue || !displayUpdateQueue || !configMutex) {
        LOG_E("TASK", "创建任务通信对象失败");
        cleanupResources();
        return;
    }
    
    // 创建按键任务 (最高优先级)
    BaseType_t result = xTaskCreatePinnedToCore(
        keypadTask,
        "KeypadTask",
        STACK_SIZE_KEYPAD,
        this,
        TASK_PRIORITY_KEYPAD,
        &keypadTaskHandle,
        1  // 绑定到Core 1 (APP_CPU)
    );
    
    if (result != pdPASS) {
        LOG_E("TASK", "创建按键任务失败: %d", result);
        cleanupResources();
        return;
    }
    
    // 创建显示任务 (中等优先级)
    result = xTaskCreatePinnedToCore(
        displayTask,
        "DisplayTask",
        STACK_SIZE_DISPLAY,
        this,
        TASK_PRIORITY_DISPLAY,
        &displayTaskHandle,
        1  // 绑定到Core 1 (APP_CPU)
    );
    
    if (result != pdPASS) {
        LOG_E("TASK", "创建显示任务失败: %d", result);
        cleanupResources();
        return;
    }
    
    // 创建系统任务 (低优先级)
    result = xTaskCreatePinnedToCore(
        systemTask,
        "SystemTask",
        STACK_SIZE_SYSTEM,
        this,
        TASK_PRIORITY_SYSTEM,
        &systemTaskHandle,
        0  // 绑定到Core 0 (PRO_CPU)
    );
    
    if (result != pdPASS) {
        LOG_E("TASK", "创建系统任务失败: %d", result);
        cleanupResources();
        return;
    }
    
    isRunning = true;
    LOG_I("TASK", "所有任务创建完成");
}

void TaskManager::startScheduler() {
    if (!isRunning) {
        LOG_E("TASK", "任务未创建，无法启动调度器");
        return;
    }
    
    LOG_I("TASK", "FreeRTOS任务系统已启动");
    // 注意：在Arduino ESP32框架中，FreeRTOS调度器已经由框架自动启动
    // 不需要手动调用vTaskStartScheduler()，任务已经开始运行
    LOG_I("TASK", "所有任务已在后台运行");
}

// 按键任务实现
void TaskManager::keypadTask(void* parameter) {
    TaskManager* self = static_cast<TaskManager*>(parameter);
    TaskKeyEvent keyEvent;
    TickType_t lastWakeTime = xTaskGetTickCount();
    uint32_t cycleStartTime;
    
    LOG_I("KEYPAD", "按键任务启动");
    
    while (true) {
        cycleStartTime = xTaskGetTickCount();
        
        // 喂狗
        self->feedWatchdog();
        
        try {
            // 扫描按键矩阵
            self->calculatorApp->scanKeypadMatrix();
            
            // 处理按键事件
            while (self->calculatorApp->getKeyEvent(&keyEvent)) {
                // 添加时间戳
                keyEvent.timestamp = xTaskGetTickCount();
                
                // 发送到事件队列
                if (xQueueSend(self->keyEventQueue, &keyEvent, 0) != pdTRUE) {
                    LOG_W("KEYPAD", "按键事件队列满");
                }
            }
            
            // 更新LED效果
            self->calculatorApp->updateLEDEffects();
            
            // 处理蜂鸣器
            self->calculatorApp->updateBuzzer();
            
        } catch (const std::exception& e) {
            LOG_E("KEYPAD", "按键任务异常: %s", e.what());
            self->keypadTaskErrors++;
            self->handleTaskError(self->keypadTaskHandle, "KeypadTask");
        }
        
        // 更新任务统计
        uint32_t cycleTime = xTaskGetTickCount() - cycleStartTime;
        self->taskStatistics[0].cycleCount++;
        self->taskStatistics[0].lastCycleTime = cycleTime;
        if (cycleTime > self->taskStatistics[0].maxCycleTime) {
            self->taskStatistics[0].maxCycleTime = cycleTime;
        }
        self->taskStatistics[0].avgCycleTime = 
            (self->taskStatistics[0].avgCycleTime + cycleTime) / 2;
        
        // 精确定时，10ms周期
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(10));
    }
}

// 显示任务实现
void TaskManager::displayTask(void* parameter) {
    TaskManager* self = static_cast<TaskManager*>(parameter);
    TaskKeyEvent keyEvent;
    DisplayUpdate displayUpdate;
    TickType_t lastWakeTime = xTaskGetTickCount();
    uint32_t cycleStartTime;
    
    LOG_I("DISPLAY", "显示任务启动");
    
    while (true) {
        cycleStartTime = xTaskGetTickCount();
        
        // 喂狗
        self->feedWatchdog();
        
        try {
            // 处理按键事件
            while (xQueueReceive(self->keyEventQueue, &keyEvent, 0) == pdTRUE) {
                self->calculatorApp->handleKeyEvent(keyEvent);
            }
            
            // 处理显示更新
            while (xQueueReceive(self->displayUpdateQueue, &displayUpdate, 0) == pdTRUE) {
                self->calculatorApp->handleDisplayUpdate(displayUpdate);
            }
            
            // 更新显示
            self->calculatorApp->updateDisplay();
            
            // 更新背光控制（处理渐变效果）
            BacklightControl::getInstance().update();
            
            // 处理LVGL定时器（时间源由LVGLDisplay管理）
            lv_timer_handler();
            
        } catch (const std::exception& e) {
            LOG_E("DISPLAY", "显示任务异常: %s", e.what());
            self->displayTaskErrors++;
            self->handleTaskError(self->displayTaskHandle, "DisplayTask");
        }
        
        // 更新任务统计
        uint32_t cycleTime = xTaskGetTickCount() - cycleStartTime;
        self->taskStatistics[1].cycleCount++;
        self->taskStatistics[1].lastCycleTime = cycleTime;
        if (cycleTime > self->taskStatistics[1].maxCycleTime) {
            self->taskStatistics[1].maxCycleTime = cycleTime;
        }
        self->taskStatistics[1].avgCycleTime = 
            (self->taskStatistics[1].avgCycleTime + cycleTime) / 2;
        
        // 20ms周期，50fps
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(20));
    }
}

// 系统任务实现
void TaskManager::systemTask(void* parameter) {
    TaskManager* self = static_cast<TaskManager*>(parameter);
    TickType_t lastWakeTime = xTaskGetTickCount();
    uint32_t cycleCount = 0;
    uint32_t cycleStartTime;
    
    LOG_I("SYSTEM", "系统任务启动");
    
    while (true) {
        cycleStartTime = xTaskGetTickCount();
        
        // 喂狗
        self->feedWatchdog();
        
        try {
            // 处理串口命令
            self->systemController->processSerialCommands();
            
            // 更新休眠管理 (每秒执行一次)
            if (cycleCount % 1 == 0) {
                self->systemController->updateSleepManager();
            }
            
            // 配置自动保存 (每5秒检查一次)
            if (cycleCount % 5 == 0) {
                if (xSemaphoreTake(self->configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    self->systemController->saveConfigIfDirty();
                    xSemaphoreGive(self->configMutex);
                }
            }
            
            // 系统监控 (每10秒执行一次)
            if (cycleCount % 10 == 0) {
                self->systemController->performSystemMonitoring();
                self->monitorTaskPerformance();
                self->checkSystemHealth();
            }
            
            // 内存管理 (每30秒执行一次)
            if (cycleCount % 30 == 0) {
                self->systemController->performMemoryCleanup();
            }
            
            // 堆栈使用检查 (每60秒执行一次)
            if (cycleCount % 60 == 0) {
                self->checkTaskStackUsage();
            }
            
        } catch (const std::exception& e) {
            LOG_E("SYSTEM", "系统任务异常: %s", e.what());
            self->systemTaskErrors++;
            self->handleTaskError(self->systemTaskHandle, "SystemTask");
        }
        
        // 更新任务统计
        uint32_t cycleTime = xTaskGetTickCount() - cycleStartTime;
        self->taskStatistics[2].cycleCount++;
        self->taskStatistics[2].lastCycleTime = cycleTime;
        if (cycleTime > self->taskStatistics[2].maxCycleTime) {
            self->taskStatistics[2].maxCycleTime = cycleTime;
        }
        self->taskStatistics[2].avgCycleTime = 
            (self->taskStatistics[2].avgCycleTime + cycleTime) / 2;
        
        cycleCount++;
        
        // 1秒周期
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000));
    }
}

void TaskManager::monitorTaskPerformance() {
    LOG_D("TASK", "任务性能监控");
    
    // 基本性能监控（不使用高级FreeRTOS函数）
    LOG_D("TASK", "系统运行时间: %lu ms", millis());
    LOG_D("TASK", "空闲堆内存: %u 字节", ESP.getFreeHeap());
    LOG_D("TASK", "详细任务监控已禁用（需要configUSE_TRACE_FACILITY=1）");
    
    /*
#if (configUSE_TRACE_FACILITY == 1)
    // 获取任务信息
    TaskStatus_t taskStatus;
    
    // 监控按键任务
    if (keypadTaskHandle) {
        vTaskGetInfo(keypadTaskHandle, &taskStatus, pdTRUE, eInvalid);
        taskStatistics[0].stackHighWaterMark = taskStatus.usStackHighWaterMark;
        
        LOG_D("TASK", "按键任务 - 周期: %lums, 最大: %lums, 平均: %lums, 堆栈剩余: %u",
              taskStatistics[0].lastCycleTime, taskStatistics[0].maxCycleTime,
              taskStatistics[0].avgCycleTime, taskStatus.usStackHighWaterMark);
    }
    
    // 监控显示任务
    if (displayTaskHandle) {
        vTaskGetInfo(displayTaskHandle, &taskStatus, pdTRUE, eInvalid);
        taskStatistics[1].stackHighWaterMark = taskStatus.usStackHighWaterMark;
        
        LOG_D("TASK", "显示任务 - 周期: %lums, 最大: %lums, 平均: %lums, 堆栈剩余: %u",
              taskStatistics[1].lastCycleTime, taskStatistics[1].maxCycleTime,
              taskStatistics[1].avgCycleTime, taskStatus.usStackHighWaterMark);
    }
    
    // 监控系统任务
    if (systemTaskHandle) {
        vTaskGetInfo(systemTaskHandle, &taskStatus, pdTRUE, eInvalid);
        taskStatistics[2].stackHighWaterMark = taskStatus.usStackHighWaterMark;
        
        LOG_D("TASK", "系统任务 - 周期: %lums, 最大: %lums, 平均: %lums, 堆栈剩余: %u",
              taskStatistics[2].lastCycleTime, taskStatistics[2].maxCycleTime,
              taskStatistics[2].avgCycleTime, taskStatus.usStackHighWaterMark);
    }
#else
    LOG_D("TASK", "详细任务监控需要启用 configUSE_TRACE_FACILITY");
#endif
    */
}

void TaskManager::checkSystemHealth() {
    // 检查堆内存
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < MIN_FREE_HEAP_SIZE) {
        LOG_W("TASK", "堆内存不足: %lu bytes", freeHeap);
    }
    
    // 检查对象池使用情况
    size_t keyEventPoolUsed = keyEventPool.getUsedCount();
    size_t displayUpdatePoolUsed = displayUpdatePool.getUsedCount();
    
    if (keyEventPoolUsed > 15) {  // 75%使用率
        LOG_W("TASK", "按键事件池使用率高: %zu/20", keyEventPoolUsed);
    }
    
    if (displayUpdatePoolUsed > 7) {  // 70%使用率
        LOG_W("TASK", "显示更新池使用率高: %zu/10", displayUpdatePoolUsed);
    }
    
    // 检查错误计数
    if (keypadTaskErrors > 0 || displayTaskErrors > 0 || systemTaskErrors > 0) {
        LOG_W("TASK", "任务错误计数 - 按键: %lu, 显示: %lu, 系统: %lu",
              keypadTaskErrors, displayTaskErrors, systemTaskErrors);
    }
}

void TaskManager::checkTaskStackUsage() {
    // 基本堆栈检查（不使用高级FreeRTOS函数）
    LOG_D("TASK", "基本堆栈检查");
    LOG_D("TASK", "空闲堆内存: %u 字节", ESP.getFreeHeap());
    LOG_D("TASK", "最小空闲堆内存: %u 字节", ESP.getMinFreeHeap());
    LOG_D("TASK", "详细堆栈监控已禁用（需要configUSE_TRACE_FACILITY=1）");
    
    /*
#if (configUSE_TRACE_FACILITY == 1)
    TaskStatus_t taskStatus;
    
    // 检查按键任务
    if (keypadTaskHandle) {
        vTaskGetInfo(keypadTaskHandle, &taskStatus, pdTRUE, eInvalid);
        if (taskStatus.usStackHighWaterMark < MIN_STACK_REMAINING) {
            LOG_W("TASK", "按键任务堆栈不足: %u words", taskStatus.usStackHighWaterMark);
        }
    }
    
    // 检查显示任务
    if (displayTaskHandle) {
        vTaskGetInfo(displayTaskHandle, &taskStatus, pdTRUE, eInvalid);
        if (taskStatus.usStackHighWaterMark < MIN_STACK_REMAINING) {
            LOG_W("TASK", "显示任务堆栈不足: %u words", taskStatus.usStackHighWaterMark);
        }
    }
    
    // 检查系统任务
    if (systemTaskHandle) {
        vTaskGetInfo(systemTaskHandle, &taskStatus, pdTRUE, eInvalid);
        if (taskStatus.usStackHighWaterMark < MIN_STACK_REMAINING) {
            LOG_W("TASK", "系统任务堆栈不足: %u words", taskStatus.usStackHighWaterMark);
        }
    }
#else
    LOG_D("TASK", "堆栈监控需要启用 configUSE_TRACE_FACILITY");
#endif
    */
}

void TaskManager::handleTaskError(TaskHandle_t taskHandle, const char* taskName) {
    LOG_E("TASK", "任务 %s 发生错误", taskName);
    
    // 记录错误信息
    // TODO: 实现错误恢复机制
    
    // 可选：重启任务
    // 注意：这里需要谨慎处理，避免无限重启循环
}

void TaskManager::adjustTaskPriorities() {
    // 根据系统负载动态调整任务优先级
    // 这是一个高级功能，当前保持简单实现
}

void TaskManager::initWatchdog() {
    // 简化看门狗初始化
    esp_task_wdt_init(30, true);  // 30秒超时，触发panic
    LOG_I("TASK", "任务看门狗初始化完成");
}

void TaskManager::feedWatchdog() {
    // 喂狗操作
    esp_task_wdt_reset();
}

void TaskManager::cleanupResources() {
    LOG_I("TASK", "清理任务管理器资源");
    
    // 删除任务
    if (keypadTaskHandle) {
        vTaskDelete(keypadTaskHandle);
        keypadTaskHandle = nullptr;
    }
    
    if (displayTaskHandle) {
        vTaskDelete(displayTaskHandle);
        displayTaskHandle = nullptr;
    }
    
    if (systemTaskHandle) {
        vTaskDelete(systemTaskHandle);
        systemTaskHandle = nullptr;
    }
    
    // 删除队列和信号量
    if (keyEventQueue) {
        vQueueDelete(keyEventQueue);
        keyEventQueue = nullptr;
    }
    
    if (displayUpdateQueue) {
        vQueueDelete(displayUpdateQueue);
        displayUpdateQueue = nullptr;
    }
    
    if (configMutex) {
        vSemaphoreDelete(configMutex);
        configMutex = nullptr;
    }
    
    // 重置状态
    isInitialized = false;
    isRunning = false;
}

// TaskHealthMonitor 实现
TaskHealthMonitor::TaskHealthMonitor() : 
    monitorTaskHandle(nullptr), 
    isMonitoring(false) {
}

TaskHealthMonitor::~TaskHealthMonitor() {
    stopMonitoring();
}

void TaskHealthMonitor::addTask(TaskHandle_t handle, const String& name, uint32_t maxInterval) {
    TaskHealth health = {
        .handle = handle,
        .lastHeartbeat = millis(),
        .maxInterval = maxInterval,
        .name = name,
        .isHealthy = true
    };
    
    monitoredTasks.push_back(health);
    LOG_I("HEALTH", "添加任务监控: %s", name.c_str());
}

void TaskHealthMonitor::heartbeat(TaskHandle_t handle) {
    uint32_t currentTime = millis();
    
    for (auto& task : monitoredTasks) {
        if (task.handle == handle) {
            task.lastHeartbeat = currentTime;
            task.isHealthy = true;
            break;
        }
    }
}

void TaskHealthMonitor::startMonitoring() {
    if (isMonitoring) return;
    
    isMonitoring = true;
    xTaskCreate(monitorTask, "HealthMonitor", 2048, this, 1, &monitorTaskHandle);
    LOG_I("HEALTH", "启动任务健康监控");
}

void TaskHealthMonitor::stopMonitoring() {
    if (!isMonitoring) return;
    
    isMonitoring = false;
    if (monitorTaskHandle) {
        vTaskDelete(monitorTaskHandle);
        monitorTaskHandle = nullptr;
    }
    LOG_I("HEALTH", "停止任务健康监控");
}

void TaskHealthMonitor::monitorTask(void* parameter) {
    TaskHealthMonitor* self = static_cast<TaskHealthMonitor*>(parameter);
    
    while (self->isMonitoring) {
        self->checkTaskHealth();
        vTaskDelay(pdMS_TO_TICKS(5000));  // 5秒检查一次
    }
}

void TaskHealthMonitor::checkTaskHealth() {
    uint32_t currentTime = millis();
    
    for (auto& task : monitoredTasks) {
        if (currentTime - task.lastHeartbeat > task.maxInterval) {
            if (task.isHealthy) {
                LOG_E("HEALTH", "任务 %s 无响应", task.name.c_str());
                task.isHealthy = false;
                // 可选：重启任务
                // restartTask(task.handle, task.name);
            }
        }
    }
}

void TaskHealthMonitor::restartTask(TaskHandle_t handle, const String& name) {
    LOG_W("HEALTH", "重启任务: %s", name.c_str());
    // 实现任务重启逻辑
    // 注意：这需要谨慎处理，避免系统不稳定
}