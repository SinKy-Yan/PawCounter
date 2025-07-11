/**
 * @file KeyboardCustomizer.h
 * @brief 键盘自定义配置器 - 扩展键盘配置功能
 * @details 基于现有KeyboardConfig系统，提供高级键盘自定义功能
 * 
 * 核心功能：
 * - 自定义按键功能映射
 * - 多层键盘布局管理
 * - 按键宏录制和播放
 * - 组合键和快捷键支持
 * - 按键LED效果自定义
 * - 键盘布局导入导出
 * - 实时键盘预览
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef KEYBOARD_CUSTOMIZER_H
#define KEYBOARD_CUSTOMIZER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <functional>
#include <queue>
#include "KeyboardConfig.h"
#include "Logger.h"

/**
 * @brief 自定义按键动作类型
 */
enum class CustomKeyAction {
    NONE = 0,           ///< 无动作
    NUMBER_INPUT,       ///< 数字输入
    OPERATOR,           ///< 运算符
    FUNCTION_CALL,      ///< 函数调用
    MACRO_PLAY,         ///< 播放宏
    LAYER_SWITCH,       ///< 层级切换
    LAYER_TOGGLE,       ///< 层级切换（切换式）
    COMBO_TRIGGER,      ///< 触发组合键
    SYSTEM_COMMAND,     ///< 系统命令
    USER_SCRIPT,        ///< 用户脚本
    LED_EFFECT,         ///< LED效果
    AUDIO_PLAY,         ///< 播放音频
    CUSTOM_CALLBACK     ///< 自定义回调
};

/**
 * @brief 按键宏指令类型
 */
enum class MacroCommand {
    KEY_PRESS,          ///< 按键按下
    KEY_RELEASE,        ///< 按键释放
    KEY_CLICK,          ///< 按键点击（按下+释放）
    DELAY,              ///< 延迟
    LOOP_START,         ///< 循环开始
    LOOP_END,           ///< 循环结束
    CONDITION,          ///< 条件判断
    VARIABLE_SET,       ///< 设置变量
    VARIABLE_GET,       ///< 获取变量
    FUNCTION_CALL       ///< 调用函数
};

/**
 * @brief 宏指令结构
 */
struct MacroInstruction {
    MacroCommand command;       ///< 指令类型
    uint16_t parameter1;        ///< 参数1
    uint16_t parameter2;        ///< 参数2
    String stringParam;         ///< 字符串参数
    uint32_t timestamp;         ///< 时间戳（录制时记录）
};

/**
 * @brief 按键宏定义
 */
struct KeyMacro {
    String macroId;                             ///< 宏ID
    String name;                                ///< 宏名称
    String description;                         ///< 宏描述
    std::vector<MacroInstruction> instructions; ///< 指令序列
    bool isRecording = false;                   ///< 是否正在录制
    bool isLooping = false;                     ///< 是否循环播放
    uint32_t createdTime;                       ///< 创建时间
    uint32_t modifiedTime;                      ///< 修改时间
    uint16_t playCount = 0;                     ///< 播放次数
};

/**
 * @brief 自定义按键定义
 */
struct CustomKeyDefinition {
    uint8_t position;                   ///< 物理按键位置
    KeyLayer layer;                     ///< 键盘层级
    CustomKeyAction primaryAction;      ///< 主要动作
    CustomKeyAction secondaryAction;    ///< 次要动作（长按）
    CustomKeyAction tertiaryAction;     ///< 第三动作（双击）
    
    // 动作参数
    String primaryParam;                ///< 主要动作参数
    String secondaryParam;              ///< 次要动作参数
    String tertiaryParam;               ///< 第三动作参数
    
    // 视觉反馈
    uint32_t primaryColor = 0xFFFFFF;   ///< 主要颜色
    uint32_t secondaryColor = 0xFF0000; ///< 次要颜色
    String ledEffect = "fade";          ///< LED效果
    
    // 音频反馈
    uint16_t primaryTone = 2000;        ///< 主要音调
    uint16_t secondaryTone = 1500;      ///< 次要音调
    uint16_t toneDuration = 50;         ///< 音调持续时间
    
    // 行为设置
    bool isEnabled = true;              ///< 是否启用
    bool requiresConfirm = false;       ///< 是否需要确认
    uint16_t longPressThreshold = 800;  ///< 长按阈值
    uint16_t doubleClickThreshold = 300; ///< 双击阈值
    
    // 回调函数
    std::function<void()> onPrimaryAction;   ///< 主要动作回调
    std::function<void()> onSecondaryAction; ///< 次要动作回调
    std::function<void()> onTertiaryAction;  ///< 第三动作回调
};

/**
 * @brief 自定义键盘布局
 */
struct CustomKeyboardLayout {
    String layoutId;                                    ///< 布局ID
    String name;                                        ///< 布局名称
    String description;                                 ///< 布局描述
    String version;                                     ///< 版本号
    uint32_t createdTime;                               ///< 创建时间
    uint32_t modifiedTime;                              ///< 修改时间
    String baseLayoutId;                                ///< 基础布局ID
    
    std::map<KeyLayer, std::vector<CustomKeyDefinition>> layerKeys; ///< 各层按键定义
    std::map<String, KeyMacro> macros;                  ///< 宏定义
    
    // 布局设置
    bool isActive = false;                              ///< 是否为活动布局
    bool isReadOnly = false;                            ///< 是否只读
    bool isSystemLayout = false;                        ///< 是否为系统布局
    uint8_t priority = 100;                             ///< 优先级
    
    // 元数据
    String author;                                      ///< 作者
    String category;                                    ///< 类别
    std::vector<String> tags;                           ///< 标签
    uint32_t downloadCount = 0;                         ///< 下载次数
    float rating = 0.0f;                                ///< 评分
};

/**
 * @brief 组合键定义
 */
struct ComboKeyDefinition {
    String comboId;                     ///< 组合键ID
    String name;                        ///< 组合键名称
    std::vector<uint8_t> keySequence;   ///< 按键序列
    uint16_t timeoutMs = 1000;          ///< 超时时间
    bool requiresOrder = true;          ///< 是否要求顺序
    CustomKeyAction action;             ///< 触发动作
    String actionParam;                 ///< 动作参数
    bool isEnabled = true;              ///< 是否启用
};

/**
 * @brief 键盘自定义配置器类
 */
class KeyboardCustomizer {
public:
    /**
     * @brief 构造函数
     */
    KeyboardCustomizer();
    
    /**
     * @brief 析构函数
     */
    ~KeyboardCustomizer();
    
    /**
     * @brief 初始化键盘自定义器
     * @return true 成功，false 失败
     */
    bool initialize();
    
    /**
     * @brief 关闭键盘自定义器
     */
    void shutdown();
    
    // =================== 自定义布局管理 ===================
    
    /**
     * @brief 创建自定义键盘布局
     * @param name 布局名称
     * @param description 布局描述
     * @param baseLayoutId 基础布局ID，空字符串表示创建全新布局
     * @return 布局ID，失败返回空字符串
     */
    String createCustomLayout(const String& name, const String& description = "", const String& baseLayoutId = "");
    
    /**
     * @brief 删除自定义布局
     * @param layoutId 布局ID
     * @return true 成功，false 失败
     */
    bool deleteCustomLayout(const String& layoutId);
    
    /**
     * @brief 复制布局
     * @param sourceLayoutId 源布局ID
     * @param newName 新布局名称
     * @return 新布局ID，失败返回空字符串
     */
    String cloneLayout(const String& sourceLayoutId, const String& newName);
    
    /**
     * @brief 应用自定义布局
     * @param layoutId 布局ID
     * @return true 成功，false 失败
     */
    bool applyCustomLayout(const String& layoutId);
    
    /**
     * @brief 获取所有自定义布局
     * @return 布局列表
     */
    std::vector<CustomKeyboardLayout> getAllCustomLayouts() const;
    
    /**
     * @brief 获取当前活动布局
     * @return 布局指针，未找到返回nullptr
     */
    const CustomKeyboardLayout* getCurrentLayout() const;
    
    // =================== 按键自定义 ===================
    
    /**
     * @brief 设置自定义按键
     * @param layoutId 布局ID
     * @param keyDefinition 按键定义
     * @return true 成功，false 失败
     */
    bool setCustomKey(const String& layoutId, const CustomKeyDefinition& keyDefinition);
    
    /**
     * @brief 获取自定义按键定义
     * @param layoutId 布局ID
     * @param position 按键位置
     * @param layer 键盘层级
     * @return 按键定义指针，未找到返回nullptr
     */
    const CustomKeyDefinition* getCustomKey(const String& layoutId, uint8_t position, KeyLayer layer) const;
    
    /**
     * @brief 重置按键为默认配置
     * @param layoutId 布局ID
     * @param position 按键位置
     * @param layer 键盘层级
     * @return true 成功，false 失败
     */
    bool resetKey(const String& layoutId, uint8_t position, KeyLayer layer);
    
    /**
     * @brief 交换两个按键的功能
     * @param layoutId 布局ID
     * @param pos1 按键位置1
     * @param pos2 按键位置2
     * @param layer 键盘层级
     * @return true 成功，false 失败
     */
    bool swapKeys(const String& layoutId, uint8_t pos1, uint8_t pos2, KeyLayer layer);
    
    // =================== 宏管理 ===================
    
    /**
     * @brief 创建按键宏
     * @param layoutId 布局ID
     * @param name 宏名称
     * @param description 宏描述
     * @return 宏ID，失败返回空字符串
     */
    String createMacro(const String& layoutId, const String& name, const String& description = "");
    
    /**
     * @brief 开始录制宏
     * @param layoutId 布局ID
     * @param macroId 宏ID
     * @return true 成功，false 失败
     */
    bool startMacroRecording(const String& layoutId, const String& macroId);
    
    /**
     * @brief 停止录制宏
     * @param layoutId 布局ID
     * @param macroId 宏ID
     * @return true 成功，false 失败
     */
    bool stopMacroRecording(const String& layoutId, const String& macroId);
    
    /**
     * @brief 播放宏
     * @param layoutId 布局ID
     * @param macroId 宏ID
     * @param loop 是否循环播放
     * @return true 成功，false 失败
     */
    bool playMacro(const String& layoutId, const String& macroId, bool loop = false);
    
    /**
     * @brief 停止播放宏
     * @param macroId 宏ID
     * @return true 成功，false 失败
     */
    bool stopMacro(const String& macroId);
    
    /**
     * @brief 删除宏
     * @param layoutId 布局ID
     * @param macroId 宏ID
     * @return true 成功，false 失败
     */
    bool deleteMacro(const String& layoutId, const String& macroId);
    
    /**
     * @brief 编辑宏指令
     * @param layoutId 布局ID
     * @param macroId 宏ID
     * @param instructions 指令序列
     * @return true 成功，false 失败
     */
    bool editMacro(const String& layoutId, const String& macroId, const std::vector<MacroInstruction>& instructions);
    
    // =================== 组合键管理 ===================
    
    /**
     * @brief 添加组合键
     * @param layoutId 布局ID
     * @param comboDefinition 组合键定义
     * @return true 成功，false 失败
     */
    bool addComboKey(const String& layoutId, const ComboKeyDefinition& comboDefinition);
    
    /**
     * @brief 删除组合键
     * @param layoutId 布局ID
     * @param comboId 组合键ID
     * @return true 成功，false 失败
     */
    bool removeComboKey(const String& layoutId, const String& comboId);
    
    /**
     * @brief 检测组合键输入
     * @param keySequence 按键序列
     * @return 匹配的组合键ID，未匹配返回空字符串
     */
    String detectComboKey(const std::vector<uint8_t>& keySequence) const;
    
    // =================== 实时功能 ===================
    
    /**
     * @brief 处理按键事件（集成到主按键处理流程）
     * @param position 按键位置
     * @param layer 当前层级
     * @param eventType 事件类型（按下、释放、长按等）
     * @return true 已处理，false 未处理（继续默认处理）
     */
    bool handleKeyEvent(uint8_t position, KeyLayer layer, const String& eventType);
    
    /**
     * @brief 更新宏播放状态
     */
    void updateMacroPlayback();
    
    /**
     * @brief 更新组合键检测
     * @param currentPressedKeys 当前按下的按键列表
     */
    void updateComboDetection(const std::vector<uint8_t>& currentPressedKeys);
    
    // =================== 布局导入导出 ===================
    
    /**
     * @brief 导出布局为JSON
     * @param layoutId 布局ID
     * @return JSON字符串，失败返回空字符串
     */
    String exportLayoutToJSON(const String& layoutId) const;
    
    /**
     * @brief 从JSON导入布局
     * @param jsonData JSON数据
     * @param errors 错误信息输出
     * @return 导入的布局ID，失败返回空字符串
     */
    String importLayoutFromJSON(const String& jsonData, std::vector<String>& errors);
    
    /**
     * @brief 导出布局到文件
     * @param layoutId 布局ID
     * @param filename 文件名
     * @return true 成功，false 失败
     */
    bool exportLayoutToFile(const String& layoutId, const String& filename) const;
    
    /**
     * @brief 从文件导入布局
     * @param filename 文件名
     * @param errors 错误信息输出
     * @return 导入的布局ID，失败返回空字符串
     */
    String importLayoutFromFile(const String& filename, std::vector<String>& errors);
    
    // =================== 布局验证 ===================
    
    /**
     * @brief 验证布局完整性
     * @param layoutId 布局ID
     * @param errors 错误信息输出
     * @return true 验证通过，false 验证失败
     */
    bool validateLayout(const String& layoutId, std::vector<String>& errors) const;
    
    /**
     * @brief 检查按键冲突
     * @param layoutId 布局ID
     * @param conflicts 冲突信息输出
     * @return true 无冲突，false 有冲突
     */
    bool checkKeyConflicts(const String& layoutId, std::vector<String>& conflicts) const;
    
    // =================== 预设模板 ===================
    
    /**
     * @brief 获取内置布局模板
     * @return 模板列表
     */
    std::vector<String> getBuiltinTemplates() const;
    
    /**
     * @brief 从模板创建布局
     * @param templateName 模板名称
     * @param layoutName 新布局名称
     * @return 布局ID，失败返回空字符串
     */
    String createFromTemplate(const String& templateName, const String& layoutName);
    
    // =================== 状态查询 ===================
    
    /**
     * @brief 检查是否已初始化
     * @return true 已初始化，false 未初始化
     */
    bool isInitialized() const { return _initialized; }
    
    /**
     * @brief 检查是否正在录制宏
     * @return true 正在录制，false 未录制
     */
    bool isRecordingMacro() const;
    
    /**
     * @brief 检查是否正在播放宏
     * @return true 正在播放，false 未播放
     */
    bool isPlayingMacro() const;
    
    /**
     * @brief 获取统计信息
     * @return 统计信息映射
     */
    std::map<String, uint32_t> getStatistics() const;
    
    /**
     * @brief 保存所有自定义配置
     * @return true 成功，false 失败
     */
    bool saveAllConfigurations();
    
    /**
     * @brief 重新加载配置
     * @return true 成功，false 失败
     */
    bool reloadConfigurations();

private:
    bool _initialized;
    
    // 核心数据
    std::map<String, CustomKeyboardLayout> _customLayouts;
    String _currentLayoutId;
    std::map<String, ComboKeyDefinition> _comboKeys;
    
    // 宏播放状态
    std::map<String, KeyMacro*> _playingMacros;
    std::queue<MacroInstruction> _macroPlaybackQueue;
    uint32_t _lastMacroUpdate;
    
    // 组合键检测
    std::vector<uint8_t> _currentKeySequence;
    uint32_t _lastKeyTime;
    
    // 录制状态
    String _recordingLayoutId;
    String _recordingMacroId;
    uint32_t _recordingStartTime;
    
    // 存储
    Preferences _preferences;
    static const char* PREF_NAMESPACE;
    static const char* PREF_LAYOUTS_KEY;
    static const char* PREF_CURRENT_LAYOUT_KEY;
    
    // 内部方法
    void loadCustomLayouts();
    void saveCustomLayouts();
    void createBuiltinTemplates();
    
    CustomKeyboardLayout createDefaultLayout() const;
    CustomKeyboardLayout createCalculatorLayout() const;
    CustomKeyboardLayout createScientificLayout() const;
    CustomKeyboardLayout createProgrammerLayout() const;
    
    bool executeKeyAction(const CustomKeyAction& action, const String& param);
    void recordMacroInstruction(const MacroInstruction& instruction);
    
    String generateUniqueId() const;
    bool isValidLayoutId(const String& layoutId) const;
    bool isValidMacroId(const String& macroId) const;
    
    // JSON序列化辅助
    void serializeCustomKey(const CustomKeyDefinition& key, JsonDocument& doc) const;
    bool deserializeCustomKey(const JsonDocument& doc, CustomKeyDefinition& key) const;
    void serializeMacro(const KeyMacro& macro, JsonDocument& doc) const;
    bool deserializeMacro(const JsonDocument& doc, KeyMacro& macro) const;
    
    // 常量定义
    static constexpr size_t MAX_CUSTOM_LAYOUTS = 20;
    static constexpr size_t MAX_MACROS_PER_LAYOUT = 50;
    static constexpr size_t MAX_MACRO_INSTRUCTIONS = 1000;
    static constexpr size_t MAX_COMBO_KEYS = 100;
    static constexpr uint32_t COMBO_TIMEOUT_MS = 1000;
    static constexpr uint32_t MACRO_INSTRUCTION_INTERVAL = 10;
};

// 全局实例访问
extern KeyboardCustomizer keyboardCustomizer;

// 工具函数
namespace KeyCustomizerUtils {
    String actionToString(CustomKeyAction action);
    CustomKeyAction stringToAction(const String& str);
    String commandToString(MacroCommand command);
    MacroCommand stringToCommand(const String& str);
    
    // 预定义动作
    CustomKeyDefinition createNumberKey(uint8_t number, uint32_t color = 0xFFFFFF);
    CustomKeyDefinition createOperatorKey(const String& op, uint32_t color = 0x00FF00);
    CustomKeyDefinition createFunctionKey(const String& func, uint32_t color = 0xFF0000);
    CustomKeyDefinition createMacroKey(const String& macroId, uint32_t color = 0x0000FF);
}

#endif // KEYBOARD_CUSTOMIZER_H