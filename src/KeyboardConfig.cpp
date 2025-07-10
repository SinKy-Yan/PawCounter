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
    KEYBOARD_LOG_I("初始化键盘配置管理器");
    
    // 初始化Preferences
    if (!_preferences.begin(PREF_NAMESPACE, false)) {
        KEYBOARD_LOG_E("初始化Preferences失败");
        return false;
    }
    
    // 加载配置
    if (!loadConfig()) {
        KEYBOARD_LOG_W("加载已保存的配置失败，使用默认配置");
        resetToDefault();
    }
    
    KEYBOARD_LOG_I("键盘配置管理器初始化成功");
    return true;
}

bool KeyboardConfigManager::loadConfig(bool forceDefault) {
    if (forceDefault) {
        KEYBOARD_LOG_I("强制使用默认配置");
        _layoutConfig = createDefaultConfig();
        return true;
    }
    
    // 检查是否有保存的配置
    if (!_preferences.isKey(PREF_CONFIG_KEY)) {
        KEYBOARD_LOG_I("未找到已保存的配置，创建默认配置");
        _layoutConfig = createDefaultConfig();
        return saveConfig();
    }
    
    // 读取配置版本和校验和
    String savedVersion = _preferences.getString(PREF_VERSION_KEY, "");
    uint32_t savedChecksum = _preferences.getULong(PREF_CHECKSUM_KEY, 0);
    
    // 读取配置数据
    size_t configSize = _preferences.getBytesLength(PREF_CONFIG_KEY);
    if (configSize == 0) {
        KEYBOARD_LOG_E("无效的配置大小");
        return false;
    }
    
    uint8_t* buffer = new uint8_t[configSize];
    size_t readSize = _preferences.getBytes(PREF_CONFIG_KEY, buffer, configSize);
    
    if (readSize != configSize) {
        KEYBOARD_LOG_E("配置读取大小不匹配");
        delete[] buffer;
        return false;
    }
    
    // 反序列化配置
    bool result = deserializeConfig(buffer, configSize);
    delete[] buffer;
    
    if (!result) {
        KEYBOARD_LOG_E("反序列化配置失败");
        return false;
    }
    
    // 验证校验和
    uint32_t calculatedChecksum = calculateChecksum();
    if (calculatedChecksum != savedChecksum) {
        KEYBOARD_LOG_W("配置校验和不匹配，使用默认配置");
        _layoutConfig = createDefaultConfig();
        return saveConfig();
    }
    
    // 验证配置完整性
    if (!validateConfig()) {
        KEYBOARD_LOG_E("配置验证失败");
        _layoutConfig = createDefaultConfig();
        return saveConfig();
    }
    
    KEYBOARD_LOG_I("配置加载成功，版本: %s", savedVersion.c_str());
    return true;
}

bool KeyboardConfigManager::saveConfig() {
    KEYBOARD_LOG_D("正在保存键盘配置");
    
    // 更新校验和
    _layoutConfig.checksum = calculateChecksum();
    
    // 序列化配置
    const size_t maxBufferSize = 4096;  // 4KB应该足够
    uint8_t* buffer = new uint8_t[maxBufferSize];
    size_t configSize = serializeConfig(buffer, maxBufferSize);
    
    if (configSize == 0) {
        KEYBOARD_LOG_E("序列化配置失败");
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
        KEYBOARD_LOG_I("配置保存成功");
    } else {
        KEYBOARD_LOG_E("配置保存失败");
    }
    
    return result;
}

bool KeyboardConfigManager::resetToDefault() {
    KEYBOARD_LOG_I("正在重置为默认配置");
    
    _layoutConfig = createDefaultConfig();
    _currentLayer = _layoutConfig.defaultLayer;
    
    return saveConfig();
}

bool KeyboardConfigManager::switchToLayer(KeyLayer layer) {
    if (layer >= KeyLayer::MAX_LAYERS) {
        KEYBOARD_LOG_E("无效的层: %d", (int)layer);
        return false;
    }
    
    const LayerConfig* layerConfig = findLayerConfig(layer);
    if (!layerConfig) {
        KEYBOARD_LOG_E("未找到层配置: %d", (int)layer);
        return false;
    }
    
    _currentLayer = layer;
    _lastLayerSwitchTime = millis();
    
    KEYBOARD_LOG_I("已切换到层: %s", layerConfig->name.c_str());
    return true;
}

bool KeyboardConfigManager::handleTabKey(bool isLongPress) {
    if (isLongPress) {
        KEYBOARD_LOG_D("检测到Tab键长按");
        
        // 执行长按回调
        if (_tabBehavior.onLongPress) {
            _tabBehavior.onLongPress();
        }
        
        return true;
    } else {
        KEYBOARD_LOG_D("检测到Tab键短按");
        
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
        KEYBOARD_LOG_E("未找到层: %d", (int)layer);
        return false;
    }
    
    // 查找现有按键配置
    for (auto& keyConfig : layerConfig->keys) {
        if (keyConfig.position == position) {
            keyConfig = config;
            KEYBOARD_LOG_I("已更新位置 %d 在层 %s 的按键配置", 
                          position, layerConfig->name.c_str());
            return true;
        }
    }
    
    // 如果没找到，添加新的配置
    layerConfig->keys.push_back(config);
    KEYBOARD_LOG_I("已为位置 %d 在层 %s 添加新的按键配置", 
                  position, layerConfig->name.c_str());
    return true;
}

bool KeyboardConfigManager::validateConfig() const {
    // 检查基本配置
    if (_layoutConfig.layers.empty()) {
        KEYBOARD_LOG_E("未配置任何层");
        return false;
    }
    
    // 检查必需的层级
    bool hasPrimary = false, hasSecondary = false;
    for (const auto& layer : _layoutConfig.layers) {
        if (layer.layer == KeyLayer::PRIMARY) hasPrimary = true;
        if (layer.layer == KeyLayer::SECONDARY) hasSecondary = true;
    }
    
    if (!hasPrimary) {
        KEYBOARD_LOG_E("未找到主层");
        return false;
    }
    if (!hasSecondary) {
        KEYBOARD_LOG_E("未找到次层");
        return false;
    }
    
    // 验证Tab键位置
    if (_layoutConfig.tabKeyPosition == 0 || _layoutConfig.tabKeyPosition > 22) {
        KEYBOARD_LOG_E("Tab键位置 %d 无效", _layoutConfig.tabKeyPosition);
        return false;
    }
    
    // 检查每个层
    for (const auto& layer : _layoutConfig.layers) {
        std::vector<uint8_t> positions;
        for (const auto& key : layer.keys) {
            // 验证按键类型
            if (key.type >= KeyType::MAX_KEY_TYPES) {
                KEYBOARD_LOG_E("在层 %s 的按键位置 %d 有无效的类型 %d", 
                               layer.name.c_str(), key.position, (int)key.type);
                return false;
            }
            // 检查重复位置
            if (std::find(positions.begin(), positions.end(), key.position) != positions.end()) {
                KEYBOARD_LOG_E("在层 %s 的按键位置 %d 是重复的", 
                               layer.name.c_str(), key.position);
                return false;
            }
            positions.push_back(key.position);
        }
    }
    
    KEYBOARD_LOG_I("配置有效");
    return true;
}

void KeyboardConfigManager::printConfig() const {
    Serial.println("=== 键盘配置 ===");
    Serial.printf("名称: %s\n", _layoutConfig.name.c_str());
    Serial.printf("版本: %s\n", _layoutConfig.version.c_str());
    Serial.printf("Tab键位置: %d\n", _layoutConfig.tabKeyPosition);
    Serial.printf("当前层: %d\n", (int)_currentLayer);
    for (const auto& layer : _layoutConfig.layers) {
        Serial.printf("层: %s (%d 个键)\n", layer.name.c_str(), layer.keys.size());
    }
}

KeyboardLayoutConfig KeyboardConfigManager::createDefaultConfig() {
    KEYBOARD_LOG_D("创建默认键盘配置");
    
    KeyboardLayoutConfig config;
    config.name = "标准计算器布局";
    config.version = "1.0";
    config.tabKeyPosition = 6;
    config.defaultLayer = KeyLayer::PRIMARY;
    
    // --- 主层 (Primary Layer) ---
    LayerConfig primaryLayer;
    primaryLayer.layer = KeyLayer::PRIMARY;
    primaryLayer.name = "主层";
    primaryLayer.keys = {
        {1, KeyType::POWER, "ON", "POWER", Operator::NONE, "power", 0, true},
        {2, KeyType::NUMBER, "7", "SEVEN", Operator::NONE, "", 0, true},
        {3, KeyType::NUMBER, "4", "FOUR", Operator::NONE, "", 0, true},
        {4, KeyType::NUMBER, "1", "ONE", Operator::NONE, "", 0, true},
        {5, KeyType::NUMBER, "0", "ZERO", Operator::NONE, "", 0, true},
        {6, KeyType::LAYER_SWITCH, "TAB", "LAYER_SWITCH", Operator::NONE, "", 0, true},
        {7, KeyType::NUMBER, "8", "EIGHT", Operator::NONE, "", 0, true},
        {8, KeyType::NUMBER, "5", "FIVE", Operator::NONE, "", 0, true},
        {9, KeyType::NUMBER, "2", "TWO", Operator::NONE, "", 0, true},
        {10, KeyType::FUNCTION, "%", "PERCENT", Operator::PERCENT, "", 0, true},
        {11, KeyType::NUMBER, "9", "NINE", Operator::NONE, "", 0, true},
        {12, KeyType::NUMBER, "6", "SIX", Operator::NONE, "", 0, true},
        {13, KeyType::NUMBER, "3", "THREE", Operator::NONE, "", 0, true},
        {14, KeyType::DECIMAL, ".", "DOT", Operator::NONE, "", 0, true},
        {15, KeyType::DELETE, "⌫", "BACKSPACE", Operator::NONE, "", 0, true},
        {16, KeyType::OPERATOR, "×", "MUL", Operator::MULTIPLY, "", 0, true},
        {17, KeyType::OPERATOR, "-", "SUB", Operator::SUBTRACT, "", 0, true},
        {18, KeyType::OPERATOR, "+", "ADD", Operator::ADD, "", 0, true},
        {19, KeyType::CLEAR, "C", "CLEAR", Operator::NONE, "", 0, true},
        {20, KeyType::FUNCTION, "±", "SIGN", Operator::NONE, "", 0, true},
        {21, KeyType::OPERATOR, "÷", "DIV", Operator::DIVIDE, "", 0, true},
        {22, KeyType::FUNCTION, "=", "EQUALS", Operator::EQUALS, "", 0, true}
    };
    config.layers.push_back(primaryLayer);
    
    // --- 次层 (Secondary Layer) ---
    LayerConfig secondaryLayer;
    secondaryLayer.layer = KeyLayer::SECONDARY;
    secondaryLayer.name = "次层";
    secondaryLayer.keys = {
        // 示例：可以添加更多科学计算功能
        {2, KeyType::FUNCTION, "√", "SQRT", Operator::SQUARE_ROOT},
        {3, KeyType::FUNCTION, "x²", "SQUARE", Operator::SQUARE},
        {4, KeyType::FUNCTION, "1/x", "RECIPROCAL", Operator::RECIPROCAL},
        // 其他按键保持与主层一致或禁用
        {7, KeyType::MEMORY, "M+", "M_ADD"},
        {8, KeyType::MEMORY, "M-", "M_SUB"},
        {9, KeyType::MEMORY, "MR", "M_RECALL"},
        {11, KeyType::MEMORY, "MC", "M_CLEAR"},
    };
    config.layers.push_back(secondaryLayer);
    
    return config;
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
    // 简化的序列化实现 - 避免直接复制含有std::vector的结构体
    if (maxSize < 64) {  // 最小缓冲区大小
        return 0;
    }
    
    // 只序列化基本信息，避免std::vector的问题
    size_t offset = 0;
    
    // 写入版本信息长度和内容
    uint8_t versionLen = _layoutConfig.version.length();
    buffer[offset++] = versionLen;
    memcpy(buffer + offset, _layoutConfig.version.c_str(), versionLen);
    offset += versionLen;
    
    // 写入其他基本信息
    buffer[offset++] = (uint8_t)_layoutConfig.defaultLayer;
    buffer[offset++] = _layoutConfig.tabKeyPosition;
    
    // 写入简单的配置标识
    uint32_t configId = 0x12345678;  // 简单的配置标识
    memcpy(buffer + offset, &configId, sizeof(configId));
    offset += sizeof(configId);
    
    return offset;
}

bool KeyboardConfigManager::deserializeConfig(const uint8_t* buffer, size_t size) {
    // 简化的反序列化实现 - 避免直接复制含有std::vector的结构体
    if (size < 8) {  // 最小数据大小
        return false;
    }
    
    size_t offset = 0;
    
    // 读取版本信息
    uint8_t versionLen = buffer[offset++];
    if (offset + versionLen > size) {
        return false;
    }
    
    String version;
    version.reserve(versionLen);
    for (int i = 0; i < versionLen; i++) {
        version += (char)buffer[offset++];
    }
    
    // 读取其他基本信息
    if (offset + 2 + sizeof(uint32_t) > size) {
        return false;
    }
    
    KeyLayer defaultLayer = (KeyLayer)buffer[offset++];
    uint8_t tabKeyPosition = buffer[offset++];
    
    // 读取配置标识
    uint32_t configId;
    memcpy(&configId, buffer + offset, sizeof(configId));
    offset += sizeof(configId);
    
    // 验证配置标识
    if (configId != 0x12345678) {
        KEYBOARD_LOG_W("Invalid config ID: 0x%08X", configId);
        return false;
    }
    
    // 由于我们无法安全地反序列化std::vector，使用默认配置
    _layoutConfig = createDefaultConfig();
    _layoutConfig.version = version;
    _layoutConfig.defaultLayer = defaultLayer;
    _layoutConfig.tabKeyPosition = tabKeyPosition;
    
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