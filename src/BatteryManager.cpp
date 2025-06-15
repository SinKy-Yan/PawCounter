// BatteryManager.cpp
#include "BatteryManager.h"

/**
 * @brief 初始化电池管理器
 * @details 配置GPIO引脚，初始化I2C通信和MAX17048
 * @return 初始化成功返回true，失败返回false
 */
bool BatteryManager::begin() {
    // 配置TP4056引脚为输入上拉
    pinMode(TP4056_STDBY_PIN, INPUT_PULLUP);
    pinMode(TP4056_CHRG_PIN, INPUT_PULLUP);

    // 初始化I2C通信
    Wire.begin(MAX17048_SDA_PIN, MAX17048_SCL_PIN);
    
    // 初始化MAX17048
    if (!initMAX17048()) {
        return false;
    }

    // 首次更新数据
    update();
    
    return true;
}

/**
 * @brief 初始化MAX17048电量计
 * @details 检查设备是否存在并等待其就绪
 * @return 初始化成功返回true，失败返回false
 */
bool BatteryManager::initMAX17048() {
    Wire.beginTransmission(MAX17048_ADDR);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    delay(10);  // 等待设备就绪
    return true;
}

/**
 * @brief 更新电池状态
 * @details 定期更新电池电压、电量和充电状态
 */
void BatteryManager::update() {
    uint32_t currentTime = millis();
    if (currentTime - _lastUpdateTime >= UPDATE_INTERVAL) {
        updateBatteryReadings();
        updateChargingState();
        _lastUpdateTime = currentTime;
    }
}

/**
 * @brief 更新电池电压和电量读数
 * @details 从MAX17048读取电压和SOC寄存器并转换为实际值
 */
void BatteryManager::updateBatteryReadings() {
    // 读取电压并转换为伏特
    uint16_t vcell = readRegister(VCELL_REG);
    _voltage = (float)vcell * 78.125f / 1000000.0f;

    // 读取SOC并转换为百分比
    uint16_t soc = readRegister(SOC_REG);
    _percentage = ((float)soc) / 256.0f;
}

/**
 * @brief 更新充电状态
 * @details 根据TP4056的STDBY和CHRG引脚状态判断当前充电状态
 */
void BatteryManager::updateChargingState() {
    bool stdby = digitalRead(TP4056_STDBY_PIN); // 低电平表示充电完成
    bool chrg = digitalRead(TP4056_CHRG_PIN);   // 低电平表示正在充电

    if (!chrg) {  // 正在充电
        _chargingState = CHARGING;
    } else if (!stdby) {  // 充电完成
        _chargingState = FULLY_CHARGED;
    } else {  // 未充电
        _chargingState = NOT_CHARGING;
    }
}

/**
 * @brief 读取MAX17048寄存器值
 * @param reg 要读取的寄存器地址
 * @return 寄存器的16位值，读取失败返回0
 */
uint16_t BatteryManager::readRegister(uint8_t reg) {
    Wire.beginTransmission(MAX17048_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return 0;
    }
    
    if (Wire.requestFrom((uint8_t)MAX17048_ADDR, (uint8_t)2) != 2) {
        return 0;
    }
    
    uint16_t value = (Wire.read() << 8) | Wire.read();
    return value;
}