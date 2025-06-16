/**
 * @file BuzzerSoundManager.h
 * @brief 蜂鸣器音效管理系统
 * @details 提供完整的蜂鸣器音效控制，包括音调、节拍、旋律和音效模式管理
 * 
 * 功能特性：
 * - 多种预定义音效（按键音、提示音、警告音等）
 * - 可配置的音调、音量、持续时间参数
 * - 音乐旋律播放支持
 * - 音效优先级管理
 * - 非阻塞音效播放
 * - 音效队列系统
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef BUZZER_SOUND_MANAGER_H
#define BUZZER_SOUND_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "Logger.h"

/**
 * @brief 音效类型枚举
 */
enum SoundEffectType {
    SOUND_NONE = 0,             ///< 无声
    SOUND_BEEP,                 ///< 简单蜂鸣
    SOUND_DOUBLE_BEEP,          ///< 双重蜂鸣
    SOUND_TRIPLE_BEEP,          ///< 三重蜂鸣
    SOUND_ASCENDING_BEEP,       ///< 上升音调
    SOUND_DESCENDING_BEEP,      ///< 下降音调
    SOUND_CLICK,                ///< 点击音
    SOUND_TICK,                 ///< 滴答音
    SOUND_SUCCESS,              ///< 成功音
    SOUND_ERROR,                ///< 错误音
    SOUND_WARNING,              ///< 警告音
    SOUND_NOTIFICATION,         ///< 通知音
    SOUND_STARTUP,              ///< 启动音
    SOUND_SHUTDOWN,             ///< 关机音
    SOUND_LOW_BATTERY,          ///< 低电量音
    SOUND_CHARGE_START,         ///< 开始充电音
    SOUND_CHARGE_COMPLETE,      ///< 充电完成音
    SOUND_MELODY_MARIO,         ///< 马里奥旋律
    SOUND_MELODY_TETRIS,        ///< 俄罗斯方块旋律
    SOUND_MELODY_NOKIA,         ///< 诺基亚铃声
    SOUND_CUSTOM                ///< 自定义音效
};

/**
 * @brief 音效优先级枚举
 */
enum SoundPriority {
    SOUND_PRIORITY_LOW = 0,     ///< 低优先级（背景音效）
    SOUND_PRIORITY_NORMAL = 1,  ///< 普通优先级（按键音）
    SOUND_PRIORITY_HIGH = 2,    ///< 高优先级（系统提示）
    SOUND_PRIORITY_CRITICAL = 3 ///< 关键优先级（紧急警告）
};

/**
 * @brief 音符结构体
 */
struct Note {
    uint16_t frequency;         ///< 频率 (Hz)
    uint16_t duration;          ///< 持续时间 (ms)
    uint16_t pause;             ///< 暂停时间 (ms)
    
    Note() : frequency(0), duration(0), pause(0) {}
    Note(uint16_t freq, uint16_t dur, uint16_t p = 0) : 
        frequency(freq), duration(dur), pause(p) {}
};

/**
 * @brief 音效配置结构体
 */
struct SoundEffectConfig {
    SoundEffectType type;       ///< 音效类型
    uint8_t volume;             ///< 音量 (0-100)
    SoundPriority priority;     ///< 优先级
    bool repeat;                ///< 是否重复
    uint8_t repeatCount;        ///< 重复次数 (0=无限)
    uint16_t repeatDelay;       ///< 重复间隔 (ms)
    
    // 自定义音效参数
    Note* customNotes;          ///< 自定义音符数组
    uint8_t noteCount;          ///< 音符数量
    
    // 构造函数，提供默认值
    SoundEffectConfig() :
        type(SOUND_BEEP),
        volume(50),
        priority(SOUND_PRIORITY_NORMAL),
        repeat(false),
        repeatCount(1),
        repeatDelay(100),
        customNotes(nullptr),
        noteCount(0) {}
};

/**
 * @brief 音效实例结构体
 */
struct SoundEffectInstance {
    SoundEffectConfig config;   ///< 音效配置
    uint32_t startTime;         ///< 开始时间
    uint32_t nextUpdate;        ///< 下次更新时间
    uint8_t currentNote;        ///< 当前音符索引
    uint8_t currentRepeat;      ///< 当前重复次数
    bool active;                ///< 是否激活
    bool playing;               ///< 是否正在播放
    bool paused;                ///< 是否暂停
    
    SoundEffectInstance() : 
        startTime(0), nextUpdate(0), currentNote(0), 
        currentRepeat(0), active(false), playing(false), paused(false) {}
};

/**
 * @brief 音效队列项结构体
 */
struct SoundQueueItem {
    SoundEffectConfig config;   ///< 音效配置
    uint32_t queueTime;         ///< 入队时间
    
    SoundQueueItem() : queueTime(0) {}
    SoundQueueItem(const SoundEffectConfig& cfg) : config(cfg), queueTime(millis()) {}
};

/**
 * @brief 蜂鸣器音效管理器类
 */
class BuzzerSoundManager {
public:
    /**
     * @brief 获取音效管理器单例
     * @return BuzzerSoundManager& 管理器实例引用
     */
    static BuzzerSoundManager& getInstance();

    /**
     * @brief 初始化音效管理器
     * @return true 初始化成功，false 初始化失败
     */
    bool begin();

    /**
     * @brief 更新音效播放（在主循环中调用）
     */
    void update();

    // === 音效播放控制 ===

    /**
     * @brief 播放音效
     * @param config 音效配置
     * @param interrupt 是否中断当前音效
     * @return true 播放成功，false 播放失败
     */
    bool playSound(const SoundEffectConfig& config, bool interrupt = false);

    /**
     * @brief 播放预定义音效
     * @param type 音效类型
     * @param volume 音量 (0-100, 默认使用系统音量)
     * @param interrupt 是否中断当前音效
     * @return true 播放成功，false 播放失败
     */
    bool playSound(SoundEffectType type, uint8_t volume = 255, bool interrupt = false);

    /**
     * @brief 停止当前音效
     */
    void stopSound();

    /**
     * @brief 暂停当前音效
     */
    void pauseSound();

    /**
     * @brief 恢复当前音效
     */
    void resumeSound();

    /**
     * @brief 清空音效队列
     */
    void clearQueue();

    // === 预定义音效 ===

    /**
     * @brief 按键按下音
     * @param volume 音量 (0-100)
     */
    void keyPressSound(uint8_t volume = 255);

    /**
     * @brief 按键释放音
     * @param volume 音量 (0-100)
     */
    void keyReleaseSound(uint8_t volume = 255);

    /**
     * @brief 长按音
     * @param volume 音量 (0-100)
     */
    void longPressSound(uint8_t volume = 255);

    /**
     * @brief 错误提示音
     * @param volume 音量 (0-100)
     */
    void errorSound(uint8_t volume = 255);

    /**
     * @brief 成功提示音
     * @param volume 音量 (0-100)
     */
    void successSound(uint8_t volume = 255);

    /**
     * @brief 警告音
     * @param volume 音量 (0-100)
     */
    void warningSound(uint8_t volume = 255);

    /**
     * @brief 通知音
     * @param volume 音量 (0-100)
     */
    void notificationSound(uint8_t volume = 255);

    /**
     * @brief 启动音
     */
    void startupSound();

    /**
     * @brief 关机音
     */
    void shutdownSound();

    /**
     * @brief 低电量警告音
     */
    void lowBatterySound();

    /**
     * @brief 充电开始音
     */
    void chargeStartSound();

    /**
     * @brief 充电完成音
     */
    void chargeCompleteSound();

    // === 旋律播放 ===

    /**
     * @brief 播放马里奥旋律
     * @param volume 音量 (0-100)
     */
    void playMarioMelody(uint8_t volume = 50);

    /**
     * @brief 播放俄罗斯方块旋律
     * @param volume 音量 (0-100)
     */
    void playTetrisMelody(uint8_t volume = 50);

    /**
     * @brief 播放诺基亚铃声
     * @param volume 音量 (0-100)
     */
    void playNokiaMelody(uint8_t volume = 50);

    // === 自定义音效 ===

    /**
     * @brief 播放自定义音符序列
     * @param notes 音符数组
     * @param noteCount 音符数量
     * @param volume 音量 (0-100)
     * @param repeat 是否重复
     * @return true 播放成功，false 播放失败
     */
    bool playCustomSound(const Note* notes, uint8_t noteCount, 
                        uint8_t volume = 50, bool repeat = false);

    /**
     * @brief 播放单音符
     * @param frequency 频率 (Hz)
     * @param duration 持续时间 (ms)
     * @param volume 音量 (0-100)
     */
    void playTone(uint16_t frequency, uint16_t duration, uint8_t volume = 50);

    // === 配置管理 ===

    /**
     * @brief 设置全局音量
     * @param volume 音量 (0-100)
     */
    void setGlobalVolume(uint8_t volume);

    /**
     * @brief 获取全局音量
     * @return uint8_t 音量 (0-100)
     */
    uint8_t getGlobalVolume() const { return _globalVolume; }

    /**
     * @brief 设置是否启用音效
     * @param enabled true启用，false禁用
     */
    void setEnabled(bool enabled);

    /**
     * @brief 获取音效启用状态
     * @return true启用，false禁用
     */
    bool isEnabled() const { return _enabled; }

    /**
     * @brief 设置是否启用按键音
     * @param enabled true启用，false禁用
     */
    void setKeyClickEnabled(bool enabled);

    /**
     * @brief 获取按键音启用状态
     * @return true启用，false禁用
     */
    bool isKeyClickEnabled() const { return _keyClickEnabled; }

    /**
     * @brief 获取当前播放状态
     * @return true正在播放，false未播放
     */
    bool isPlaying() const { return _currentEffect.playing; }

    /**
     * @brief 获取队列长度
     * @return uint8_t 队列中音效数量
     */
    uint8_t getQueueLength() const { return _queueSize; }

private:
    BuzzerSoundManager() = default;
    ~BuzzerSoundManager() = default;
    BuzzerSoundManager(const BuzzerSoundManager&) = delete;
    BuzzerSoundManager& operator=(const BuzzerSoundManager&) = delete;

    bool _initialized;                          ///< 初始化状态
    bool _enabled;                              ///< 音效启用状态
    bool _keyClickEnabled;                      ///< 按键音启用状态
    uint8_t _globalVolume;                      ///< 全局音量 (0-100)
    SoundEffectInstance _currentEffect;         ///< 当前播放的音效实例
    
    // 音效队列
    static const uint8_t MAX_QUEUE_SIZE = 8;    ///< 最大队列长度
    SoundQueueItem _soundQueue[MAX_QUEUE_SIZE]; ///< 音效队列
    uint8_t _queueSize;                         ///< 当前队列大小

    /**
     * @brief 开始播放音效实例
     * @param instance 音效实例
     */
    void startPlayback(SoundEffectInstance& instance);

    /**
     * @brief 停止播放音效实例
     * @param instance 音效实例
     */
    void stopPlayback(SoundEffectInstance& instance);

    /**
     * @brief 更新音效播放状态
     * @param instance 音效实例
     */
    void updatePlayback(SoundEffectInstance& instance);

    /**
     * @brief 播放下一个音符
     * @param instance 音效实例
     */
    void playNextNote(SoundEffectInstance& instance);

    /**
     * @brief 获取预定义音效的音符数据
     * @param type 音效类型
     * @param notes 输出音符数组指针
     * @param count 输出音符数量
     * @return true 获取成功，false 获取失败
     */
    bool getPredefinedNotes(SoundEffectType type, const Note*& notes, uint8_t& count);

    /**
     * @brief 计算音量对应的PWM占空比
     * @param volume 音量 (0-100)
     * @return uint8_t PWM占空比 (0-255)
     */
    uint8_t volumeToPWMDuty(uint8_t volume);

    /**
     * @brief 添加音效到队列
     * @param config 音效配置
     * @return true 添加成功，false 队列已满
     */
    bool enqueueSound(const SoundEffectConfig& config);

    /**
     * @brief 从队列取出下一个音效
     * @return true 取出成功，false 队列为空
     */
    bool dequeueSound();

    /**
     * @brief 检查是否可以中断当前音效
     * @param newPriority 新音效优先级
     * @return true 可以中断，false 不可中断
     */
    bool canInterrupt(SoundPriority newPriority);
};

// 便捷宏定义
#define BUZZER_MGR BuzzerSoundManager::getInstance()

// 常用音符频率定义 (Hz)
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

#endif // BUZZER_SOUND_MANAGER_H