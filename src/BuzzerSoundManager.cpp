/**
 * @file BuzzerSoundManager.cpp
 * @brief 蜂鸣器音效管理系统实现文件
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "BuzzerSoundManager.h"

// 预定义音效数据
static const Note SOUND_BEEP_NOTES[] = {
    Note(800, 100)
};

static const Note SOUND_DOUBLE_BEEP_NOTES[] = {
    Note(800, 100, 50),
    Note(800, 100)
};

static const Note SOUND_SUCCESS_NOTES[] = {
    Note(NOTE_C4, 150),
    Note(NOTE_E4, 150),
    Note(NOTE_G4, 300)
};

static const Note SOUND_ERROR_NOTES[] = {
    Note(400, 200, 50),
    Note(300, 200, 50),
    Note(200, 200)
};

static const Note SOUND_WARNING_NOTES[] = {
    Note(1000, 100, 100),
    Note(1000, 100, 100),
    Note(1000, 100)
};

static const Note SOUND_STARTUP_NOTES[] = {
    Note(800, 200, 100),  // "等" - 800Hz，200ms，100ms间隔
    Note(1000, 200)       // "登" - 1000Hz，200ms
};

BuzzerSoundManager& BuzzerSoundManager::getInstance() {
    static BuzzerSoundManager instance;
    return instance;
}

bool BuzzerSoundManager::begin() {
    LOG_I(TAG_SYSTEM, "Initializing Buzzer Sound Manager...");
    
    _initialized = false;
    _enabled = true;
    _keyClickEnabled = true;
    _globalVolume = 50;
    _queueSize = 0;

    // 初始化LEDC通道用于蜂鸣器控制
    ledcAttachChannel(BUZZ_PIN, 2000, 8, BUZZER_CHANNEL);  // 使用配置中定义的通道
    LOG_D(TAG_SYSTEM, "LEDC channel %d attached to BUZZ_PIN %d", BUZZER_CHANNEL, BUZZ_PIN);

    // 初始化当前效果实例
    _currentEffect = SoundEffectInstance();

    // 初始化音效队列
    for (uint8_t i = 0; i < MAX_QUEUE_SIZE; i++) {
        _soundQueue[i] = SoundQueueItem();
    }

    _initialized = true;
    LOG_I(TAG_SYSTEM, "Buzzer Sound Manager initialized successfully");
    
    return true;
}

void BuzzerSoundManager::update() {
    if (!_initialized || !_enabled) return;

    // 更新当前播放的音效
    if (_currentEffect.active) {
        updatePlayback(_currentEffect);
    }

    // 如果当前没有播放音效，检查队列
    if (!_currentEffect.active && _queueSize > 0) {
        dequeueSound();
    }
}

bool BuzzerSoundManager::playSound(const SoundEffectConfig& config, bool interrupt) {
    if (!_initialized || !_enabled) return false;

    // 检查是否可以中断当前音效
    if (_currentEffect.active && !interrupt && !canInterrupt(config.priority)) {
        // 添加到队列
        return enqueueSound(config);
    }

    // 停止当前音效并播放新音效
    if (_currentEffect.active) {
        stopPlayback(_currentEffect);
    }

    _currentEffect.config = config;
    startPlayback(_currentEffect);

    LOG_D(TAG_SYSTEM, "Playing sound: type=%d, volume=%d", config.type, config.volume);
    return true;
}

bool BuzzerSoundManager::playSound(SoundEffectType type, uint8_t volume, bool interrupt) {
    SoundEffectConfig config;
    config.type = type;
    config.volume = (volume == 255) ? _globalVolume : volume;
    config.priority = SOUND_PRIORITY_NORMAL;

    return playSound(config, interrupt);
}

void BuzzerSoundManager::stopSound() {
    if (_currentEffect.active) {
        stopPlayback(_currentEffect);
        LOG_D(TAG_SYSTEM, "Sound stopped");
    }
}

void BuzzerSoundManager::pauseSound() {
    if (_currentEffect.active && !_currentEffect.paused) {
        _currentEffect.paused = true;
        ledcWrite(BUZZ_PIN, 0); // 停止PWM输出
        LOG_D(TAG_SYSTEM, "Sound paused");
    }
}

void BuzzerSoundManager::resumeSound() {
    if (_currentEffect.active && _currentEffect.paused) {
        _currentEffect.paused = false;
        LOG_D(TAG_SYSTEM, "Sound resumed");
    }
}

void BuzzerSoundManager::clearQueue() {
    _queueSize = 0;
    LOG_D(TAG_SYSTEM, "Sound queue cleared");
}

// === 预定义音效实现 ===

void BuzzerSoundManager::keyPressSound(uint8_t volume) {
    if (!_keyClickEnabled) return;
    playSound(SOUND_CLICK, volume);
}

void BuzzerSoundManager::keyReleaseSound(uint8_t volume) {
    if (!_keyClickEnabled) return;
    playSound(SOUND_TICK, volume);
}

void BuzzerSoundManager::longPressSound(uint8_t volume) {
    if (!_keyClickEnabled) return;
    playSound(SOUND_DOUBLE_BEEP, volume);
}

void BuzzerSoundManager::errorSound(uint8_t volume) {
    playSound(SOUND_ERROR, volume, true);
}

void BuzzerSoundManager::successSound(uint8_t volume) {
    playSound(SOUND_SUCCESS, volume, true);
}

void BuzzerSoundManager::warningSound(uint8_t volume) {
    playSound(SOUND_WARNING, volume, true);
}

void BuzzerSoundManager::notificationSound(uint8_t volume) {
    playSound(SOUND_NOTIFICATION, volume);
}

void BuzzerSoundManager::startupSound() {
    playSound(SOUND_STARTUP, _globalVolume, true);
}

void BuzzerSoundManager::shutdownSound() {
    playSound(SOUND_DESCENDING_BEEP, _globalVolume, true);
}

void BuzzerSoundManager::lowBatterySound() {
    SoundEffectConfig config;
    config.type = SOUND_WARNING;
    config.volume = _globalVolume;
    config.priority = SOUND_PRIORITY_CRITICAL;
    config.repeat = true;
    config.repeatCount = 3;
    config.repeatDelay = 500;

    playSound(config, true);
}

void BuzzerSoundManager::chargeStartSound() {
    playSound(SOUND_ASCENDING_BEEP, _globalVolume);
}

void BuzzerSoundManager::chargeCompleteSound() {
    playSound(SOUND_SUCCESS, _globalVolume);
}

bool BuzzerSoundManager::playCustomSound(const Note* notes, uint8_t noteCount, 
                                        uint8_t volume, bool repeat) {
    if (!notes || noteCount == 0) return false;

    SoundEffectConfig config;
    config.type = SOUND_CUSTOM;
    config.volume = volume;
    config.priority = SOUND_PRIORITY_NORMAL;
    config.repeat = repeat;
    config.customNotes = const_cast<Note*>(notes);
    config.noteCount = noteCount;

    return playSound(config);
}

void BuzzerSoundManager::playTone(uint16_t frequency, uint16_t duration, uint8_t volume) {
    Note note(frequency, duration);
    playCustomSound(&note, 1, volume);
}

void BuzzerSoundManager::setGlobalVolume(uint8_t volume) {
    _globalVolume = constrain(volume, 0, 100);
    LOG_D(TAG_SYSTEM, "Global volume set to: %d", _globalVolume);
}

void BuzzerSoundManager::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!enabled) {
        stopSound();
        clearQueue();
    }
    LOG_I(TAG_SYSTEM, "Buzzer %s", enabled ? "enabled" : "disabled");
}

void BuzzerSoundManager::setKeyClickEnabled(bool enabled) {
    _keyClickEnabled = enabled;
    LOG_I(TAG_SYSTEM, "Key click sound %s", enabled ? "enabled" : "disabled");
}

// === 私有方法实现 ===

void BuzzerSoundManager::startPlayback(SoundEffectInstance& instance) {
    instance.startTime = millis();
    instance.nextUpdate = instance.startTime;
    instance.currentNote = 0;
    instance.currentRepeat = 0;
    instance.active = true;
    instance.playing = false;
    instance.paused = false;

    // 开始播放第一个音符
    playNextNote(instance);
}

void BuzzerSoundManager::stopPlayback(SoundEffectInstance& instance) {
    instance.active = false;
    instance.playing = false;
    instance.paused = false;
    
    // 停止PWM输出
    ledcWrite(BUZZ_PIN, 0);
}

void BuzzerSoundManager::updatePlayback(SoundEffectInstance& instance) {
    if (!instance.active || instance.paused) return;

    uint32_t currentTime = millis();

    // 检查是否到了下一个音符的时间
    if (currentTime >= instance.nextUpdate) {
        if (instance.playing) {
            // 当前音符播放结束，停止声音
            ledcWrite(BUZZ_PIN, 0);
            instance.playing = false;
            
            // 准备下一个音符
            instance.currentNote++;
        }

        // 检查是否还有音符要播放
        const Note* notes;
        uint8_t noteCount;
        if (!getPredefinedNotes(instance.config.type, notes, noteCount)) {
            // 使用自定义音符
            notes = instance.config.customNotes;
            noteCount = instance.config.noteCount;
        }

        if (instance.currentNote < noteCount) {
            // 播放下一个音符
            playNextNote(instance);
        } else {
            // 当前循环结束，检查是否需要重复
            if (instance.config.repeat && 
                (instance.config.repeatCount == 0 || instance.currentRepeat < instance.config.repeatCount)) {
                
                instance.currentNote = 0;
                instance.currentRepeat++;
                instance.nextUpdate = currentTime + instance.config.repeatDelay;
            } else {
                // 播放结束
                stopPlayback(instance);
            }
        }
    }
}

void BuzzerSoundManager::playNextNote(SoundEffectInstance& instance) {
    const Note* notes;
    uint8_t noteCount;
    
    if (!getPredefinedNotes(instance.config.type, notes, noteCount)) {
        notes = instance.config.customNotes;
        noteCount = instance.config.noteCount;
    }

    if (!notes || instance.currentNote >= noteCount) return;

    const Note& note = notes[instance.currentNote];
    
    if (note.frequency > 0) {
        // 计算PWM占空比
        uint8_t duty = volumeToPWMDuty(instance.config.volume);
        
        // 设置频率和开始播放（使用已初始化的通道）
        ledcChangeFrequency(BUZZ_PIN, note.frequency, 8);
        ledcWrite(BUZZ_PIN, duty);
        
        instance.playing = true;
        instance.nextUpdate = millis() + note.duration;
    } else {
        // 休止符，直接跳到下一个音符
        instance.playing = false;
        instance.nextUpdate = millis() + note.duration;
    }
}

bool BuzzerSoundManager::getPredefinedNotes(SoundEffectType type, const Note*& notes, uint8_t& count) {
    switch (type) {
        case SOUND_BEEP:
        case SOUND_CLICK:
        case SOUND_TICK:
            notes = SOUND_BEEP_NOTES;
            count = sizeof(SOUND_BEEP_NOTES) / sizeof(Note);
            return true;

        case SOUND_DOUBLE_BEEP:
            notes = SOUND_DOUBLE_BEEP_NOTES;
            count = sizeof(SOUND_DOUBLE_BEEP_NOTES) / sizeof(Note);
            return true;

        case SOUND_SUCCESS:
            notes = SOUND_SUCCESS_NOTES;
            count = sizeof(SOUND_SUCCESS_NOTES) / sizeof(Note);
            return true;

        case SOUND_ERROR:
            notes = SOUND_ERROR_NOTES;
            count = sizeof(SOUND_ERROR_NOTES) / sizeof(Note);
            return true;

        case SOUND_WARNING:
            notes = SOUND_WARNING_NOTES;
            count = sizeof(SOUND_WARNING_NOTES) / sizeof(Note);
            return true;

        case SOUND_STARTUP:
            notes = SOUND_STARTUP_NOTES;
            count = sizeof(SOUND_STARTUP_NOTES) / sizeof(Note);
            return true;

        default:
            return false;
    }
}

uint8_t BuzzerSoundManager::volumeToPWMDuty(uint8_t volume) {
    if (volume == 0) return 0;
    
    // 应用全局音量
    uint8_t actualVolume = (volume * _globalVolume) / 100;
    
    // 转换为PWM占空比 (0-255)
    // 使用非线性映射，因为人耳对音量的感知是非线性的
    float normalizedVolume = (float)actualVolume / 100.0;
    float pwmRatio = pow(normalizedVolume, 2.0); // 平方曲线
    
    return (uint8_t)(pwmRatio * 255);
}

bool BuzzerSoundManager::enqueueSound(const SoundEffectConfig& config) {
    if (_queueSize >= MAX_QUEUE_SIZE) return false;

    _soundQueue[_queueSize] = SoundQueueItem(config);
    _queueSize++;
    
    LOG_D(TAG_SYSTEM, "Sound queued, queue size: %d", _queueSize);
    return true;
}

bool BuzzerSoundManager::dequeueSound() {
    if (_queueSize == 0) return false;

    // 取出第一个音效
    SoundEffectConfig config = _soundQueue[0].config;
    
    // 移动队列
    for (uint8_t i = 1; i < _queueSize; i++) {
        _soundQueue[i-1] = _soundQueue[i];
    }
    _queueSize--;

    // 播放音效
    _currentEffect.config = config;
    startPlayback(_currentEffect);
    
    LOG_D(TAG_SYSTEM, "Sound dequeued, queue size: %d", _queueSize);
    return true;
}

bool BuzzerSoundManager::canInterrupt(SoundPriority newPriority) {
    if (!_currentEffect.active) return true;
    
    return newPriority >= _currentEffect.config.priority;
}