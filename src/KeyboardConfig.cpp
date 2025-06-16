/**
 * @file KeyboardConfig.cpp
 * @brief 键盘配置管理系统实现
 * 
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "KeyboardConfig.h"
#include <functional>

// 静态常量定义
const char* KeyboardConfigManager::PREF_NAMESPACE = "keyboard_cfg";
const char* KeyboardConfigManager::PREF_CONFIG_KEY = "layout_data";
const char* KeyboardConfigManager::PREF_VERSION_KEY = "version";
const char* KeyboardConfigManager::PREF_CHECKSUM_KEY = "checksum";

// 全局实例定义
KeyboardConfigManager keyboardConfig;

KeyboardConfigManager::KeyboardConfigManager()
    : _currentLayer(KeyLayer::PRIMARY)
    , _lastLayerSwitchTime(0) {
    
    // 初始化Tab键行为配置
    _tabBehavior.shortPressThreshold = 200;      // 200ms内为短按
    _tabBehavior.longPressThreshold = 1000;      // 1000ms以上为长按
    _tabBehavior.enableAutoReturn = true;        // 启用自动返回主层
    _tabBehavior.autoReturnTimeout = 30000;      // 30秒后自动返回
    _tabBehavior.onLongPress = nullptr;          // 长按回调
    _tabBehavior.onDoublePress = nullptr;        // 双击回调
}

KeyboardConfigManager::~KeyboardConfigManager() {
    // 析构函数
}

bool KeyboardConfigManager::begin() {
    KEYBOARD_LOG_I("Initializing keyboard configuration manager");
    
    // 初始化Preferences
    if (!_preferences.begin(PREF_NAMESPACE, false)) {
        KEYBOARD_LOG_E("Failed to initialize preferences");
        return false;
    }
    
    // 加载配置
    if (!loadConfig()) {
        KEYBOARD_LOG_W("Failed to load saved config, using defaults");
        resetToDefault();
    }
    
    KEYBOARD_LOG_I("Keyboard configuration manager initialized successfully");
    return true;
}

bool KeyboardConfigManager::loadConfig(bool forceDefault) {
    if (forceDefault) {
        KEYBOARD_LOG_I("Forcing default configuration");
        _layoutConfig = createDefaultConfig();
        return true;
    }
    
    // 检查是否有保存的配置
    if (!_preferences.isKey(PREF_CONFIG_KEY)) {
        KEYBOARD_LOG_I("No saved configuration found, creating default");
        _layoutConfig = createDefaultConfig();
        return saveConfig();
    }
    
    // 读取配置版本和校验和
    String savedVersion = _preferences.getString(PREF_VERSION_KEY, "");
    uint32_t savedChecksum = _preferences.getULong(PREF_CHECKSUM_KEY, 0);
    
    // 读取配置数据
    size_t configSize = _preferences.getBytesLength(PREF_CONFIG_KEY);
    if (configSize == 0) {
        KEYBOARD_LOG_E("Invalid configuration size");
        return false;
    }
    
    uint8_t* buffer = new uint8_t[configSize];
    size_t readSize = _preferences.getBytes(PREF_CONFIG_KEY, buffer, configSize);
    
    if (readSize != configSize) {
        KEYBOARD_LOG_E("Configuration read size mismatch");
        delete[] buffer;
        return false;
    }
    
    // 反序列化配置
    bool result = deserializeConfig(buffer, configSize);
    delete[] buffer;
    
    if (!result) {
        KEYBOARD_LOG_E("Failed to deserialize configuration");
        return false;
    }
    
    // 验证校验和
    uint32_t calculatedChecksum = calculateChecksum();
    if (calculatedChecksum != savedChecksum) {
        KEYBOARD_LOG_W("Configuration checksum mismatch, using defaults");
        _layoutConfig = createDefaultConfig();
        return saveConfig();
    }
    
    // 验证配置完整性
    if (!validateConfig()) {
        KEYBOARD_LOG_E("Configuration validation failed");
        _layoutConfig = createDefaultConfig();
        return saveConfig();
    }
    
    KEYBOARD_LOG_I("Configuration loaded successfully, version: %s", savedVersion.c_str());
    return true;
}

bool KeyboardConfigManager::saveConfig() {
    KEYBOARD_LOG_D("Saving keyboard configuration");
    
    // 更新校验和
    _layoutConfig.checksum = calculateChecksum();
    
    // 序列化配置
    const size_t maxBufferSize = 4096;  // 4KB应该足够
    uint8_t* buffer = new uint8_t[maxBufferSize];
    size_t configSize = serializeConfig(buffer, maxBufferSize);
    
    if (configSize == 0) {
        KEYBOARD_LOG_E("Failed to serialize configuration");
        delete[] buffer;
        return false;
    }
    
    // 保存到Flash
    bool result = true;
    result &= _preferences.putString(PREF_VERSION_KEY, _layoutConfig.version);
    result &= _preferences.putULong(PREF_CHECKSUM_KEY, _layoutConfig.checksum);
    result &= (_preferences.putBytes(PREF_CONFIG_KEY, buffer, configSize) == configSize);
    
    delete[] buffer;
    
    if (result) {
        KEYBOARD_LOG_I("Configuration saved successfully");
    } else {
        KEYBOARD_LOG_E("Failed to save configuration");
    }
    
    return result;
}

bool KeyboardConfigManager::resetToDefault() {
    KEYBOARD_LOG_I("Resetting to default configuration");
    
    _layoutConfig = createDefaultConfig();
    _currentLayer = _layoutConfig.defaultLayer;
    
    return saveConfig();
}

bool KeyboardConfigManager::switchToLayer(KeyLayer layer) {
    if (layer >= KeyLayer::MAX_LAYERS) {
        KEYBOARD_LOG_E("Invalid layer: %d", (int)layer);
        return false;
    }
    
    const LayerConfig* layerConfig = findLayerConfig(layer);
    if (!layerConfig) {
        KEYBOARD_LOG_E("Layer configuration not found: %d", (int)layer);
        return false;
    }
    
    _currentLayer = layer;
    _lastLayerSwitchTime = millis();
    
    KEYBOARD_LOG_I("Switched to layer: %s", layerConfig->name.c_str());
    return true;
}

bool KeyboardConfigManager::handleTabKey(bool isLongPress) {
    if (isLongPress) {
        KEYBOARD_LOG_D("Tab key long press detected");
        
        // 执行长按回调
        if (_tabBehavior.onLongPress) {
            _tabBehavior.onLongPress();
        }
        
        return true;
    } else {
        KEYBOARD_LOG_D("Tab key short press detected");
        
        // 短按切换层级
        KeyLayer targetLayer = (_currentLayer == KeyLayer::PRIMARY) ? 
                              KeyLayer::SECONDARY : KeyLayer::PRIMARY;
        
        return switchToLayer(targetLayer);
    }
}

const KeyConfig* KeyboardConfigManager::getKeyConfig(uint8_t position, KeyLayer layer) const {
    const LayerConfig* layerConfig = findLayerConfig(layer);
    if (!layerConfig) {
        return nullptr;
    }
    
    for (const auto& keyConfig : layerConfig->keys) {
        if (keyConfig.position == position) {
            return &keyConfig;
        }
    }
    
    return nullptr;
}

bool KeyboardConfigManager::setKeyConfig(uint8_t position, const KeyConfig& config, KeyLayer layer) {
    LayerConfig* layerConfig = findLayerConfig(layer);
    if (!layerConfig) {
        KEYBOARD_LOG_E("Layer not found: %d", (int)layer);
        return false;
    }
    
    // 查找现有按键配置
    for (auto& keyConfig : layerConfig->keys) {
        if (keyConfig.position == position) {
            keyConfig = config;
            KEYBOARD_LOG_I("Updated key config for position %d in layer %s", 
                          position, layerConfig->name.c_str());
            return true;
        }
    }
    
    // 如果没找到，添加新的配置
    layerConfig->keys.push_back(config);
    KEYBOARD_LOG_I("Added new key config for position %d in layer %s", 
                  position, layerConfig->name.c_str());
    return true;
}

bool KeyboardConfigManager::validateConfig() const {
    // 检查基本配置
    if (_layoutConfig.layers.empty()) {
        KEYBOARD_LOG_E("No layers configured");
        return false;
    }
    
    // 检查必需的层级
    bool hasPrimary = false, hasSecondary = false;
    for (const auto& layer : _layoutConfig.layers) {
        if (layer.layer == KeyLayer::PRIMARY) hasPrimary = true;
        if (layer.layer == KeyLayer::SECONDARY) hasSecondary = true;
    }
    
    if (!hasPrimary || !hasSecondary) {
        KEYBOARD_LOG_E("Missing required layers (Primary: %d, Secondary: %d)", 
                      hasPrimary, hasSecondary);
        return false;
    }
    
    // 检查Tab键配置
    if (_layoutConfig.tabKeyPosition == 0 || _layoutConfig.tabKeyPosition > 22) {
        KEYBOARD_LOG_E("Invalid tab key position: %d", _layoutConfig.tabKeyPosition);
        return false;
    }
    
    return true;
}

void KeyboardConfigManager::printConfig() const {
    KEYBOARD_LOG_I("=== Keyboard Configuration ===");
    KEYBOARD_LOG_I("Name: %s", _layoutConfig.name.c_str());
    KEYBOARD_LOG_I("Version: %s", _layoutConfig.version.c_str());
    KEYBOARD_LOG_I("Tab Key Position: %d", _layoutConfig.tabKeyPosition);
    KEYBOARD_LOG_I("Current Layer: %d", (int)_currentLayer);
    
    for (const auto& layer : _layoutConfig.layers) {
        KEYBOARD_LOG_I("Layer: %s (%d keys)", layer.name.c_str(), layer.keys.size());
        for (const auto& key : layer.keys) {
            KEYBOARD_LOG_D("  Key %d: %s (%s)", key.position, 
                          key.symbol.c_str(), key.label.c_str());
        }
    }
}

KeyboardLayoutConfig KeyboardConfigManager::createDefaultConfig() {
    KeyboardLayoutConfig config;
    config.name = "Standard Calculator Layout";
    config.version = "1.0";
    config.defaultLayer = KeyLayer::PRIMARY;
    config.tabKeyPosition = 6;  // Key 6 as Tab
    config.layerSwitchTimeout = 30000;  // 30 seconds
    config.checksum = 0;  // Will be calculated when saving
    
    // 创建两个层级
    config.layers.push_back(createPrimaryLayer());
    config.layers.push_back(createSecondaryLayer());
    
    return config;
}

LayerConfig KeyboardConfigManager::createPrimaryLayer() {
    LayerConfig layer;
    layer.layer = KeyLayer::PRIMARY;
    layer.name = "Primary Layer";
    layer.description = "Main calculator functions";
    layer.isActive = true;
    
    // 按照新的布局配置按键
    layer.keys = {
        // 第一排
        {1,  KeyType::POWER,       "ON",  "Power On/Off",    Operator::NONE, "",           0, true},
        {6,  KeyType::LAYER_SWITCH,"TAB", "Layer Switch",   Operator::NONE, "",           0, true},
        {10, KeyType::FUNCTION,    "%",   "Percent",        Operator::PERCENT, "",        0, true},
        {15, KeyType::DELETE,      "⌫",   "Backspace",      Operator::NONE, "",           0, true},
        {19, KeyType::CLEAR,       "C",   "Clear",          Operator::NONE, "",           0, true},
        
        // 第二排
        {2,  KeyType::NUMBER,      "7",   "Seven",          Operator::NONE, "",           0, true},
        {7,  KeyType::NUMBER,      "8",   "Eight",          Operator::NONE, "",           0, true},
        {11, KeyType::NUMBER,      "9",   "Nine",           Operator::NONE, "",           0, true},
        {16, KeyType::OPERATOR,    "×",   "Multiply",       Operator::MULTIPLY, "",       0, true},
        {20, KeyType::FUNCTION,    "±",   "Plus/Minus",     Operator::NONE, "",           0, true},
        
        // 第三排
        {3,  KeyType::NUMBER,      "4",   "Four",           Operator::NONE, "",           0, true},
        {8,  KeyType::NUMBER,      "5",   "Five",           Operator::NONE, "",           0, true},
        {12, KeyType::NUMBER,      "6",   "Six",            Operator::NONE, "",           0, true},
        {17, KeyType::OPERATOR,    "−",   "Subtract",       Operator::SUBTRACT, "",       0, true},
        {21, KeyType::OPERATOR,    "÷",   "Divide",         Operator::DIVIDE, "",         0, true},
        
        // 第四排
        {4,  KeyType::NUMBER,      "1",   "One",            Operator::NONE, "",           0, true},
        {9,  KeyType::NUMBER,      "2",   "Two",            Operator::NONE, "",           0, true},
        {13, KeyType::NUMBER,      "3",   "Three",          Operator::NONE, "",           0, true},
        {18, KeyType::OPERATOR,    "+",   "Add",            Operator::ADD, "",            0, true},
        {22, KeyType::FUNCTION,    "=",   "Equals",         Operator::EQUALS, "",         0, true},
        
        // 第五排
        {5,  KeyType::NUMBER,      "0",   "Zero",           Operator::NONE, "",           0, true},
        {14, KeyType::DECIMAL,     ".",   "Decimal Point",  Operator::NONE, "",           0, true}
    };
    
    return layer;
}

LayerConfig KeyboardConfigManager::createSecondaryLayer() {
    LayerConfig layer;
    layer.layer = KeyLayer::SECONDARY;
    layer.name = "Secondary Layer";
    layer.description = "Extended functions (accessible via Tab)";
    layer.isActive = false;
    
    // 第二层的功能键配置（预留接口，可后续扩展）
    layer.keys = {
        // 第一排 - 预留扩展功能
        {1,  KeyType::POWER,       "OFF", "Power Off",      Operator::NONE, "power_off",  0, true},
        {6,  KeyType::LAYER_SWITCH,"TAB", "Back to Primary", Operator::NONE, "",          0, true},
        {10, KeyType::FUNCTION,    "M+",  "Memory Add",     Operator::NONE, "mem_add",    0, true},
        {15, KeyType::FUNCTION,    "MC",  "Memory Clear",   Operator::NONE, "mem_clear",  0, true},
        {19, KeyType::FUNCTION,    "AC",  "All Clear",      Operator::NONE, "all_clear",  0, true},
        
        // 第二排 - 科学函数预留
        {2,  KeyType::FUNCTION,    "x²",  "Square",         Operator::SQUARE, "square",   0, true},
        {7,  KeyType::FUNCTION,    "√",   "Square Root",    Operator::SQUARE_ROOT, "sqrt", 0, true},
        {11, KeyType::FUNCTION,    "1/x", "Reciprocal",     Operator::RECIPROCAL, "recip", 0, true},
        {16, KeyType::FUNCTION,    "×10", "Sci Notation",   Operator::NONE, "sci_not",    0, true},
        {20, KeyType::FUNCTION,    "M-",  "Memory Sub",     Operator::NONE, "mem_sub",    0, true},
        
        // 第三排 - 更多预留功能
        {3,  KeyType::FUNCTION,    "(",   "Left Paren",     Operator::NONE, "lparen",     0, true},
        {8,  KeyType::FUNCTION,    ")",   "Right Paren",    Operator::NONE, "rparen",     0, true},
        {12, KeyType::FUNCTION,    "π",   "Pi",             Operator::NONE, "pi",         0, true},
        {17, KeyType::FUNCTION,    "e",   "Euler Number",   Operator::NONE, "euler",      0, true},
        {21, KeyType::FUNCTION,    "MR",  "Memory Recall",  Operator::NONE, "mem_recall", 0, true},
        
        // 第四排 - 预留
        {4,  KeyType::FUNCTION,    "F1",  "Function 1",     Operator::NONE, "func1",      0, true},
        {9,  KeyType::FUNCTION,    "F2",  "Function 2",     Operator::NONE, "func2",      0, true},
        {13, KeyType::FUNCTION,    "F3",  "Function 3",     Operator::NONE, "func3",      0, true},
        {18, KeyType::FUNCTION,    "F4",  "Function 4",     Operator::NONE, "func4",      0, true},
        {22, KeyType::FUNCTION,    "ENT", "Enter",          Operator::NONE, "enter",      0, true},
        
        // 第五排 - 预留
        {5,  KeyType::FUNCTION,    "SPC", "Space",          Operator::NONE, "space",      0, true},
        {14, KeyType::FUNCTION,    ",",   "Comma/Separator", Operator::NONE, "comma",     0, true}
    };
    
    return layer;
}

uint32_t KeyboardConfigManager::calculateChecksum() const {
    // 简单的校验和算法
    uint32_t checksum = 0;
    
    // 配置名称和版本
    for (char c : _layoutConfig.name) {
        checksum += c;
    }
    for (char c : _layoutConfig.version) {
        checksum += c;
    }
    
    // 层级配置
    for (const auto& layer : _layoutConfig.layers) {
        checksum += (uint32_t)layer.layer;
        for (const auto& key : layer.keys) {
            checksum += key.position;
            checksum += (uint32_t)key.type;
            checksum += (uint32_t)key.operation;
        }
    }
    
    return checksum;
}

size_t KeyboardConfigManager::serializeConfig(uint8_t* buffer, size_t maxSize) const {
    // 简化的序列化实现（实际项目中可能需要更复杂的协议）
    if (maxSize < sizeof(KeyboardLayoutConfig)) {
        return 0;
    }
    
    // 这里应该实现具体的序列化逻辑
    // 为了简化，我们暂时只保存关键信息
    memcpy(buffer, &_layoutConfig, sizeof(_layoutConfig));
    return sizeof(_layoutConfig);
}

bool KeyboardConfigManager::deserializeConfig(const uint8_t* buffer, size_t size) {
    // 简化的反序列化实现
    if (size < sizeof(KeyboardLayoutConfig)) {
        return false;
    }
    
    // 这里应该实现具体的反序列化逻辑
    memcpy(&_layoutConfig, buffer, sizeof(_layoutConfig));
    return true;
}

LayerConfig* KeyboardConfigManager::findLayerConfig(KeyLayer layer) {
    for (auto& layerConfig : _layoutConfig.layers) {
        if (layerConfig.layer == layer) {
            return &layerConfig;
        }
    }
    return nullptr;
}

const LayerConfig* KeyboardConfigManager::findLayerConfig(KeyLayer layer) const {
    for (const auto& layerConfig : _layoutConfig.layers) {
        if (layerConfig.layer == layer) {
            return &layerConfig;
        }
    }
    return nullptr;
}

void KeyboardConfigManager::logMessage(const char* level, const char* format, ...) const {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // 使用现有的日志系统
    if (strcmp(level, "E") == 0) {
        LOG_E("KEYBOARD", "%s", buffer);
    } else if (strcmp(level, "W") == 0) {
        LOG_W("KEYBOARD", "%s", buffer);
    } else if (strcmp(level, "I") == 0) {
        LOG_I("KEYBOARD", "%s", buffer);
    } else if (strcmp(level, "D") == 0) {
        LOG_D("KEYBOARD", "%s", buffer);
    } else {
        LOG_V("KEYBOARD", "%s", buffer);
    }
}