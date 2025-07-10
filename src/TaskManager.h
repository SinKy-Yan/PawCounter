#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <memory>
#include <vector>

// 前向声明
class SystemController;
class CalculatorApplication;

// 任务优先级定义
#define TASK_PRIORITY_KEYPAD     3  // 最高优先级 - 按键响应
#define TASK_PRIORITY_DISPLAY    2  // 中等优先级 - 显示更新
#define TASK_PRIORITY_SYSTEM     1  // 低优先级 - 系统管理
#define TASK_PRIORITY_IDLE       0  // 空闲任务

// 任务堆栈大小
#define STACK_SIZE_KEYPAD        4096
#define STACK_SIZE_DISPLAY       4096
#define STACK_SIZE_SYSTEM        6144

// 队列大小
#define KEY_EVENT_QUEUE_SIZE     10
#define DISPLAY_UPDATE_QUEUE_SIZE 5

// 最小资源监控阈值
#define MIN_FREE_HEAP_SIZE       8192   // 8KB
#define MIN_STACK_REMAINING      256    // 256 words

// 按键事件结构 - 与KeypadControl兼容
enum class TaskKeyEventType {
    PRESS,
    RELEASE,
    LONGPRESS,
    REPEAT,
    COMBO
};

struct TaskKeyEvent {
    TaskKeyEventType type;
    uint8_t key;
    uint8_t combo[5];
    uint8_t comboCount;
    uint32_t timestamp;
};

// 显示更新结构
struct DisplayUpdate {
    enum Type {
        EXPRESSION_UPDATE,
        RESULT_UPDATE,
        HISTORY_UPDATE,
        FULL_REFRESH
    } type;
    String data;
    bool forceUpdate;
};

// 任务管理器类
class TaskManager {
public:
    TaskManager();
    ~TaskManager();
    
    // 初始化和管理
    bool initializeHardware();
    void createTasks();
    void startScheduler();
    
    // 任务间通信接口
    QueueHandle_t getKeyEventQueue() const { return keyEventQueue; }
    QueueHandle_t getDisplayUpdateQueue() const { return displayUpdateQueue; }
    SemaphoreHandle_t getConfigMutex() const { return configMutex; }
    
    // 系统监控
    void monitorTaskPerformance();
    void checkSystemHealth();
    
    // 错误处理
    void handleTaskError(TaskHandle_t taskHandle, const char* taskName);
    
private:
    // 任务句柄
    TaskHandle_t keypadTaskHandle;
    TaskHandle_t displayTaskHandle;
    TaskHandle_t systemTaskHandle;
    
    // 任务间通信
    QueueHandle_t keyEventQueue;
    QueueHandle_t displayUpdateQueue;
    SemaphoreHandle_t configMutex;
    
    // 组件管理
    std::unique_ptr<SystemController> systemController;
    std::unique_ptr<CalculatorApplication> calculatorApp;
    
    // 任务函数（静态函数）
    static void keypadTask(void* parameter);
    static void displayTask(void* parameter);
    static void systemTask(void* parameter);
    
    // 内部方法
    void cleanupResources();
    void checkTaskStackUsage();
    void adjustTaskPriorities();
    
    // 看门狗相关
    void initWatchdog();
    void feedWatchdog();
    
    // 性能监控
    struct TaskStats {
        uint32_t cycleCount;
        uint32_t lastCycleTime;
        uint32_t maxCycleTime;
        uint32_t avgCycleTime;
        uint32_t stackHighWaterMark;
    };
    
    std::vector<TaskStats> taskStatistics;
    
    // 系统状态
    bool isInitialized;
    bool isRunning;
    uint32_t systemStartTime;
    
    // 错误计数
    uint32_t keypadTaskErrors;
    uint32_t displayTaskErrors;
    uint32_t systemTaskErrors;
};

// 任务健康监控类
class TaskHealthMonitor {
public:
    TaskHealthMonitor();
    ~TaskHealthMonitor();
    
    void addTask(TaskHandle_t handle, const String& name, uint32_t maxInterval);
    void heartbeat(TaskHandle_t handle);
    void startMonitoring();
    void stopMonitoring();
    
private:
    struct TaskHealth {
        TaskHandle_t handle;
        uint32_t lastHeartbeat;
        uint32_t maxInterval;
        String name;
        bool isHealthy;
    };
    
    std::vector<TaskHealth> monitoredTasks;
    TaskHandle_t monitorTaskHandle;
    bool isMonitoring;
    
    static void monitorTask(void* parameter);
    void checkTaskHealth();
    void restartTask(TaskHandle_t handle, const String& name);
};

// 对象池模板类 - 用于内存优化
template<typename T, size_t PoolSize>
class ObjectPool {
public:
    ObjectPool() {
        mutex = xSemaphoreCreateMutex();
        used.fill(false);
    }
    
    ~ObjectPool() {
        if (mutex) {
            vSemaphoreDelete(mutex);
        }
    }
    
    T* acquire() {
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            for (size_t i = 0; i < PoolSize; ++i) {
                if (!used[i]) {
                    used[i] = true;
                    xSemaphoreGive(mutex);
                    return &pool[i];
                }
            }
            xSemaphoreGive(mutex);
        }
        return nullptr;
    }
    
    void release(T* obj) {
        if (!obj) return;
        
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            size_t index = obj - &pool[0];
            if (index < PoolSize) {
                used[index] = false;
                // 重置对象状态
                *obj = T{};
            }
            xSemaphoreGive(mutex);
        }
    }
    
    size_t getUsedCount() const {
        size_t count = 0;
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            for (size_t i = 0; i < PoolSize; ++i) {
                if (used[i]) count++;
            }
            xSemaphoreGive(mutex);
        }
        return count;
    }
    
private:
    std::array<T, PoolSize> pool;
    std::array<bool, PoolSize> used;
    SemaphoreHandle_t mutex;
};

// 全局对象池实例
extern ObjectPool<TaskKeyEvent, 20> keyEventPool;
extern ObjectPool<DisplayUpdate, 10> displayUpdatePool;

#endif // TASKMANAGER_H