#include "SystemController.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "SleepManager.h"
#include "BackLightControl.h"
#include "config.h"
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 全局系统控制器实例
static SystemController* g_systemController = nullptr;

SystemController* getSystemController() {
    return g_systemController;
}

SystemController::SystemController() :
    currentState(SystemState::INITIALIZING),
    lastStateChangeTime(0),
    systemMutex(nullptr),
    configDirty(false),
    lastConfigSave(0),
    lastMonitorTime(0),
    errorCount(0) {
    
    // 设置全局实例
    g_systemController = this;
    
    // 初始化监控数据
    memset(&monitorData, 0, sizeof(monitorData));
    monitorData.state = currentState;
    
    // 创建线程安全互斥锁
    systemMutex = xSemaphoreCreateMutex();
    
    // 预留错误日志空间
    errorLog.reserve(MAX_ERROR_LOG_SIZE);
    
    // 预留命令空间
    serialCommands.reserve(30);
}

SystemController::~SystemController() {
    shutdown();
    
    if (systemMutex) {
        vSemaphoreDelete(systemMutex);
        systemMutex = nullptr;
    }
    
    // 清除全局实例
    if (g_systemController == this) {
        g_systemController = nullptr;
    }
}

bool SystemController::initialize() {
    LOG_I("SYS", "系统控制器初始化");
    
    // enterCriticalSection();
    
    try {
        // 初始化组件
        initializeComponents();
        
        // 设置默认命令
        setupDefaultCommands();
        
        // 加载系统配置
        loadSystemConfig();
        
        // 设置状态
        setState(SystemState::RUNNING);
        
        // exitCriticalSection();
        
        LOG_I("SYS", "系统控制器初始化完成");
        return true;
        
    } catch (const std::exception& e) {
        // exitCriticalSection();
        LOG_E("SYS", "系统控制器初始化失败: %s", e.what());
        setState(SystemState::ERROR);
        return false;
    }
}

void SystemController::initializeComponents() {
    // 获取配置管理器引用
    configManager = &ConfigManager::getInstance();
    if (!configManager->begin()) {
        throw std::runtime_error("配置管理器初始化失败");
    }
    
    // 获取日志系统引用
    logger = &Logger::getInstance();
    LoggerConfig logConfig = Logger::getDefaultConfig();
    logConfig.level = static_cast<log_level_t>(configManager->getLogLevel());
    logger->begin(logConfig);
    
    // 获取休眠管理器引用
    sleepManager = &SleepManager::instance();
    uint32_t sleepTimeout = configManager->getSleepTimeout();
    sleepManager->begin(sleepTimeout);
    
    // 注册休眠回调
    sleepManager->addCallback(
        [](void* context) { static_cast<SystemController*>(context)->onEnterSleep(); },
        [](void* context) { static_cast<SystemController*>(context)->onExitSleep(); },
        this
    );
    
    LOG_I("SYS", "系统组件初始化完成");
}

void SystemController::setupDefaultCommands() {
    // 注册默认命令
    registerCommand("help", "显示帮助信息", 
                   [this](const String& args) { handleHelpCommand(args); });
    
    registerCommand("status", "显示系统状态", 
                   [this](const String& args) { handleStatusCommand(args); });
    
    registerCommand("mem", "显示内存使用情况", 
                   [this](const String& args) { handleMemoryCommand(args); });
    
    registerCommand("tasks", "显示任务列表", 
                   [this](const String& args) { handleTasksCommand(args); });
    
    registerCommand("config", "配置管理", 
                   [this](const String& args) { handleConfigCommand(args); });
    
    registerCommand("reboot", "重启系统", 
                   [this](const String& args) { handleRebootCommand(args); });
    
    registerCommand("log_level", "设置日志级别", 
                   [this](const String& args) { handleLogLevelCommand(args); });
    
    registerCommand("sleep", "休眠管理", 
                   [this](const String& args) { handleSleepCommand(args); });
    
    registerCommand("brightness", "设置LED亮度", 
                   [this](const String& args) { handleBrightnessCommand(args); });
    
    registerCommand("led", "LED控制", 
                   [this](const String& args) { handleLEDCommand(args); });
    
    registerCommand("buzzer", "蜂鸣器测试", 
                   [this](const String& args) { handleBuzzerCommand(args); });
    
    registerCommand("piano_mode", "钢琴模式", 
                   [this](const String& args) { handlePianoCommand(args); });
    
    registerCommand("test", "测试功能", 
                   [this](const String& args) { handleTestCommand(args); });
    
    registerCommand("hid", "HID功能", 
                   [this](const String& args) { handleHIDCommand(args); });
    
    registerCommand("font", "字体测试", 
                   [this](const String& args) { handleFontCommand(args); });
    
    // 添加重置串口命令
    registerCommand("reset_serial", "重置串口缓冲区", 
                   [this](const String& args) { handleResetSerialCommand(args); });
    
    LOG_I("SYS", "默认命令注册完成，共 %zu 个命令", serialCommands.size());
}

void SystemController::setState(SystemState state) {
    if (currentState != state) {
        LOG_I("SYS", "系统状态变更: %d -> %d", static_cast<int>(currentState), static_cast<int>(state));
        
        currentState = state;
        lastStateChangeTime = millis();
        monitorData.state = state;
        
        // 执行状态变更回调
        if (state == SystemState::SLEEPING) {
            executeSystemCallbacks(true);
        } else if (currentState == SystemState::SLEEPING && state == SystemState::RUNNING) {
            executeSystemCallbacks(false);
        }
    }
}

void SystemController::registerCommand(const String& cmd, const String& desc, 
                                     std::function<void(const String&)> handler) {
    SerialCommand command = {cmd, desc, handler};
    serialCommands.push_back(command);
}

void SystemController::processSerialCommands() {
    if (!Serial.available()) return;
    
    // 读取命令并进行改进的缓冲区管理
    uint32_t startTime = millis();
    
    // 给串口一点时间接收完整的命令
    delay(10);
    
    // 先清空旧的缓冲区
    if (commandBuffer.length() > 100) {  // 如果缓冲区异常大，可能存在数据堆积
        LOG_W("SYS", "命令缓冲区异常长度: %d，已重置", commandBuffer.length());
        commandBuffer.clear();  // 重置异常的缓冲区
    }
    
    // 读取所有可用字符
    bool hasNewLine = false;
    while (Serial.available()) {
        char c = Serial.read();
        
        // 检测到换行符时处理命令
        if (c == '\n' || c == '\r') {
            hasNewLine = true;
            // 不将换行符添加到缓冲区
        } else {
            commandBuffer += c;
        }
        
        // 防止超时
        if (millis() - startTime > COMMAND_TIMEOUT) {
            LOG_W("SYS", "命令读取超时");
            break;
        }
    }
    
    // 只有接收到完整的命令行（有换行符）时才处理命令
    if (hasNewLine && !commandBuffer.isEmpty()) {
        LOG_D("SYS", "处理命令: '%s'", commandBuffer.c_str());
        handleCommand(commandBuffer);
        commandBuffer.clear();  // 处理完成后清空缓冲区
    }
}

void SystemController::handleCommand(const String& command) {
    // 去除前后空白
    String cmd = command;
    cmd.trim();
    
    if (cmd.isEmpty()) return;
    
    // 检查命令是否包含非打印字符，这可能表示数据损坏
    bool hasNonPrintable = false;
    for (unsigned int i = 0; i < cmd.length(); i++) {
        if (cmd[i] < 32 || cmd[i] > 126) {
            hasNonPrintable = true;
            break;
        }
    }
    
    if (hasNonPrintable) {
        LOG_W("SYS", "命令包含非法字符，已忽略");
        return;
    }
    
    // 分割命令和参数
    int spaceIndex = cmd.indexOf(' ');
    String mainCmd = (spaceIndex > 0) ? cmd.substring(0, spaceIndex) : cmd;
    String args = (spaceIndex > 0) ? cmd.substring(spaceIndex + 1) : "";
    
    // 转换命令为小写，但参数保持原样
    mainCmd.toLowerCase();
    
    LOG_D("SYS", "解析命令: '%s'，参数: '%s'", mainCmd.c_str(), args.c_str());
    
    // 查找并执行命令
    bool found = false;
    for (const auto& serialCmd : serialCommands) {
        if (serialCmd.command.equalsIgnoreCase(mainCmd)) {
            try {
                serialCmd.handler(args);
                found = true;
                break;
            } catch (const std::exception& e) {
                Serial.printf("命令执行错误: %s\n", e.what());
                LOG_E("SYS", "命令执行错误: %s", e.what());
            }
        }
    }
    
    if (!found) {
        Serial.printf("未知命令: '%s'，输入 'help' 查看帮助\n", mainCmd.c_str());
        LOG_W("SYS", "未知命令: '%s'", mainCmd.c_str());
    }
}

void SystemController::handleHelpCommand(const String& args) {
    Serial.println("=== 系统命令帮助 ===");
    for (const auto& cmd : serialCommands) {
        Serial.printf("  %-12s - %s\n", cmd.command.c_str(), cmd.description.c_str());
    }
    Serial.println("==================");
}

void SystemController::handleStatusCommand(const String& args) {
    updateSystemMonitorData();
    printSystemStatus();
}

void SystemController::handleMemoryCommand(const String& args) {
    printMemoryInfo();
}

void SystemController::handleTasksCommand(const String& args) {
    printTaskStatus();
}

void SystemController::handleConfigCommand(const String& args) {
    if (args.isEmpty()) {
        configManager->printConfig();
    } else if (args.startsWith("save")) {
        saveSystemConfig();
        Serial.println("配置已保存");
    } else if (args.startsWith("load")) {
        loadSystemConfig();
        Serial.println("配置已重新加载");
    } else if (args.startsWith("reset")) {
        configManager->reset();
        Serial.println("配置已重置为默认值");
    } else {
        Serial.println("用法: config [save|load|reset]");
    }
}

void SystemController::handleRebootCommand(const String& args) {
    Serial.println("系统重启中...");
    delay(1000);
    ESP.restart();
}

void SystemController::handleLogLevelCommand(const String& args) {
    if (args.isEmpty()) {
        Serial.printf("当前日志级别: %d\n", static_cast<int>(logger->getLevel()));
        return;
    }
    
    int level = args.toInt();
    if (level >= LOG_LEVEL_NONE && level <= LOG_LEVEL_VERBOSE) {
        logger->setLevel(static_cast<log_level_t>(level));
        configManager->setLogLevel(level);
        Serial.printf("日志级别已设置为: %d\n", level);
    } else {
        Serial.println("无效的日志级别 (0-5)");
    }
}

void SystemController::handleSleepCommand(const String& args) {
    if (args.isEmpty()) {
        uint32_t timeout = sleepManager->getTimeout();
        if (timeout > 0) {
            Serial.printf("休眠超时: %lu 秒\n", timeout / 1000);
        } else {
            Serial.println("休眠功能已禁用");
        }
        return;
    }
    
    if (args.equalsIgnoreCase("off")) {
        sleepManager->setTimeout(0);
        configManager->setSleepTimeout(0);
        Serial.println("休眠功能已禁用");
    } else {
        int seconds = args.toInt();
        if (seconds > 0) {
            uint32_t timeout = seconds * 1000;
            sleepManager->setTimeout(timeout);
            configManager->setSleepTimeout(timeout);
            Serial.printf("休眠超时设置为: %d 秒\n", seconds);
        } else {
            Serial.println("无效的休眠时间");
        }
    }
    
    sleepManager->feed();  // 重置休眠计时器
}

void SystemController::handleBrightnessCommand(const String& args) {
    if (args.isEmpty()) {
        Serial.printf("当前LED亮度: %d\n", configManager->getLEDBrightness());
        return;
    }
    
    int brightness = args.toInt();
    if (brightness >= 0 && brightness <= 255) {
        configManager->setLEDBrightness(brightness);
        Serial.printf("LED亮度已设置为: %d\n", brightness);
    } else {
        Serial.println("亮度值必须在 0-255 之间");
    }
}

void SystemController::handleLEDCommand(const String& args) {
    // LED控制命令的具体实现
    Serial.println("LED控制功能");
    // TODO: 实现LED控制逻辑
}

void SystemController::handleBuzzerCommand(const String& args) {
    // 蜂鸣器测试命令的具体实现
    Serial.println("蜂鸣器测试功能");
    // TODO: 实现蜂鸣器测试逻辑
}

void SystemController::handlePianoCommand(const String& args) {
    // 钢琴模式命令的具体实现
    Serial.println("钢琴模式功能");
    // TODO: 实现钢琴模式逻辑
}

void SystemController::handleTestCommand(const String& args) {
    // 测试功能命令的具体实现
    Serial.println("测试功能");
    // TODO: 实现测试逻辑
}

void SystemController::handleHIDCommand(const String& args) {
    // HID功能命令的具体实现
    Serial.println("HID功能");
    // TODO: 实现HID功能逻辑
}

void SystemController::handleFontCommand(const String& args) {
    // 字体测试命令的具体实现
    Serial.println("字体测试功能");
    // TODO: 实现字体测试逻辑
}

void SystemController::updateSleepManager() {
    if (sleepManager) {
        sleepManager->update();
    }
}

void SystemController::performSystemMonitoring() {
    uint32_t currentTime = millis();
    
    if (currentTime - lastMonitorTime >= MONITOR_INTERVAL) {
        lastMonitorTime = currentTime;
        
        updateSystemMonitorData();
        checkSystemHealth();
        
        LOG_D("SYS", "系统监控 - 堆内存: %lu, CPU: %lu MHz, 运行时间: %lu 秒",
              monitorData.freeHeap, monitorData.cpuFreq, monitorData.uptime);
    }
}

void SystemController::performMemoryCleanup() {
    cleanupMemory();
    
    // 清理错误日志
    if (errorLog.size() > MAX_ERROR_LOG_SIZE) {
        errorLog.erase(errorLog.begin(), errorLog.begin() + (errorLog.size() - MAX_ERROR_LOG_SIZE));
    }
    
    LOG_D("SYS", "内存清理完成");
}

void SystemController::saveConfigIfDirty() {
    if (configDirty) {
        uint32_t currentTime = millis();
        if (currentTime - lastConfigSave >= CONFIG_SAVE_INTERVAL) {
            saveSystemConfig();
            configDirty = false;
            lastConfigSave = currentTime;
        }
    }
}

bool SystemController::getConfigValue(const String& key, String& value) const {
    if (!configManager) return false;
    
    // enterCriticalSection();
    // TODO: 实现配置值获取
    // exitCriticalSection();
    
    return true;
}

bool SystemController::setConfigValue(const String& key, const String& value) {
    if (!configManager) return false;
    
    // enterCriticalSection();
    // TODO: 实现配置值设置
    configDirty = true;
    // exitCriticalSection();
    
    return true;
}

void SystemController::updateSystemMonitorData() {
    monitorData.freeHeap = ESP.getFreeHeap();
    monitorData.minFreeHeap = ESP.getMinFreeHeap();
    monitorData.maxAllocHeap = ESP.getMaxAllocHeap();
    monitorData.freeSketchSpace = ESP.getFreeSketchSpace();
    monitorData.cpuFreq = ESP.getCpuFreqMHz();
    monitorData.uptime = millis() / 1000;
    monitorData.taskCount = uxTaskGetNumberOfTasks();
    monitorData.state = currentState;
}

void SystemController::printSystemStatus() {
    Serial.println("\n=== 系统状态 ===");
    Serial.printf("运行时间: %lu 秒\n", monitorData.uptime);
    Serial.printf("CPU频率: %lu MHz\n", monitorData.cpuFreq);
    Serial.printf("系统状态: %d\n", static_cast<int>(monitorData.state));
    Serial.printf("任务数量: %lu\n", monitorData.taskCount);
    Serial.printf("错误计数: %lu\n", errorCount);
    
    if (sleepManager) {
        uint32_t sleepTimeout = sleepManager->getTimeout();
        if (sleepTimeout > 0) {
            Serial.printf("休眠超时: %lu 秒\n", sleepTimeout / 1000);
        } else {
            Serial.println("休眠功能: 已禁用");
        }
    }
    
    Serial.println("================\n");
}

void SystemController::printMemoryInfo() {
    Serial.println("\n=== 内存信息 ===");
    Serial.printf("堆内存总大小: %lu 字节\n", ESP.getHeapSize());
    Serial.printf("可用堆内存: %lu 字节\n", monitorData.freeHeap);
    Serial.printf("最小剩余堆: %lu 字节\n", monitorData.minFreeHeap);
    Serial.printf("最大分配块: %lu 字节\n", monitorData.maxAllocHeap);
    Serial.printf("可用程序空间: %lu 字节\n", monitorData.freeSketchSpace);
    
    if (ESP.getPsramSize() > 0) {
        Serial.printf("PSRAM总大小: %lu 字节\n", ESP.getPsramSize());
        Serial.printf("PSRAM可用: %lu 字节\n", ESP.getFreePsram());
    }
    
    Serial.println("================\n");
}

void SystemController::printTaskStatus() {
    Serial.println("\n=== 任务状态 ===");
    Serial.printf("任务总数: %lu\n", monitorData.taskCount);
    
    // 显示基本系统信息（不使用高级FreeRTOS监控功能）
    Serial.println("基本系统信息:");
    Serial.printf("空闲堆内存: %u 字节\n", ESP.getFreeHeap());
    Serial.printf("最小空闲堆内存: %u 字节\n", ESP.getMinFreeHeap());
    Serial.printf("CPU频率: %u MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("运行时间: %u 秒\n", millis() / 1000);
    Serial.println("详细任务监控功能已禁用（需要configUSE_TRACE_FACILITY=1）");
    
    /*
#if (configUSE_TRACE_FACILITY == 1)
    // 获取任务列表
    TaskStatus_t* taskStatusArray;
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    
    taskStatusArray = static_cast<TaskStatus_t*>(pvPortMalloc(taskCount * sizeof(TaskStatus_t)));
    
    if (taskStatusArray != nullptr) {
        taskCount = uxTaskGetSystemState(taskStatusArray, taskCount, nullptr);
        
        Serial.println("任务名称\t状态\t优先级\t堆栈剩余");
        Serial.println("------------------------------------");
        
        for (UBaseType_t i = 0; i < taskCount; i++) {
            const char* taskState;
            switch (taskStatusArray[i].eCurrentState) {
                case eRunning:   taskState = "运行"; break;
                case eReady:     taskState = "就绪"; break;
                case eBlocked:   taskState = "阻塞"; break;
                case eSuspended: taskState = "挂起"; break;
                case eDeleted:   taskState = "删除"; break;
                default:         taskState = "未知"; break;
            }
            
            Serial.printf("%-12s\t%s\t%u\t%u\n",
                          taskStatusArray[i].pcTaskName,
                          taskState,
                          taskStatusArray[i].uxCurrentPriority,
                          taskStatusArray[i].usStackHighWaterMark);
        }
        
        vPortFree(taskStatusArray);
    }
#else
    Serial.println("任务监控功能需要启用 configUSE_TRACE_FACILITY");
#endif
    */
    
    Serial.println("================\n");
}

void SystemController::checkSystemHealth() {
    // 检查内存使用
    if (monitorData.freeHeap < MIN_FREE_HEAP) {
        String error = String("内存不足: ") + String(monitorData.freeHeap) + " bytes";
        handleSystemError(error);
    }
    
    // 检查系统状态
    if (currentState == SystemState::ERROR) {
        LOG_W("SYS", "系统处于错误状态");
    }
}

void SystemController::cleanupMemory() {
    // 执行内存清理
    heap_caps_check_integrity_all(true);
    
    // TODO: 实现更多内存清理逻辑
    
    LOG_D("SYS", "内存清理完成");
}

void SystemController::handleSystemError(const String& error) {
    errorCount++;
    
    if (errorLog.size() >= MAX_ERROR_LOG_SIZE) {
        errorLog.erase(errorLog.begin());
    }
    
    String errorEntry = String(millis()) + ": " + error;
    errorLog.push_back(errorEntry);
    
    LOG_E("SYS", "系统错误: %s", error.c_str());
    
    // 根据错误类型决定是否需要改变系统状态
    if (error.indexOf("内存不足") >= 0) {
        setState(SystemState::ERROR);
    }
}

void SystemController::loadSystemConfig() {
    if (configManager) {
        configManager->load();
        LOG_I("SYS", "系统配置已加载");
    }
}

void SystemController::saveSystemConfig() {
    if (configManager) {
        configManager->save();
        LOG_I("SYS", "系统配置已保存");
    }
}

void SystemController::onEnterSleep() {
    LOG_I("SYS", "进入休眠模式");
    
    // 降低CPU频率
    setCpuFrequencyMhz(80);
    
    // 降低背光亮度
    BacklightControl::getInstance().setBacklight(10, 800);
    
    setState(SystemState::SLEEPING);
}

void SystemController::onExitSleep() {
    LOG_I("SYS", "退出休眠模式");
    
    // 恢复CPU频率
    setCpuFrequencyMhz(240);
    
    // 恢复背光亮度
    BacklightControl::getInstance().setBacklight(100, 500);
    
    setState(SystemState::RUNNING);
}

void SystemController::executeSystemCallbacks(bool entering) {
    for (const auto& callbacks : systemCallbacks) {
        try {
            if (entering && callbacks.first) {
                callbacks.first(nullptr);
            } else if (!entering && callbacks.second) {
                callbacks.second(nullptr);
            }
        } catch (const std::exception& e) {
            LOG_E("SYS", "回调执行错误: %s", e.what());
        }
    }
}

void SystemController::addSystemCallback(SystemCallback enterCallback, SystemCallback exitCallback) {
    systemCallbacks.push_back(std::make_pair(enterCallback, exitCallback));
}

void SystemController::enterCriticalSection() {
    if (systemMutex) {
        xSemaphoreTake(systemMutex, portMAX_DELAY);
    }
}

void SystemController::exitCriticalSection() {
    if (systemMutex) {
        xSemaphoreGive(systemMutex);
    }
}

bool SystemController::isThreadSafeOperation() const {
    return systemMutex != nullptr;
}

void SystemController::shutdown() {
    LOG_I("SYS", "系统控制器关闭");
    
    setState(SystemState::SHUTDOWN);
    
    // 清理组件（单例对象不需要手动释放）
    sleepManager = nullptr;
    logger = nullptr;
    configManager = nullptr;
    
    // 清理资源
    serialCommands.clear();
    systemCallbacks.clear();
    errorLog.clear();
}

void SystemController::handleResetSerialCommand(const String& args) {
    // 清空串口缓冲区
    while (Serial.available()) {
        Serial.read();
    }
    
    // 清空命令缓冲区
    commandBuffer.clear();
    
    Serial.println("串口缓冲区已重置");
    LOG_I("SYS", "串口缓冲区已重置");
}