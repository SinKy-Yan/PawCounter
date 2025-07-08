/**
 * @file SimpleHID.h
 * @brief 简单的HID小键盘功能
 * 
 * 实现基本的USB HID键盘功能，支持：
 * - 数字键 0-9
 * - 计算功能键 +, -, *, /, =, .
 * - 功能键 Enter, Backspace, Escape
 * 
 * 按键同时触发计算器功能和HID功能，无需模式切换
 * 
 * @author PawCounter Team
 * @date 2024-01-08
 */

#ifndef SIMPLE_HID_H
#define SIMPLE_HID_H

#include <Arduino.h>
#include <stdint.h>
#include "USB.h"
#include "USBHIDKeyboard.h"

/**
 * @brief 简单HID键盘类
 */
class SimpleHID {
public:
    /**
     * @brief 构造函数
     */
    SimpleHID();

    /**
     * @brief 初始化HID功能
     * @return true 成功，false 失败
     */
    bool begin();

    /**
     * @brief 处理按键事件
     * @param keyPosition 物理按键位置（1-22）
     * @param pressed true为按下，false为释放
     * @return true 处理成功，false 处理失败
     */
    bool handleKey(uint8_t keyPosition, bool pressed);

    /**
     * @brief 检查HID是否已连接
     * @return true 已连接，false 未连接
     */
    bool isConnected() const;

    /**
     * @brief 启用/禁用HID功能
     * @param enabled true 启用，false 禁用
     */
    void setEnabled(bool enabled);

    /**
     * @brief 检查HID功能是否启用
     * @return true 启用，false 禁用
     */
    bool isEnabled() const { return _enabled; }

    /**
     * @brief 获取按键映射的HID键码
     * @param keyPosition 物理按键位置（1-22）
     * @return HID键码，0表示无映射
     */
    uint8_t getHIDKeyCode(uint8_t keyPosition) const;

    /**
     * @brief 打印调试信息
     */
    void printDebugInfo() const;

private:
    USBHIDKeyboard _keyboard;  // USB HID键盘对象
    bool _enabled;             // HID功能是否启用
    bool _initialized;         // 是否已初始化
    uint8_t _pressedKeys[6];   // 当前按下的按键（最多6个）
    uint8_t _pressedKeyCount;  // 按下的按键数量

    /**
     * @brief 按键映射表
     * 将22个物理按键映射到HID键码
     * 0表示无映射
     */
    static const uint8_t _keyMapping[22];

    /**
     * @brief 更新HID报告
     */
    void updateReport();

    /**
     * @brief 添加按键到报告
     * @param keyCode HID键码
     * @return true 成功，false 失败（报告已满）
     */
    bool addKey(uint8_t keyCode);

    /**
     * @brief 从报告中移除按键
     * @param keyCode HID键码
     * @return true 成功，false 未找到
     */
    bool removeKey(uint8_t keyCode);

    /**
     * @brief 检查按键是否在报告中
     * @param keyCode HID键码
     * @return true 存在，false 不存在
     */
    bool isKeyPressed(uint8_t keyCode) const;
};

#endif // SIMPLE_HID_H