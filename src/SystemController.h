#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <Arduino.h>
#include <memory>
#include <vector>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// 系统控制器相关常量
#define MAX_ERROR_LOG_ENTRIES 10
#define MAX_COMMAND_LENGTH 64
#define COMMAND_TIMEOUT 100  // 命令读取超时时间（毫秒）

// 前向声明
class ConfigManager;
class Logger;
class SleepManager;

// 系统监控回调函数类型
using SystemCallback = std::function<void(void*)>;

// 系统状态枚举
enum class SystemState {
    INITIALIZING,
    RUNNING,
    SLEEPING,
    ERROR,
    SHUTDOWN
};

// 系统监控数据结构
struct SystemMonitorData {
    uint32_t freeHeap;
    uint32_t minFreeHeap;
    uint32_t maxAllocHeap;
    uint32_t freeSketchSpace;
    uint32_t cpuFreq;
    uint32_t uptime;
    float cpuUsage;
    uint32_t taskCount;
    SystemState state;
};

// 命令处理结构
struct SerialCommand {
    String command;
    String description;
    std::function<void(const String&)> handler;
};

// 系统控制器类
class SystemController {
public:
    SystemController();
    ~SystemController();
    
    // 初始化和管理
    bool initialize();
    void shutdown();
    
    // 状态管理
    SystemState getState() const { return currentState; }
    void setState(SystemState state);
    
    // 串口命令处理
    void processSerialCommands();
    void registerCommand(const String& cmd, const String& desc, 
                        std::function<void(const String&)> handler);
    
    // 系统监控
    void updateSleepManager();
    void performSystemMonitoring();
    void performMemoryCleanup();
    
    // 配置管理
    void saveConfigIfDirty();
    bool getConfigValue(const String& key, String& value) const;
    bool setConfigValue(const String& key, const String& value);
    
    // 回调管理
    void addSystemCallback(SystemCallback enterCallback, SystemCallback exitCallback);
    
    // 错误处理
    void handleSystemError(const String& error);
    void saveCrashInfo(const String& crashInfo);
    
    // 线程安全接口
    bool isThreadSafeOperation() const;
    void enterCriticalSection();
    void exitCriticalSection();
    
private:
    // 组件管理
    ConfigManager* configManager;  // ConfigManager是单例，不用unique_ptr管理
    Logger* logger;  // Logger是单例，不用unique_ptr管理
    SleepManager* sleepManager;  // SleepManager是单例，不用unique_ptr管理
    
    // 系统状态
    SystemState currentState;
    uint32_t lastStateChangeTime;
    
    // 线程安全
    mutable SemaphoreHandle_t systemMutex;
    bool configDirty;
    uint32_t lastConfigSave;
    
    // 命令系统
    std::vector<SerialCommand> serialCommands;
    String commandBuffer;
    
    // 监控数据
    SystemMonitorData monitorData;
    uint32_t lastMonitorTime;
    
    // 回调系统
    std::vector<std::pair<SystemCallback, SystemCallback>> systemCallbacks;
    
    // 错误处理
    std::vector<String> errorLog;
    uint32_t errorCount;
    
    // 内部方法
    void initializeComponents();
    void setupDefaultCommands();
    void handleCommand(const String& command);
    void printHelp();
    void printSystemStatus();
    void printTaskStatus();
    void printMemoryInfo();
    void updateSystemMonitorData();
    void executeSystemCallbacks(bool entering);
    
    // 命令处理函数
    void handleHelpCommand(const String& args);
    void handleStatusCommand(const String& args);
    void handleMemoryCommand(const String& args);
    void handleTasksCommand(const String& args);
    void handleConfigCommand(const String& args);
    void handleRebootCommand(const String& args);
    void handleLogLevelCommand(const String& args);
    void handleSleepCommand(const String& args);
    void handleBrightnessCommand(const String& args);
    void handleLEDCommand(const String& args);
    void handleBuzzerCommand(const String& args);
    void handlePianoCommand(const String& args);
    void handleTestCommand(const String& args);
    void handleHIDCommand(const String& args);
    void handleFontCommand(const String& args);
    void handleResetSerialCommand(const String& args);  // 新增串口重置命令处理函数
    
    // 系统监控方法
    void checkSystemHealth();
    void checkMemoryUsage();
    void checkTaskPerformance();
    void cleanupMemory();
    
    // 配置管理方法
    void loadSystemConfig();
    void saveSystemConfig();
    bool validateConfig();
    
    // 休眠管理回调
    void onEnterSleep();
    void onExitSleep();
    
    // 常量定义
    static constexpr uint32_t CONFIG_SAVE_INTERVAL = 5000;  // 5秒
    static constexpr uint32_t MONITOR_INTERVAL = 10000;     // 10秒
    static constexpr uint32_t MEMORY_CLEANUP_INTERVAL = 30000;  // 30秒
    static constexpr uint32_t MIN_FREE_HEAP = 8192;         // 8KB
    static constexpr uint32_t MAX_ERROR_LOG_SIZE = 50;      // 最多50条错误日志
};

// 全局系统控制器实例访问
extern SystemController* getSystemController();

#endif // SYSTEMCONTROLLER_H