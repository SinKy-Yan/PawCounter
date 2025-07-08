/**
 * @file SimpleHID.cpp
 * @brief 简单的HID小键盘功能实现
 * 
 * @author PawCounter Team
 * @date 2024-01-08
 */

#include "SimpleHID.h"
#include "Logger.h"

#define TAG_HID "SimpleHID"

// 物理按键 -> USB HID Usage (十六进制)
// 参考 KeyboardConfig.cpp 主层定义
// 使用数字小键盘键码，方便在主机侧直接输入数字 / 运算符
const uint8_t SimpleHID::_keyMapping[22] = {
    0x00,  // 1  POWER        -> 无 HID 输出
    0x37,  // 2  "7"          -> ASCII '7'
    0x34,  // 3  "4"          -> ASCII '4'
    0x31,  // 4  "1"          -> ASCII '1'
    0x30,  // 5  "0"          -> ASCII '0'
    0x09,  // 6  TAB           -> ASCII Tab
    0x38,  // 7  "8"          -> ASCII '8'
    0x35,  // 8  "5"          -> ASCII '5'
    0x32,  // 9  "2"          -> ASCII '2'
    0x25,  // 10 "%"          -> ASCII '%'
    0x39,  // 11 "9"          -> ASCII '9'
    0x36,  // 12 "6"          -> ASCII '6'
    0x33,  // 13 "3"          -> ASCII '3'
    0x2E,  // 14 "."          -> ASCII '.'
    0x08,  // 15 Backspace     -> ASCII Backspace
    0x2A,  // 16 "×"(乘号)    -> ASCII '*'
    0x2D,  // 17 "-"          -> ASCII '-'
    0x2B,  // 18 "+"          -> ASCII '+'
    0x43,  // 19 "C"(Clear)   -> ASCII 'C'
    0x00,  // 20 "±"          -> 无映射
    0x2F,  // 21 "÷"          -> ASCII '/'
    0x3D   // 22 "="          -> ASCII '='
};

SimpleHID::SimpleHID() 
    : _enabled(false)
    , _initialized(false)
    , _pressedKeyCount(0) {
    // 初始化按键数组
    memset(_pressedKeys, 0, sizeof(_pressedKeys));
}

bool SimpleHID::begin() {
    if (_initialized) {
        return true;
    }

    LOG_I(TAG_HID, "初始化简单HID键盘功能");

    // 初始化USB
    USB.begin();
    
    // 初始化HID键盘
    _keyboard.begin();
    
    _initialized = true;
    _enabled = true;
    
    LOG_I(TAG_HID, "HID键盘功能初始化完成");
    return true;
}

bool SimpleHID::handleKey(uint8_t keyPosition, bool pressed) {
    // 检查HID功能是否启用
    if (!_enabled || !_initialized) {
        return false;
    }

    // 检查按键位置是否有效
    if (keyPosition < 1 || keyPosition > 22) {
        LOG_W(TAG_HID, "无效的按键位置: %d", keyPosition);
        return false;
    }

    // 获取HID键码
    uint8_t keyCode = getHIDKeyCode(keyPosition);
    if (keyCode == 0) {
        // 该按键无HID映射，忽略
        return true;
    }

    LOG_D(TAG_HID, "处理按键事件: 位置=%d, 按下=%s, HID码=0x%02X", 
          keyPosition, pressed ? "是" : "否", keyCode);

    bool success = false;
    if (pressed) {
        // 按键按下
        success = addKey(keyCode);
        if (success) {
            _keyboard.press(keyCode);
        }
    } else {
        // 按键释放
        success = removeKey(keyCode);
        if (success) {
            _keyboard.release(keyCode);
        }
    }

    return success;
}

bool SimpleHID::isConnected() const {
    // 检查USB是否已连接 (Arduino-ESP32 TinyUSB 提供 connected())
    return (bool)USB;
}

void SimpleHID::setEnabled(bool enabled) {
    _enabled = enabled;
    LOG_I(TAG_HID, "HID功能%s", enabled ? "已启用" : "已禁用");
    
    if (!enabled) {
        // 禁用时释放所有按键
        _keyboard.releaseAll();
        _pressedKeyCount = 0;
        memset(_pressedKeys, 0, sizeof(_pressedKeys));
    }
}

uint8_t SimpleHID::getHIDKeyCode(uint8_t keyPosition) const {
    if (keyPosition < 1 || keyPosition > 22) {
        return 0;
    }
    return _keyMapping[keyPosition - 1];
}

void SimpleHID::printDebugInfo() const {
    Serial.println("=== 简单HID键盘状态 ===");
    Serial.printf("初始化状态: %s\n", _initialized ? "已初始化" : "未初始化");
    Serial.printf("启用状态: %s\n", _enabled ? "已启用" : "已禁用");
    Serial.printf("USB连接: %s\n", isConnected() ? "已连接" : "未连接");
    Serial.printf("当前按下按键数: %d\n", _pressedKeyCount);
    
    if (_pressedKeyCount > 0) {
        Serial.print("按下的按键: ");
        for (uint8_t i = 0; i < _pressedKeyCount; i++) {
            Serial.printf("0x%02X ", _pressedKeys[i]);
        }
        Serial.println();
    }
    
    Serial.println("\n--- 按键映射表 ---");
    for (uint8_t i = 0; i < 22; i++) {
        uint8_t keyCode = _keyMapping[i];
        if (keyCode != 0) {
            Serial.printf("按键%2d -> HID 0x%02X\n", i + 1, keyCode);
        } else {
            Serial.printf("按键%2d -> 无映射\n", i + 1);
        }
    }
    Serial.println("======================");
}

bool SimpleHID::addKey(uint8_t keyCode) {
    // 检查是否已经按下
    if (isKeyPressed(keyCode)) {
        return true; // 已经在列表中
    }
    
    // 检查是否还有空间
    if (_pressedKeyCount >= 6) {
        LOG_W(TAG_HID, "按键缓冲区已满，无法添加按键 0x%02X", keyCode);
        return false;
    }
    
    // 添加到列表
    _pressedKeys[_pressedKeyCount] = keyCode;
    _pressedKeyCount++;
    
    LOG_D(TAG_HID, "添加按键 0x%02X，当前按键数: %d", keyCode, _pressedKeyCount);
    return true;
}

bool SimpleHID::removeKey(uint8_t keyCode) {
    // 查找按键
    for (uint8_t i = 0; i < _pressedKeyCount; i++) {
        if (_pressedKeys[i] == keyCode) {
            // 找到了，移除它
            for (uint8_t j = i; j < _pressedKeyCount - 1; j++) {
                _pressedKeys[j] = _pressedKeys[j + 1];
            }
            _pressedKeyCount--;
            _pressedKeys[_pressedKeyCount] = 0; // 清零最后一个位置
            
            LOG_D(TAG_HID, "移除按键 0x%02X，当前按键数: %d", keyCode, _pressedKeyCount);
            return true;
        }
    }
    
    LOG_W(TAG_HID, "未找到要移除的按键 0x%02X", keyCode);
    return false;
}

bool SimpleHID::isKeyPressed(uint8_t keyCode) const {
    for (uint8_t i = 0; i < _pressedKeyCount; i++) {
        if (_pressedKeys[i] == keyCode) {
            return true;
        }
    }
    return false;
}