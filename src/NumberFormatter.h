/**
 * @file NumberFormatter.h
 * @brief 数字格式化工具类
 * @details 提供统一的数字格式化功能，支持智能小数显示和精度控制
 * 
 * 格式化规则：
 * - 整数显示为整数（如：3）
 * - 小数显示最多3位小数并去尾零（如：0.625、2.5）
 * - 采用四舍五入规则
 * 
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef NUMBER_FORMATTER_H
#define NUMBER_FORMATTER_H

#include <Arduino.h>
#include <math.h>

/**
 * @brief 数字格式化工具类
 */
class NumberFormatter {
public:
    /**
     * @brief 智能格式化数字
     * @param value 要格式化的数字
     * @param maxDecimals 最大小数位数（默认为3）
     * @return 格式化后的字符串
     * 
     * 规则：
     * - 如果是整数，不显示小数点
     * - 如果是小数，最多显示maxDecimals位，去掉尾随的零
     */
    static String format(double value, int maxDecimals = 3) {
        // 首先进行四舍五入到指定精度
        double rounded = roundTo(value, maxDecimals);
        
        // 检查是否为整数
        if (isInteger(rounded)) {
            return String((long)rounded);
        }
        
        // 格式化为字符串并去掉尾随的零
        String result = String(rounded, maxDecimals);
        
        // 去掉尾随的零和可能的小数点
        while (result.endsWith("0") && result.indexOf('.') != -1) {
            result.remove(result.length() - 1);
        }
        if (result.endsWith(".")) {
            result.remove(result.length() - 1);
        }
        
        return result;
    }
    
    /**
     * @brief 四舍五入到指定小数位数
     * @param value 要处理的数字
     * @param decimals 小数位数
     * @return 四舍五入后的数字
     */
    static double roundTo(double value, int decimals) {
        if (decimals <= 0) {
            return round(value);
        }
        
        double multiplier = pow(10.0, decimals);
        return round(value * multiplier) / multiplier;
    }
    
    /**
     * @brief 检查数字是否为整数（在浮点精度范围内）
     * @param value 要检查的数字
     * @return 如果是整数返回true，否则返回false
     */
    static bool isInteger(double value) {
        return abs(value - round(value)) < 1e-9;  // 考虑浮点精度误差
    }
    
    /**
     * @brief 格式化为固定小数位数（不去零）
     * @param value 要格式化的数字
     * @param decimals 小数位数
     * @return 格式化后的字符串
     */
    static String formatFixed(double value, int decimals) {
        double rounded = roundTo(value, decimals);
        return String(rounded, decimals);
    }
};

#endif // NUMBER_FORMATTER_H