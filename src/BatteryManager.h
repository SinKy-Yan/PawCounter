// BatteryManager.h
#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

class BatteryManager {
public:
    /**
     * @brief 电池充电状态枚举
     */
    enum ChargingState {
        NOT_CHARGING,    // 未充电
        CHARGING,        // 充电中
        FULLY_CHARGED    // 充电完成
    };

    /**
     * @brief 初始化电池管理器
     * @details 初始化I2C通信、配置GPIO引脚和MAX17048
     * @return 如果初始化成功返回true，失败返回false
     */
    bool begin();

    /**
     * @brief 更新电池状态
     * @details 定期调用此函数更新电池电压、电量和充电状态
     * @note 建议每秒调用一次
     */
    void update();

    /**
     * @brief 获取电池电压
     * @return 电池电压值(V)
     */
    float getVoltage() { return _voltage; }

    /**
     * @brief 获取电池电量百分比
     * @return 电池电量(0-100%)
     */
    float getPercentage() { return _percentage; }

    /**
     * @brief 获取充电状态
     * @return 当前充电状态(NOT_CHARGING/CHARGING/FULLY_CHARGED)
     */
    ChargingState getChargingState() { return _chargingState; }

    /**
     * @brief 检查电池是否处于低电量状态
     * @return 如果电量低于阈值返回true，否则返回false
     */
    bool isLowBattery() { return _percentage <= LOW_BATTERY_THRESHOLD; }

private:
    // 常量定义
    static const uint8_t MAX17048_ADDR = 0x36;      // MAX17048 I2C地址
    static const uint8_t VCELL_REG = 0x02;          // 电压寄存器地址
    static const uint8_t SOC_REG = 0x04;            // 电量寄存器地址
    static constexpr float LOW_BATTERY_THRESHOLD = 15.0f;  // 低电量警告阈值(15%)
    static const uint32_t UPDATE_INTERVAL = 1000;    // 更新间隔(ms)
    
    // 状态变量
    float _voltage = 0;              // 当前电池电压
    float _percentage = 0;           // 当前电池电量百分比
    ChargingState _chargingState = NOT_CHARGING;  // 当前充电状态
    uint32_t _lastUpdateTime = 0;    // 上次更新时间

    /**
     * @brief 初始化MAX17048电量计
     * @return 初始化成功返回true，失败返回false
     */
    bool initMAX17048();

    /**
     * @brief 更新电池电压和电量读数
     */
    void updateBatteryReadings();

    /**
     * @brief 更新充电状态
     */
    void updateChargingState();

    /**
     * @brief 读取MAX17048寄存器值
     * @param reg 要读取的寄存器地址
     * @return 寄存器的16位值
     */
    uint16_t readRegister(uint8_t reg);
};

#endif