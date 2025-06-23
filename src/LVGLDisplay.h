/**
 * @file LVGLDisplay.h
 * @brief LVGL显示适配器
 * @details 基于LVGL图形库的计算器显示实现
 * 
 * 功能特性：
 * - 基于LVGL 8.x的现代UI界面
 * - 硬件加速支持（如果可用）
 * - 响应式布局设计
 * - 流畅的动画和转场效果
 * - 触摸屏支持（预留）
 * - 主题系统集成
 * 
 * @author Calculator Project  
 * @date 2024-01-07
 * @version 1.0
 */

#ifndef LVGL_DISPLAY_H
#define LVGL_DISPLAY_H

#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "CalculatorDisplay.h"
#include "Logger.h"

/**
 * @brief LVGL显示适配器类
 * @details 将LCDDisplay接口适配到LVGL图形库
 */
class LVGLDisplay : public LCDDisplay {
public:
    /**
     * @brief 构造函数
     * @param gfx Arduino_GFX显示对象指针（用于底层驱动）
     * @param width 显示宽度
     * @param height 显示高度
     */
    explicit LVGLDisplay(Arduino_GFX* gfx, uint16_t width = 480, uint16_t height = 128);
    
    /**
     * @brief 析构函数
     */
    ~LVGLDisplay() override;
    
    // 实现基类接口
    bool begin() override;
    void clear() override;
    void updateDisplay(const String& number, 
                      const String& expression,
                      const std::vector<CalculationHistory>& history,
                      CalculatorState state) override;
    void showError(CalculatorError error, const String& message) override;
    void showStatus(const String& message) override;
    void setTheme(const DisplayTheme& theme) override;
    void setNumberFormat(const NumberFormat& format) override;
    void setUnitDisplay(const UnitDisplay& unitDisplay) override;
    
    /**
     * @brief LVGL任务处理器（需要在主循环中调用）
     */
    void update();
    
    /**
     * @brief 设置显示旋转
     * @param rotation 旋转角度 (0, 90, 180, 270)
     */
    void setRotation(uint16_t rotation);
    
    /**
     * @brief 获取LVGL屏幕对象
     * @return LVGL屏幕对象指针
     */
    lv_obj_t* getScreen() const { return _screen; }
    
    /**
     * @brief 显示测试网格和边界信息
     */
    void showTestGrid();
    
    /**
     * @brief 清除测试显示，恢复正常UI
     */
    void clearTestGrid();

private:
    Arduino_GFX* _gfx;                  ///< 底层显示驱动
    uint16_t _width;                    ///< 显示宽度
    uint16_t _height;                   ///< 显示高度
    
    // LVGL对象
    lv_disp_t* _display;                ///< LVGL显示对象
    lv_obj_t* _screen;                  ///< 屏幕对象
    lv_obj_t* _mainContainer;           ///< 主容器
    
    // UI组件
    lv_obj_t* _mainNumberLabel;         ///< 主数字标签
    lv_obj_t* _expressionLabel;         ///< 表达式标签
    lv_obj_t* _statusLabel;             ///< 状态标签
    lv_obj_t* _modeLabel;               ///< 模式标签
    lv_obj_t* _historyContainer;        ///< 历史记录容器
    lv_obj_t* _errorPanel;              ///< 错误面板
    lv_obj_t* _unitPanel;               ///< 单位显示面板
    
    // 样式对象
    lv_style_t _mainStyle;              ///< 主样式
    lv_style_t _numberStyle;            ///< 数字样式
    lv_style_t _expressionStyle;        ///< 表达式样式
    lv_style_t _statusStyle;            ///< 状态样式
    lv_style_t _errorStyle;             ///< 错误样式
    lv_style_t _unitStyle;              ///< 单位样式
    
    // 配置
    DisplayTheme _theme;                ///< 当前主题
    NumberFormat _numberFormat;         ///< 数字格式
    UnitDisplay _unitDisplay;           ///< 单位显示配置
    
    // 缓冲区（LVGL需要）
    static lv_disp_draw_buf_t _drawBuf; ///< 绘制缓冲区
    static lv_color_t* _buf1;           ///< 缓冲区1
    static lv_color_t* _buf2;           ///< 缓冲区2（双缓冲）
    
    // 静态回调函数
    static void displayFlushCb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p);
    
    /**
     * @brief 初始化LVGL系统
     * @return true 成功，false 失败
     */
    bool initLVGL();
    
    /**
     * @brief 创建UI组件
     */
    void createUI();
    
    /**
     * @brief 初始化样式
     */
    void initStyles();
    
    /**
     * @brief 应用主题设置
     */
    void applyTheme();
    
    /**
     * @brief 布局UI组件
     */
    void layoutUI();
    
    /**
     * @brief 更新主数字显示
     * @param number 数字字符串
     */
    void updateMainNumber(const String& number);
    
    /**
     * @brief 更新表达式显示
     * @param expression 表达式字符串
     */
    void updateExpression(const String& expression);
    
    /**
     * @brief 更新历史记录显示
     * @param history 历史记录
     */
    void updateHistory(const std::vector<CalculationHistory>& history);
    
    /**
     * @brief 更新状态栏
     * @param state 计算器状态
     */
    void updateStatusBar(CalculatorState state);
    
    /**
     * @brief 显示单位标识
     * @param number 数字
     */
    void updateUnitLabels(const String& number);
    
    /**
     * @brief 转换显示主题到LVGL颜色
     * @param color RGB565颜色
     * @return LVGL颜色
     */
    lv_color_t convertColor(uint16_t color);
    
    /**
     * @brief 格式化数字显示
     * @param number 原始数字
     * @return 格式化后的字符串
     */
    String formatDisplayNumber(const String& number);
    
    /**
     * @brief 获取状态文本
     * @param state 计算器状态
     * @return 状态文本
     */
    String getStateText(CalculatorState state);
    
    /**
     * @brief 获取错误文本
     * @param error 错误类型
     * @return 错误文本
     */
    String getErrorText(CalculatorError error);
    
    /**
     * @brief 显示错误动画
     */
    void showErrorAnimation();
    
    /**
     * @brief 隐藏错误面板
     */
    void hideErrorPanel();
    
    /**
     * @brief 获取默认主题
     * @return 默认主题配置
     */
    DisplayTheme getDefaultTheme();
    
    /**
     * @brief 获取默认数字格式
     * @return 默认数字格式
     */
    NumberFormat getDefaultNumberFormat();
};

/**
 * @brief LVGL日志回调函数
 * @param level 日志级别
 * @param file 文件名
 * @param line 行号
 * @param func 函数名
 * @param dsc 描述
 */
void lvglLogCb(lv_log_level_t level, const char* file, uint32_t line, const char* func, const char* dsc);

// LVGL颜色转换宏
#define RGB565_TO_LVGL(color) lv_color_hex(((color & 0xF800) << 8) | ((color & 0x07E0) << 5) | ((color & 0x001F) << 3))

#endif // LVGL_DISPLAY_H