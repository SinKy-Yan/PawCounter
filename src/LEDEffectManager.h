/**
 * @file LEDEffectManager.h
 * @brief RGB LED灯效管理系统
 * @details 提供完整的LED灯效控制，包括预定义灯效、自定义效果和动画管理
 * 
 * 功能特性：
 * - 多种预定义灯效（闪烁、呼吸、渐变、彩虹等）
 * - 可配置的颜色、亮度、速度参数
 * - 分层管理：单个LED、按键组、全局效果
 * - 优先级系统处理效果冲突
 * - 非阻塞动画更新
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef LED_EFFECT_MANAGER_H
#define LED_EFFECT_MANAGER_H

#include <Arduino.h>
#include <FastLED.h>
#include "config.h"
#include "Logger.h"

/**
 * @brief LED灯效类型枚举
 */
enum LEDEffectType {
    LED_EFFECT_OFF = 0,         ///< 关闭
    LED_EFFECT_SOLID,           ///< 纯色
    LED_EFFECT_BLINK,           ///< 闪烁
    LED_EFFECT_FADE_IN,         ///< 淡入
    LED_EFFECT_FADE_OUT,        ///< 淡出
    LED_EFFECT_BREATH,          ///< 呼吸
    LED_EFFECT_PULSE,           ///< 脉冲
    LED_EFFECT_RAINBOW,         ///< 彩虹
    LED_EFFECT_RAINBOW_CYCLE,   ///< 彩虹循环
    LED_EFFECT_COLOR_WIPE,      ///< 颜色擦除
    LED_EFFECT_THEATER_CHASE,   ///< 剧院追逐
    LED_EFFECT_FIRE,            ///< 火焰效果
    LED_EFFECT_TWINKLE,         ///< 闪烁星星
    LED_EFFECT_GRADIENT,        ///< 渐变色
    LED_EFFECT_WAVE,            ///< 波浪
    LED_EFFECT_RIPPLE           ///< 水波纹
};

/**
 * @brief LED优先级枚举
 */
enum LEDPriority {
    LED_PRIORITY_LOW = 0,       ///< 低优先级（背景效果）
    LED_PRIORITY_NORMAL = 1,    ///< 普通优先级（按键反馈）
    LED_PRIORITY_HIGH = 2,      ///< 高优先级（系统状态）
    LED_PRIORITY_CRITICAL = 3   ///< 关键优先级（错误警告）
};

/**
 * @brief LED效果配置结构体
 */
struct LEDEffectConfig {
    LEDEffectType type;         ///< 效果类型
    CRGB primaryColor;          ///< 主颜色
    CRGB secondaryColor;        ///< 次颜色（用于双色效果）
    uint8_t brightness;         ///< 亮度 (0-255)
    uint16_t duration;          ///< 持续时间 (ms, 0=无限)
    uint16_t speed;             ///< 动画速度 (ms间隔)
    LEDPriority priority;       ///< 优先级
    bool repeat;                ///< 是否重复
    uint8_t repeatCount;        ///< 重复次数 (0=无限)
    
    // 构造函数，提供默认值
    LEDEffectConfig() :
        type(LED_EFFECT_SOLID),
        primaryColor(CRGB::White),
        secondaryColor(CRGB::Black),
        brightness(255),
        duration(1000),
        speed(50),
        priority(LED_PRIORITY_NORMAL),
        repeat(false),
        repeatCount(1) {}
};

/**
 * @brief LED效果实例结构体
 */
struct LEDEffectInstance {
    LEDEffectConfig config;     ///< 效果配置
    uint32_t startTime;         ///< 开始时间
    uint32_t lastUpdate;        ///< 上次更新时间
    uint8_t currentRepeat;      ///< 当前重复次数
    bool active;                ///< 是否激活
    float progress;             ///< 动画进度 (0.0-1.0)
    uint8_t phase;              ///< 动画阶段（用于复杂效果）
    CRGB currentColor;          ///< 当前颜色
    
    LEDEffectInstance() : 
        startTime(0), lastUpdate(0), currentRepeat(0), 
        active(false), progress(0.0), phase(0), 
        currentColor(CRGB::Black) {}
};

/**
 * @brief LED组定义
 */
struct LEDGroup {
    uint8_t leds[NUM_LEDS];     ///< LED索引数组
    uint8_t count;              ///< LED数量
    String name;                ///< 组名
    
    LEDGroup() : count(0) {}
};

/**
 * @brief RGB LED灯效管理器类
 */
class LEDEffectManager {
public:
    /**
     * @brief 获取LED管理器单例
     * @return LEDEffectManager& 管理器实例引用
     */
    static LEDEffectManager& getInstance();

    /**
     * @brief 初始化LED管理器
     * @return true 初始化成功，false 初始化失败
     */
    bool begin();

    /**
     * @brief 更新所有LED效果（在主循环中调用）
     */
    void update();

    // === 单个LED控制 ===
    
    /**
     * @brief 设置单个LED效果
     * @param ledIndex LED索引 (0-21)
     * @param config 效果配置
     * @return true 设置成功，false 设置失败
     */
    bool setLEDEffect(uint8_t ledIndex, const LEDEffectConfig& config);

    /**
     * @brief 停止单个LED效果
     * @param ledIndex LED索引
     */
    void stopLEDEffect(uint8_t ledIndex);

    /**
     * @brief 设置单个LED颜色（即时效果）
     * @param ledIndex LED索引
     * @param color 颜色
     * @param brightness 亮度 (0-255)
     */
    void setLEDColor(uint8_t ledIndex, CRGB color, uint8_t brightness = 255);

    // === LED组控制 ===

    /**
     * @brief 创建LED组
     * @param name 组名
     * @param ledIndices LED索引数组
     * @param count LED数量
     * @return true 创建成功，false 创建失败
     */
    bool createLEDGroup(const String& name, const uint8_t* ledIndices, uint8_t count);

    /**
     * @brief 设置LED组效果
     * @param groupName 组名
     * @param config 效果配置
     * @return true 设置成功，false 设置失败
     */
    bool setGroupEffect(const String& groupName, const LEDEffectConfig& config);

    /**
     * @brief 停止LED组效果
     * @param groupName 组名
     */
    void stopGroupEffect(const String& groupName);

    // === 全局控制 ===

    /**
     * @brief 设置全局LED效果
     * @param config 效果配置
     */
    void setGlobalEffect(const LEDEffectConfig& config);

    /**
     * @brief 停止所有LED效果
     */
    void stopAllEffects();

    /**
     * @brief 设置全局亮度
     * @param brightness 亮度 (0-255)
     */
    void setGlobalBrightness(uint8_t brightness);

    // === 预定义效果 ===

    /**
     * @brief 按键按下反馈
     * @param ledIndex LED索引
     * @param color 颜色（可选，默认白色）
     */
    void keyPressEffect(uint8_t ledIndex, CRGB color = CRGB::White);

    /**
     * @brief 按键释放反馈
     * @param ledIndex LED索引
     */
    void keyReleaseEffect(uint8_t ledIndex);

    /**
     * @brief 错误提示效果
     * @param ledIndices LED索引数组（可选，默认全部LED）
     * @param count LED数量
     */
    void errorEffect(const uint8_t* ledIndices = nullptr, uint8_t count = 0);

    /**
     * @brief 成功提示效果
     * @param ledIndices LED索引数组（可选，默认全部LED）
     * @param count LED数量
     */
    void successEffect(const uint8_t* ledIndices = nullptr, uint8_t count = 0);

    /**
     * @brief 警告提示效果
     * @param ledIndices LED索引数组（可选，默认全部LED）
     * @param count LED数量
     */
    void warningEffect(const uint8_t* ledIndices = nullptr, uint8_t count = 0);

    /**
     * @brief 启动动画效果
     */
    void startupEffect();

    /**
     * @brief 关机动画效果
     */
    void shutdownEffect();

    /**
     * @brief 低电量警告效果
     */
    void lowBatteryEffect();

    /**
     * @brief 充电状态效果
     */
    void chargingEffect();

    // === 配置管理 ===

    /**
     * @brief 设置是否启用LED
     * @param enabled true启用，false禁用
     */
    void setEnabled(bool enabled);

    /**
     * @brief 获取LED启用状态
     * @return true启用，false禁用
     */
    bool isEnabled() const { return _enabled; }

    /**
     * @brief 保存当前LED状态
     */
    void saveState();

    /**
     * @brief 恢复LED状态
     */
    void restoreState();

private:
    LEDEffectManager() = default;
    ~LEDEffectManager() = default;
    LEDEffectManager(const LEDEffectManager&) = delete;
    LEDEffectManager& operator=(const LEDEffectManager&) = delete;

    bool _initialized;                              ///< 初始化状态
    bool _enabled;                                  ///< LED启用状态
    uint8_t _globalBrightness;                      ///< 全局亮度
    LEDEffectInstance _ledEffects[NUM_LEDS];        ///< 单个LED效果实例
    LEDGroup _ledGroups[8];                         ///< LED组数组（最多8个组）
    uint8_t _groupCount;                            ///< 当前组数量
    LEDEffectInstance _globalEffect;                ///< 全局效果实例
    CRGB _savedState[NUM_LEDS];                     ///< 保存的LED状态
    bool _energySaveActive;                         ///< 节能模式激活状态

    /**
     * @brief 更新单个LED效果
     * @param instance 效果实例
     * @param ledIndex LED索引
     */
    void updateSingleEffect(LEDEffectInstance& instance, uint8_t ledIndex);

    /**
     * @brief 计算效果颜色
     * @param instance 效果实例
     * @return CRGB 计算后的颜色
     */
    CRGB calculateEffectColor(const LEDEffectInstance& instance);

    /**
     * @brief 应用亮度
     * @param color 原颜色
     * @param brightness 亮度 (0-255)
     * @return CRGB 应用亮度后的颜色
     */
    CRGB applyBrightness(CRGB color, uint8_t brightness);

    /**
     * @brief 混合两个颜色
     * @param color1 颜色1
     * @param color2 颜色2
     * @param ratio 混合比例 (0.0-1.0)
     * @return CRGB 混合后的颜色
     */
    CRGB blendColors(CRGB color1, CRGB color2, float ratio);

    /**
     * @brief 获取彩虹颜色
     * @param hue 色相 (0-255)
     * @return CRGB 彩虹颜色
     */
    CRGB getRainbowColor(uint8_t hue);

    /**
     * @brief 查找LED组
     * @param name 组名
     * @return LEDGroup* 组指针，未找到返回nullptr
     */
    LEDGroup* findGroup(const String& name);

    /**
     * @brief 验证LED索引
     * @param ledIndex LED索引
     * @return true 有效，false 无效
     */
    bool isValidLEDIndex(uint8_t ledIndex) const;

    /**
     * @brief 创建默认LED组
     */
    void createDefaultGroups();

    /**
     * @brief 更新全局效果
     */
    void updateGlobalEffect();

    /**
     * @brief 应用全局效果到所有LED
     */
    void applyGlobalEffectToAllLEDs();

    /**
     * @brief 应用颜色擦除效果
     */
    void applyColorWipeEffect();

    /**
     * @brief 应用波浪效果
     */
    void applyWaveEffect();

    /**
     * @brief 应用彩虹循环效果
     */
    void applyRainbowCycleEffect();
};

// 便捷宏定义
#define LED_MGR LEDEffectManager::getInstance()

#endif // LED_EFFECT_MANAGER_H