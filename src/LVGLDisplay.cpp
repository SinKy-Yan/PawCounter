/**
 * @file LVGLDisplay.cpp
 * @brief LVGL显示适配器实现
 * @author Calculator Project
 * @date 2024-01-07
 * @version 1.0
 */

#include "LVGLDisplay.h"
#include "config.h"

// 静态成员初始化
lv_disp_draw_buf_t LVGLDisplay::_drawBuf;
lv_color_t* LVGLDisplay::_buf1 = nullptr;
lv_color_t* LVGLDisplay::_buf2 = nullptr;

// 日志标签已在Logger.h中定义

LVGLDisplay::LVGLDisplay(Arduino_GFX* gfx, uint16_t width, uint16_t height)
    : LCDDisplay(gfx)
    , _gfx(gfx)
    , _width(width)
    , _height(height)
    , _display(nullptr)
    , _screen(nullptr)
    , _mainContainer(nullptr)
    , _mainNumberLabel(nullptr)
    , _expressionLabel(nullptr)
    , _statusLabel(nullptr)
    , _modeLabel(nullptr)
    , _historyContainer(nullptr)
    , _errorPanel(nullptr)
    , _unitPanel(nullptr)
{
    // 初始化默认配置
    _theme = getDefaultTheme();
    _numberFormat = getDefaultNumberFormat();
    _unitDisplay = {};
    _unitDisplay.enabled = false;
    
    LVGL_LOG_I("LVGL显示适配器已创建 (%dx%d)", width, height);
}

LVGLDisplay::~LVGLDisplay() {
    // 清理LVGL资源
    if (_buf1) {
#ifdef ESP32
        heap_caps_free(_buf1);
#else
        free(_buf1);
#endif
        _buf1 = nullptr;
    }
    if (_buf2) {
#ifdef ESP32
        heap_caps_free(_buf2);
#else
        free(_buf2);
#endif
        _buf2 = nullptr;
    }
    
    LVGL_LOG_I("LVGL显示适配器已销毁");
}

bool LVGLDisplay::begin() {
    LVGL_LOG_I("初始化LVGL显示系统...");
    
    // 确保底层GFX已初始化
    if (!_gfx) {
        LVGL_LOG_E("Arduino_GFX对象未设置");
        return false;
    }
    
    // 初始化LVGL
    if (!initLVGL()) {
        LVGL_LOG_E("LVGL初始化失败");
        return false;
    }
    
    // 创建UI
    createUI();
    
    // 初始化样式
    initStyles();
    
    // 应用主题
    applyTheme();
    
    // 布局UI
    layoutUI();
    
    
    LVGL_LOG_I("LVGL显示系统初始化完成");
    return true;
}

bool LVGLDisplay::initLVGL() {
    // 初始化LVGL
    lv_init();
    
    // 设置日志回调
#if LV_USE_LOG
    // LVGL 8.x使用简化的日志回调
    // lv_log_register_print_cb(lvglLogCb);
#endif
    
    // 计算缓冲区大小（增加到64行，确保完整覆盖底部）
    uint32_t bufSize = _width * 64; // 64行缓冲区，减少分片绘制
    
#ifdef ESP32
    // ESP32优先使用内部内存
    _buf1 = (lv_color_t*)heap_caps_malloc(bufSize * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
    _buf1 = (lv_color_t*)malloc(bufSize * sizeof(lv_color_t));
#endif
    
    if (!_buf1) {
        LVGL_LOG_E("缓冲区1内存分配失败 (需要%d字节)", bufSize * sizeof(lv_color_t));
        return false;
    }
    
#ifdef ESP32
    // 第二个缓冲区也使用内部内存（双缓冲）
    _buf2 = (lv_color_t*)heap_caps_malloc(bufSize * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
    _buf2 = (lv_color_t*)malloc(bufSize * sizeof(lv_color_t));
#endif
    
    if (!_buf2) {
        LVGL_LOG_W("缓冲区2内存分配失败，使用单缓冲模式");
#ifdef ESP32
        heap_caps_free(_buf1);
#else
        free(_buf1);
#endif
        _buf1 = nullptr;
        return false;
    }
    
    // 初始化绘制缓冲区
    lv_disp_draw_buf_init(&_drawBuf, _buf1, _buf2, bufSize);
    
    // 初始化显示驱动
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = _width;
    disp_drv.ver_res = 126;  // 限制到安全的126行，避开底部2行花屏
    disp_drv.flush_cb = displayFlushCb;
    disp_drv.draw_buf = &_drawBuf;
    disp_drv.user_data = _gfx;  // 传递GFX对象指针
    
    // 注册显示驱动
    _display = lv_disp_drv_register(&disp_drv);
    if (!_display) {
        LVGL_LOG_E("显示驱动注册失败");
        return false;
    }
    
    // 设置180度旋转
    lv_disp_set_rotation(_display, LV_DISP_ROT_180);
    LVGL_LOG_I("LVGL显示初始化完成，已设置180度旋转");
    
    LVGL_LOG_I("LVGL核心初始化完成 (缓冲区: %d bytes)", bufSize * sizeof(lv_color_t) * 2);
    return true;
}

void LVGLDisplay::createUI() {
    LVGL_LOG_D("创建现代化计算器UI组件...");
    
    // 创建主屏幕
    _screen = lv_obj_create(NULL);
    lv_scr_load(_screen);
    
    // 创建主容器 - 全屏
    _mainContainer = lv_obj_create(_screen);
    lv_obj_set_size(_mainContainer, _width, 126);  // 限制到安全高度
    lv_obj_align(_mainContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(_mainContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(_mainContainer, 0, 0);
    lv_obj_set_style_border_width(_mainContainer, 0, 0);
    
    // ================ 顶部显示区域 ================
    // 创建显示面板 (顶部50px高度)
    lv_obj_t* displayPanel = lv_obj_create(_mainContainer);
    lv_obj_set_size(displayPanel, _width, 50);
    lv_obj_align(displayPanel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(displayPanel, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_border_width(displayPanel, 1, 0);
    lv_obj_set_style_border_color(displayPanel, lv_color_hex(0x404040), 0);
    lv_obj_set_style_radius(displayPanel, 8, 0);
    lv_obj_clear_flag(displayPanel, LV_OBJ_FLAG_SCROLLABLE);
    
    // 主数字显示 - 大字体，右对齐
    _mainNumberLabel = lv_label_create(displayPanel);
    lv_label_set_text(_mainNumberLabel, "0");
    lv_obj_set_style_text_align(_mainNumberLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_size(_mainNumberLabel, _width - 20, 28);
    lv_obj_align(_mainNumberLabel, LV_ALIGN_BOTTOM_RIGHT, -10, -2);
    
    // 表达式显示 - 小字体，右对齐，在主数字上方
    _expressionLabel = lv_label_create(displayPanel);
    lv_label_set_text(_expressionLabel, "");
    lv_obj_set_style_text_align(_expressionLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_size(_expressionLabel, _width - 20, 16);
    lv_obj_align(_expressionLabel, LV_ALIGN_TOP_RIGHT, -10, 4);
    
    // ================ 中部功能区域 ================
    // 创建功能栏 (高度20px)
    lv_obj_t* statusBar = lv_obj_create(_mainContainer);
    lv_obj_set_size(statusBar, _width, 20);
    lv_obj_align_to(statusBar, displayPanel, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    lv_obj_set_style_bg_color(statusBar, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_border_width(statusBar, 0, 0);
    lv_obj_set_style_radius(statusBar, 4, 0);
    lv_obj_clear_flag(statusBar, LV_OBJ_FLAG_SCROLLABLE);
    
    // 状态指示器 - 左侧
    _statusLabel = lv_label_create(statusBar);
    lv_label_set_text(_statusLabel, "Ready");
    lv_obj_set_style_text_align(_statusLabel, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(_statusLabel, LV_ALIGN_LEFT_MID, 8, 0);
    
    // 模式指示器 - 中央
    _modeLabel = lv_label_create(statusBar);
    lv_label_set_text(_modeLabel, "Basic Mode");
    lv_obj_set_style_text_align(_modeLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(_modeLabel, LV_ALIGN_CENTER, 0, 0);
    
    // 电池指示器占位 - 右侧
    lv_obj_t* batteryLabel = lv_label_create(statusBar);
    lv_label_set_text(batteryLabel, "🔋100%");
    lv_obj_set_style_text_align(batteryLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(batteryLabel, LV_ALIGN_RIGHT_MID, -8, 0);
    
    // ================ 下部内容区域 ================
    // 创建内容面板 (剩余空间)
    lv_obj_t* contentPanel = lv_obj_create(_mainContainer);
    lv_obj_set_size(contentPanel, _width, 50);
    lv_obj_align_to(contentPanel, statusBar, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    lv_obj_set_style_bg_color(contentPanel, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(contentPanel, 1, 0);
    lv_obj_set_style_border_color(contentPanel, lv_color_hex(0x303030), 0);
    lv_obj_set_style_radius(contentPanel, 8, 0);
    lv_obj_clear_flag(contentPanel, LV_OBJ_FLAG_SCROLLABLE);
    
    // ================ 左侧：历史记录 ================
    _historyContainer = lv_obj_create(contentPanel);
    lv_obj_set_size(_historyContainer, 160, 46);
    lv_obj_align(_historyContainer, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_set_style_bg_color(_historyContainer, lv_color_hex(0x151515), 0);
    lv_obj_set_style_border_width(_historyContainer, 1, 0);
    lv_obj_set_style_border_color(_historyContainer, lv_color_hex(0x404040), 0);
    lv_obj_set_style_radius(_historyContainer, 6, 0);
    lv_obj_clear_flag(_historyContainer, LV_OBJ_FLAG_SCROLLABLE);
    
    // 历史记录标题
    lv_obj_t* historyTitle = lv_label_create(_historyContainer);
    lv_label_set_text(historyTitle, "History");
    lv_obj_set_style_text_color(historyTitle, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(historyTitle, LV_FONT_DEFAULT, 0);
    lv_obj_align(historyTitle, LV_ALIGN_TOP_LEFT, 4, 2);
    
    // ================ 中部：按键布局预览 ================
    lv_obj_t* keypadPreview = lv_obj_create(contentPanel);
    lv_obj_set_size(keypadPreview, 160, 46);
    lv_obj_align(keypadPreview, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(keypadPreview, lv_color_hex(0x151515), 0);
    lv_obj_set_style_border_width(keypadPreview, 1, 0);
    lv_obj_set_style_border_color(keypadPreview, lv_color_hex(0x404040), 0);
    lv_obj_set_style_radius(keypadPreview, 6, 0);
    lv_obj_clear_flag(keypadPreview, LV_OBJ_FLAG_SCROLLABLE);
    
    // 按键布局标题
    lv_obj_t* keypadTitle = lv_label_create(keypadPreview);
    lv_label_set_text(keypadTitle, "Keypad (22 keys)");
    lv_obj_set_style_text_color(keypadTitle, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(keypadTitle, LV_FONT_DEFAULT, 0);
    lv_obj_align(keypadTitle, LV_ALIGN_TOP_LEFT, 4, 2);
    
    // 按键网格预览
    const char* keyLabels[] = {"C", "⌫", "%", "÷", "7", "8", "9", "×", "4", "5", "6", "-", "1", "2", "3", "+", "0", ".", "="};
    for(int i = 0; i < 19; i++) {
        lv_obj_t* miniKey = lv_obj_create(keypadPreview);
        lv_obj_set_size(miniKey, 12, 8);
        lv_obj_set_pos(miniKey, 8 + (i % 5) * 14, 16 + (i / 5) * 10);
        lv_obj_set_style_bg_color(miniKey, lv_color_hex(0x333333), 0);
        lv_obj_set_style_border_width(miniKey, 0, 0);
        lv_obj_set_style_radius(miniKey, 2, 0);
        lv_obj_clear_flag(miniKey, LV_OBJ_FLAG_SCROLLABLE);
        
        // 迷你按键标签
        lv_obj_t* miniLabel = lv_label_create(miniKey);
        lv_label_set_text(miniLabel, keyLabels[i]);
        lv_obj_set_style_text_color(miniLabel, lv_color_hex(0xCCCCCC), 0);
        lv_obj_center(miniLabel);
    }
    
    // ================ 右侧：单位显示/系统信息 ================
    _unitPanel = lv_obj_create(contentPanel);
    lv_obj_set_size(_unitPanel, 154, 46);
    lv_obj_align(_unitPanel, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_set_style_bg_color(_unitPanel, lv_color_hex(0x151515), 0);
    lv_obj_set_style_border_width(_unitPanel, 1, 0);
    lv_obj_set_style_border_color(_unitPanel, lv_color_hex(0x404040), 0);
    lv_obj_set_style_radius(_unitPanel, 6, 0);
    lv_obj_clear_flag(_unitPanel, LV_OBJ_FLAG_SCROLLABLE);
    
    // 单位显示标题
    lv_obj_t* unitTitle = lv_label_create(_unitPanel);
    lv_label_set_text(unitTitle, "Unit Display");
    lv_obj_set_style_text_color(unitTitle, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(unitTitle, LV_FONT_DEFAULT, 0);
    lv_obj_align(unitTitle, LV_ALIGN_TOP_LEFT, 4, 2);
    
    // ================ 错误提示面板 ================
    _errorPanel = lv_obj_create(_mainContainer);
    lv_obj_set_size(_errorPanel, _width - 40, 40);
    lv_obj_align(_errorPanel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(_errorPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(_errorPanel, lv_color_hex(0xFF4444), 0);
    lv_obj_set_style_border_width(_errorPanel, 2, 0);
    lv_obj_set_style_border_color(_errorPanel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(_errorPanel, 8, 0);
    lv_obj_set_style_shadow_width(_errorPanel, 10, 0);
    lv_obj_set_style_shadow_color(_errorPanel, lv_color_hex(0xFF0000), 0);
    
    LVGL_LOG_D("现代化计算器UI组件创建完成");
}

void LVGLDisplay::initStyles() {
    LVGL_LOG_D("初始化现代化UI样式...");
    
    // 初始化主容器样式 - 深色主题
    lv_style_init(&_mainStyle);
    lv_style_set_bg_color(&_mainStyle, lv_color_hex(0x0A0A0A));  // 深黑色
    lv_style_set_text_color(&_mainStyle, lv_color_white());
    lv_style_set_border_width(&_mainStyle, 0);
    lv_style_set_pad_all(&_mainStyle, 0);
    lv_style_set_radius(&_mainStyle, 0);
    
    // 初始化主数字样式 - 大字体，高亮度
    lv_style_init(&_numberStyle);
    lv_style_set_text_color(&_numberStyle, lv_color_hex(0xFFFFFF));  // 纯白色
    lv_style_set_text_font(&_numberStyle, LV_FONT_DEFAULT);  // 暂时使用默认字体
    
    // 初始化表达式样式 - 中等字体，较淡颜色
    lv_style_init(&_expressionStyle);
    lv_style_set_text_color(&_expressionStyle, lv_color_hex(0xAAAAA));  // 浅灰色
    lv_style_set_text_font(&_expressionStyle, LV_FONT_DEFAULT);  // 暂时使用默认字体
    
    // 初始化状态样式 - 绿色指示器
    lv_style_init(&_statusStyle);
    lv_style_set_text_color(&_statusStyle, lv_color_hex(0x00DD00));  // 明亮绿色
    lv_style_set_text_font(&_statusStyle, LV_FONT_DEFAULT);   // 暂时使用默认字体
    
    // 初始化错误样式 - 红色警告
    lv_style_init(&_errorStyle);
    lv_style_set_text_color(&_errorStyle, lv_color_white());
    lv_style_set_text_font(&_errorStyle, LV_FONT_DEFAULT);    // 暂时使用默认字体
    lv_style_set_bg_color(&_errorStyle, lv_color_hex(0xFF4444));
    lv_style_set_radius(&_errorStyle, 8);
    lv_style_set_border_width(&_errorStyle, 2);
    lv_style_set_border_color(&_errorStyle, lv_color_hex(0xFF8888));
    
    // 初始化单位样式 - 金色数字
    lv_style_init(&_unitStyle);
    lv_style_set_text_color(&_unitStyle, lv_color_hex(0xFFCC00));  // 金黄色
    lv_style_set_text_font(&_unitStyle, LV_FONT_DEFAULT);     // 暂时使用默认字体
    
    LVGL_LOG_D("现代化UI样式初始化完成");
}

void LVGLDisplay::applyTheme() {
    // 应用主样式到容器和屏幕
    lv_obj_add_style(_mainContainer, &_mainStyle, 0);
    lv_obj_add_style(_screen, &_mainStyle, 0);
    
    // 应用样式到各个组件
    if (_mainNumberLabel) {
        lv_obj_add_style(_mainNumberLabel, &_numberStyle, 0);
    }
    if (_expressionLabel) {
        lv_obj_add_style(_expressionLabel, &_expressionStyle, 0);
    }
    if (_statusLabel) {
        lv_obj_add_style(_statusLabel, &_statusStyle, 0);
    }
    if (_modeLabel) {
        lv_obj_add_style(_modeLabel, &_statusStyle, 0);
    }
    
    LVGL_LOG_D("现代化主题应用完成");
}

void LVGLDisplay::layoutUI() {
    LVGL_LOG_D("现代化UI布局已在createUI中完成");
    // 新的UI布局已经在createUI()函数中定义，无需重复设置
}

void LVGLDisplay::clear() {
    if (_mainNumberLabel) {
        lv_label_set_text(_mainNumberLabel, "0");
    }
    if (_expressionLabel) {
        lv_label_set_text(_expressionLabel, "");
    }
    if (_historyContainer) {
        lv_obj_clean(_historyContainer);
    }
    hideErrorPanel();
    
    LVGL_LOG_D("显示已清除");
}

void LVGLDisplay::updateDisplay(const String& number, 
                               const String& expression,
                               const std::vector<CalculationHistory>& history,
                               CalculatorState state) {
    updateMainNumber(number);
    updateExpression(expression);
    updateHistory(history);
    updateStatusBar(state);
    
    // 如果启用了单位显示，更新单位标识
    if (_unitDisplay.enabled && !number.isEmpty() && number != "0") {
        updateUnitLabels(number);
    } else if (_unitPanel) {
        lv_obj_add_flag(_unitPanel, LV_OBJ_FLAG_HIDDEN);
    }
}

void LVGLDisplay::updateMainNumber(const String& number) {
    if (!_mainNumberLabel) return;
    
    String formatted = formatDisplayNumber(number);
    lv_label_set_text(_mainNumberLabel, formatted.c_str());
    
    // 强制刷新显示
    lv_obj_invalidate(_mainNumberLabel);
    lv_obj_invalidate(_screen);  // 强制整个屏幕重绘
    
    // 立即处理重绘请求
    lv_refr_now(_display);
    
    Serial.printf("LVGL: 主数字更新为: %s (强制重绘)\n", formatted.c_str());
    LVGL_LOG_V("主数字更新: %s", formatted.c_str());
}

void LVGLDisplay::updateExpression(const String& expression) {
    if (!_expressionLabel) return;
    
    lv_label_set_text(_expressionLabel, expression.c_str());
    
    LVGL_LOG_V("表达式更新: %s", expression.c_str());
}

void LVGLDisplay::updateHistory(const std::vector<CalculationHistory>& history) {
    if (!_historyContainer) return;
    
    // 清空现有历史内容（保留标题）
    lv_obj_t* child = lv_obj_get_child(_historyContainer, 0);
    while(lv_obj_get_child(_historyContainer, 1) != NULL) {
        lv_obj_del(lv_obj_get_child(_historyContainer, 1));
    }
    
    // 重新创建标题（如果被意外删除）
    if (lv_obj_get_child_cnt(_historyContainer) == 0) {
        lv_obj_t* historyTitle = lv_label_create(_historyContainer);
        lv_label_set_text(historyTitle, "History");
        lv_obj_set_style_text_color(historyTitle, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(historyTitle, LV_FONT_DEFAULT, 0);
        lv_obj_align(historyTitle, LV_ALIGN_TOP_LEFT, 4, 2);
    }
    
    // 显示最近的2-3条历史记录（适应新布局）
    int count = 0;
    int startY = 16;  // 在标题下方开始
    for (auto it = history.rbegin(); it != history.rend() && count < 2; ++it, ++count) {
        lv_obj_t* historyLabel = lv_label_create(_historyContainer);
        
        // 缩短表达式显示，适应更小的区域
        String historyText = it->expression + "=" + it->result;
        if (historyText.length() > 18) {
            historyText = historyText.substring(0, 15) + "...";
        }
        lv_label_set_text(historyLabel, historyText.c_str());
        
        lv_obj_set_style_text_font(historyLabel, LV_FONT_DEFAULT, 0);
        lv_obj_set_style_text_color(historyLabel, lv_color_hex(0x666666), 0);
        
        lv_obj_set_size(historyLabel, 150, 12);
        lv_obj_align(historyLabel, LV_ALIGN_TOP_LEFT, 4, startY + count * 14);
    }
    
    LVGL_LOG_V("历史记录更新: %d条", count);
}

void LVGLDisplay::updateStatusBar(CalculatorState state) {
    if (!_statusLabel) return;
    
    String statusText = getStateText(state);
    lv_label_set_text(_statusLabel, statusText.c_str());
    
    LVGL_LOG_V("状态更新: %s", statusText.c_str());
}

void LVGLDisplay::updateUnitLabels(const String& number) {
    if (!_unitPanel) return;
    
    // 清空单位面板内容（保留标题）
    lv_obj_t* child = lv_obj_get_child(_unitPanel, 0);
    while(lv_obj_get_child(_unitPanel, 1) != NULL) {
        lv_obj_del(lv_obj_get_child(_unitPanel, 1));
    }
    
    // 重新创建标题（如果被意外删除）
    if (lv_obj_get_child_cnt(_unitPanel) == 0) {
        lv_obj_t* unitTitle = lv_label_create(_unitPanel);
        lv_label_set_text(unitTitle, "Unit Display");
        lv_obj_set_style_text_color(unitTitle, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(unitTitle, LV_FONT_DEFAULT, 0);
        lv_obj_align(unitTitle, LV_ALIGN_TOP_LEFT, 4, 2);
    }
    
    // 如果启用单位显示且数字不为零
    if (_unitDisplay.enabled && !number.isEmpty() && number != "0") {
        double value = number.toDouble();
        if (value != 0) {
            // 创建单位标签
            int yOffset = 16;
            int displayCount = 0;
            
            for (int i = _unitDisplay.unitValues.size() - 1; i >= 0 && displayCount < 2; i--) {
                uint32_t unitValue = _unitDisplay.unitValues[i];
                int unitCount = (int)(value / unitValue);
                value = fmod(value, unitValue);
                
                if (unitCount > 0 && i < _unitDisplay.unitLabels.size()) {
                    lv_obj_t* unitLabel = lv_label_create(_unitPanel);
                    
                    String unitText = String(unitCount) + " " + _unitDisplay.unitLabels[i];
                    lv_label_set_text(unitLabel, unitText.c_str());
                    
                    lv_obj_add_style(unitLabel, &_unitStyle, 0);
                    lv_obj_set_size(unitLabel, 145, 12);
                    lv_obj_align(unitLabel, LV_ALIGN_TOP_LEFT, 4, yOffset);
                    
                    yOffset += 14;
                    displayCount++;
                }
            }
        }
    } else {
        // 显示系统信息
        lv_obj_t* infoLabel = lv_label_create(_unitPanel);
        lv_label_set_text(infoLabel, "480x128 LVGL");
        lv_obj_set_style_text_color(infoLabel, lv_color_hex(0x666666), 0);
        lv_obj_set_size(infoLabel, 145, 12);
        lv_obj_align(infoLabel, LV_ALIGN_TOP_LEFT, 4, 16);
        
        lv_obj_t* modeLabel = lv_label_create(_unitPanel);
        lv_label_set_text(modeLabel, "ESP32-S3");
        lv_obj_set_style_text_color(modeLabel, lv_color_hex(0x666666), 0);
        lv_obj_set_size(modeLabel, 145, 12);
        lv_obj_align(modeLabel, LV_ALIGN_TOP_LEFT, 4, 30);
    }
    
    LVGL_LOG_V("单位/信息面板更新完成");
}

void LVGLDisplay::showError(CalculatorError error, const String& message) {
    if (!_errorPanel) return;
    
    // 清空错误面板
    lv_obj_clean(_errorPanel);
    
    // 创建错误标签
    lv_obj_t* errorLabel = lv_label_create(_errorPanel);
    String errorText = getErrorText(error);
    if (!message.isEmpty()) {
        errorText += ": " + message;
    }
    
    lv_label_set_text(errorLabel, errorText.c_str());
    lv_obj_add_style(errorLabel, &_errorStyle, 0);
    lv_obj_center(errorLabel);
    
    // 显示错误面板
    lv_obj_clear_flag(_errorPanel, LV_OBJ_FLAG_HIDDEN);
    
    // 播放错误动画
    showErrorAnimation();
    
    LVGL_LOG_W("显示错误: %s", errorText.c_str());
}

void LVGLDisplay::showErrorAnimation() {
    if (!_errorPanel) return;
    
    // 创建淡入动画
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, _errorPanel);
    lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&anim, 300);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_style_bg_opa);
    lv_anim_start(&anim);
}

void LVGLDisplay::hideErrorPanel() {
    if (_errorPanel) {
        lv_obj_add_flag(_errorPanel, LV_OBJ_FLAG_HIDDEN);
    }
}

void LVGLDisplay::showStatus(const String& message) {
    if (_statusLabel) {
        lv_label_set_text(_statusLabel, message.c_str());
    }
    
    LVGL_LOG_I("状态消息: %s", message.c_str());
}

void LVGLDisplay::setTheme(const DisplayTheme& theme) {
    _theme = theme;
    applyTheme();
    
    LVGL_LOG_I("主题已更新");
}

void LVGLDisplay::setNumberFormat(const NumberFormat& format) {
    _numberFormat = format;
    
    LVGL_LOG_I("数字格式已更新");
}

void LVGLDisplay::setUnitDisplay(const UnitDisplay& unitDisplay) {
    _unitDisplay = unitDisplay;
    
    LVGL_LOG_I("单位显示配置已更新");
}

void LVGLDisplay::update() {
    static unsigned long lastDebug = 0;
    static int updateCount = 0;
    updateCount++;
    
    lv_timer_handler();
    
    // 每5秒输出一次调试信息
    if (millis() - lastDebug > 5000) {
        lastDebug = millis();
        Serial.printf("LVGL: update()已调用 %d 次\n", updateCount);
    }
}



void LVGLDisplay::showTestGrid() {
    LVGL_LOG_I("showTestGrid() 被调用 - 显示现代化计算器测试界面");
    Serial.println("DEBUG: showTestGrid() 函数开始执行");
    
    if (!_screen) {
        LVGL_LOG_E("错误: _screen 对象为空");
        Serial.println("DEBUG: _screen 对象为空，退出函数");
        return;
    }
    
    // 清空屏幕并重新创建现代化UI测试界面
    lv_obj_clean(_screen);
    lv_scr_load(_screen);
    
    // 设置屏幕背景为深灰色
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    LVGL_LOG_I("显示现代化计算器UI测试 - 分辨率: %dx%d", _width, 126);
    
    // ================ 创建现代化计算器UI演示 ================
    // 顶部显示面板
    lv_obj_t* displayPanel = lv_obj_create(_screen);
    lv_obj_set_size(displayPanel, _width, 50);
    lv_obj_align(displayPanel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(displayPanel, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_border_width(displayPanel, 2, 0);
    lv_obj_set_style_border_color(displayPanel, lv_color_hex(0x00FF00), 0);  // 绿色边框
    lv_obj_set_style_radius(displayPanel, 8, 0);
    lv_obj_clear_flag(displayPanel, LV_OBJ_FLAG_SCROLLABLE);
    
    // 测试数字显示
    lv_obj_t* testNumber = lv_label_create(displayPanel);
    lv_label_set_text(testNumber, "123,456.789");
    lv_obj_set_style_text_color(testNumber, lv_color_white(), 0);
    lv_obj_set_style_text_align(testNumber, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_size(testNumber, _width - 20, 28);
    lv_obj_align(testNumber, LV_ALIGN_BOTTOM_RIGHT, -10, -2);
    
    // 测试表达式
    lv_obj_t* testExpression = lv_label_create(displayPanel);
    lv_label_set_text(testExpression, "123 + 456 * 0.789 =");
    lv_obj_set_style_text_color(testExpression, lv_color_hex(0xAAAA), 0);
    lv_obj_set_style_text_align(testExpression, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_size(testExpression, _width - 20, 16);
    lv_obj_align(testExpression, LV_ALIGN_TOP_RIGHT, -10, 4);
    
    // 中部状态栏
    lv_obj_t* statusBar = lv_obj_create(_screen);
    lv_obj_set_size(statusBar, _width, 20);
    lv_obj_align_to(statusBar, displayPanel, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    lv_obj_set_style_bg_color(statusBar, lv_color_hex(0x404040), 0);
    lv_obj_set_style_border_width(statusBar, 1, 0);
    lv_obj_set_style_border_color(statusBar, lv_color_hex(0xFF8800), 0);  // 橙色边框
    lv_obj_set_style_radius(statusBar, 4, 0);
    lv_obj_clear_flag(statusBar, LV_OBJ_FLAG_SCROLLABLE);
    
    // 状态指示器
    lv_obj_t* statusLabel = lv_label_create(statusBar);
    lv_label_set_text(statusLabel, "Testing");
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x00DD00), 0);
    lv_obj_set_style_text_font(statusLabel, LV_FONT_DEFAULT, 0);
    lv_obj_align(statusLabel, LV_ALIGN_LEFT_MID, 8, 0);
    
    lv_obj_t* modeLabel = lv_label_create(statusBar);
    lv_label_set_text(modeLabel, "LVGL UI Test");
    lv_obj_set_style_text_color(modeLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(modeLabel, LV_FONT_DEFAULT, 0);
    lv_obj_align(modeLabel, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_t* batteryLabel = lv_label_create(statusBar);
    lv_label_set_text(batteryLabel, "🔋100%");
    lv_obj_set_style_text_color(batteryLabel, lv_color_hex(0x00DD00), 0);
    lv_obj_align(batteryLabel, LV_ALIGN_RIGHT_MID, -8, 0);
    
    // 下部内容区域
    lv_obj_t* contentPanel = lv_obj_create(_screen);
    lv_obj_set_size(contentPanel, _width, 50);
    lv_obj_align_to(contentPanel, statusBar, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    lv_obj_set_style_bg_color(contentPanel, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(contentPanel, 2, 0);
    lv_obj_set_style_border_color(contentPanel, lv_color_hex(0x0088FF), 0);  // 蓝色边框
    lv_obj_set_style_radius(contentPanel, 8, 0);
    lv_obj_clear_flag(contentPanel, LV_OBJ_FLAG_SCROLLABLE);
    
    // 左侧：测试历史记录
    lv_obj_t* historyTest = lv_obj_create(contentPanel);
    lv_obj_set_size(historyTest, 160, 46);
    lv_obj_align(historyTest, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_set_style_bg_color(historyTest, lv_color_hex(0x151515), 0);
    lv_obj_set_style_border_width(historyTest, 1, 0);
    lv_obj_set_style_border_color(historyTest, lv_color_hex(0xFF0088), 0);  // 紫红色边框
    lv_obj_set_style_radius(historyTest, 6, 0);
    lv_obj_clear_flag(historyTest, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* historyTitle = lv_label_create(historyTest);
    lv_label_set_text(historyTitle, "History Test");
    lv_obj_set_style_text_color(historyTitle, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(historyTitle, LV_FONT_DEFAULT, 0);
    lv_obj_align(historyTitle, LV_ALIGN_TOP_LEFT, 4, 2);
    
    lv_obj_t* history1 = lv_label_create(historyTest);
    lv_label_set_text(history1, "1+2=3");
    lv_obj_set_style_text_color(history1, lv_color_hex(0x666666), 0);
    lv_obj_align(history1, LV_ALIGN_TOP_LEFT, 4, 16);
    
    lv_obj_t* history2 = lv_label_create(historyTest);
    lv_label_set_text(history2, "4*5=20");
    lv_obj_set_style_text_color(history2, lv_color_hex(0x666666), 0);
    lv_obj_align(history2, LV_ALIGN_TOP_LEFT, 4, 30);
    
    // 中部：按键布局测试
    lv_obj_t* keypadTest = lv_obj_create(contentPanel);
    lv_obj_set_size(keypadTest, 160, 46);
    lv_obj_align(keypadTest, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(keypadTest, lv_color_hex(0x151515), 0);
    lv_obj_set_style_border_width(keypadTest, 1, 0);
    lv_obj_set_style_border_color(keypadTest, lv_color_hex(0xFFFF00), 0);  // 黄色边框
    lv_obj_set_style_radius(keypadTest, 6, 0);
    lv_obj_clear_flag(keypadTest, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* keypadTitle = lv_label_create(keypadTest);
    lv_label_set_text(keypadTitle, "Keypad Test");
    lv_obj_set_style_text_color(keypadTitle, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(keypadTitle, LV_FONT_DEFAULT, 0);
    lv_obj_align(keypadTitle, LV_ALIGN_TOP_LEFT, 4, 2);
    
    // 按键测试网格 - 彩色版本
    uint32_t keyColors[] = {0xFF4444, 0x44FF44, 0x4444FF, 0xFFFF44, 0xFF44FF};
    for(int i = 0; i < 15; i++) {
        lv_obj_t* miniKey = lv_obj_create(keypadTest);
        lv_obj_set_size(miniKey, 12, 8);
        lv_obj_set_pos(miniKey, 8 + (i % 5) * 14, 16 + (i / 5) * 10);
        lv_obj_set_style_bg_color(miniKey, lv_color_hex(keyColors[i % 5]), 0);
        lv_obj_set_style_border_width(miniKey, 0, 0);
        lv_obj_set_style_radius(miniKey, 2, 0);
        lv_obj_clear_flag(miniKey, LV_OBJ_FLAG_SCROLLABLE);
    }
    
    // 右侧：信息测试
    lv_obj_t* infoTest = lv_obj_create(contentPanel);
    lv_obj_set_size(infoTest, 154, 46);
    lv_obj_align(infoTest, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_set_style_bg_color(infoTest, lv_color_hex(0x151515), 0);
    lv_obj_set_style_border_width(infoTest, 1, 0);
    lv_obj_set_style_border_color(infoTest, lv_color_hex(0x00FFFF), 0);  // 青色边框
    lv_obj_set_style_radius(infoTest, 6, 0);
    lv_obj_clear_flag(infoTest, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* infoTitle = lv_label_create(infoTest);
    lv_label_set_text(infoTitle, "System Info");
    lv_obj_set_style_text_color(infoTitle, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(infoTitle, LV_FONT_DEFAULT, 0);
    lv_obj_align(infoTitle, LV_ALIGN_TOP_LEFT, 4, 2);
    
    lv_obj_t* info1 = lv_label_create(infoTest);
    lv_label_set_text(info1, "480x126 LVGL");
    lv_obj_set_style_text_color(info1, lv_color_hex(0xFFCC00), 0);
    lv_obj_align(info1, LV_ALIGN_TOP_LEFT, 4, 16);
    
    lv_obj_t* info2 = lv_label_create(infoTest);
    lv_label_set_text(info2, "ESP32-S3");
    lv_obj_set_style_text_color(info2, lv_color_hex(0xFFCC00), 0);
    lv_obj_align(info2, LV_ALIGN_TOP_LEFT, 4, 30);
    
    // 强制刷新显示
    lv_obj_invalidate(_screen);
    lv_refr_now(_display);
    lv_timer_handler();
    
    LVGL_LOG_I("现代化计算器UI测试界面显示完成");
}

void LVGLDisplay::clearTestGrid() {
    if (!_screen) return;
    
    // 清空屏幕并重新创建正常UI
    lv_obj_clean(_screen);
    createUI();
    initStyles();
    applyTheme();
    layoutUI();
    
    LVGL_LOG_I("测试网格已清除，恢复正常UI");
}

// 静态回调函数
void LVGLDisplay::displayFlushCb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    static int flush_count = 0;
    flush_count++;
    
    Serial.printf("DEBUG: displayFlushCb 被调用 #%d\n", flush_count);
    
    Arduino_GFX* gfx = (Arduino_GFX*)disp_drv->user_data;
    if (!gfx) {
        Serial.println("DEBUG: displayFlushCb - GFX对象为空");
        lv_disp_flush_ready(disp_drv);
        return;
    }
    
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    // 边界检查，限制到安全的126行
    if (area->x1 < 0 || area->y1 < 0 || area->x2 >= 480 || area->y2 >= 126) {
        Serial.printf("WARNING: displayFlushCb 边界超出 - 区域: (%d,%d)-(%d,%d)\n", 
                     area->x1, area->y1, area->x2, area->y2);
        // 修正边界
        int16_t x1 = max((int16_t)0, (int16_t)area->x1);
        int16_t y1 = max((int16_t)0, (int16_t)area->y1);
        int16_t x2 = min((int16_t)479, (int16_t)area->x2);
        int16_t y2 = min((int16_t)125, (int16_t)area->y2);  // 限制到125行
        
        if (x1 > x2 || y1 > y2) {
            Serial.println("ERROR: 修正后边界无效，跳过绘制");
            lv_disp_flush_ready(disp_drv);
            return;
        }
        
        w = x2 - x1 + 1;
        h = y2 - y1 + 1;
        
        // 使用修正后的坐标
        gfx->draw16bitRGBBitmap(x1, y1, (uint16_t*)color_p, w, h);
        lv_disp_flush_ready(disp_drv);
        return;
    }
    
    // 输出调试信息
    if (flush_count <= 5 || flush_count % 10 == 1) {
        Serial.printf("DEBUG: displayFlushCb #%d - 区域: (%d,%d)-(%d,%d) 尺寸: %dx%d\n", 
                     flush_count, area->x1, area->y1, area->x2, area->y2, w, h);
    }
    
#if (LV_COLOR_16_SWAP != 0)
    // 如果需要颜色字节交换
    for (uint32_t i = 0; i < w * h; i++) {
        ((uint16_t*)color_p)[i] = __builtin_bswap16(((uint16_t*)color_p)[i]);
    }
#endif

    // 使用Arduino_GFX的高效绘制方法
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)color_p, w, h);
    
    lv_disp_flush_ready(disp_drv);
}

// 辅助函数实现
lv_color_t LVGLDisplay::convertColor(uint16_t color) {
    return RGB565_TO_LVGL(color);
}

String LVGLDisplay::formatDisplayNumber(const String& number) {
    if (!_numberFormat.useThousandsSeparator) {
        return number;
    }
    
    // 简单的千位分隔符实现
    String result = number;
    int dotPos = result.indexOf('.');
    int insertPos = (dotPos != -1) ? dotPos : result.length();
    
    for (int i = insertPos - 3; i > 0; i -= 3) {
        result = result.substring(0, i) + _numberFormat.thousandsSeparator + result.substring(i);
    }
    
    return result;
}

String LVGLDisplay::getStateText(CalculatorState state) {
    switch (state) {
        case CalculatorState::INPUT_NUMBER:
            return "输入";
        case CalculatorState::INPUT_OPERATOR:
            return "运算";
        case CalculatorState::DISPLAY_RESULT:
            return "结果";
        case CalculatorState::ERROR:
            return "错误";
        case CalculatorState::WAITING:
            return "等待";
        default:
            return "就绪";
    }
}

String LVGLDisplay::getErrorText(CalculatorError error) {
    switch (error) {
        case CalculatorError::DIVISION_BY_ZERO:
            return "除零错误";
        case CalculatorError::INVALID_OPERATION:
            return "无效操作";
        case CalculatorError::OVERFLOW:
            return "数字过大";
        case CalculatorError::UNDERFLOW:
            return "数字过小";
        case CalculatorError::SYNTAX_ERROR:
            return "语法错误";
        case CalculatorError::MEMORY_ERROR:
            return "内存错误";
        case CalculatorError::NONE:
            return "无错误";
        default:
            return "未知错误";
    }
}

DisplayTheme LVGLDisplay::getDefaultTheme() {
    DisplayTheme theme;
    theme.backgroundColor = 0x0000;      // 黑色
    theme.textColor = 0xFFFF;            // 白色
    theme.expressionColor = 0xCCCC;      // 浅灰
    theme.resultColor = 0xFFFF;          // 白色
    theme.errorColor = 0xF800;           // 红色
    theme.statusColor = 0x07E0;          // 绿色
    theme.borderColor = 0x8410;          // 深灰
    theme.mainFontSize = 4;
    theme.expressionFontSize = 2;
    theme.statusFontSize = 1;
    theme.padding = 5;
    theme.lineSpacing = 2;
    theme.showBorders = false;
    return theme;
}

NumberFormat LVGLDisplay::getDefaultNumberFormat() {
    NumberFormat format;
    format.useThousandsSeparator = true;
    format.showTrailingZeros = false;
    format.decimalPlaces = 2;
    format.scientificNotation = false;
    format.showSign = false;
    format.currency = "¥";
    format.thousandsSeparator = ",";
    format.decimalSeparator = ".";
    return format;
}

// LVGL日志回调
void lvglLogCb(lv_log_level_t level, const char* file, uint32_t line, const char* func, const char* dsc) {
    const char* levelStr;
    switch (level) {
        case LV_LOG_LEVEL_ERROR:
            levelStr = "ERROR";
            break;
        case LV_LOG_LEVEL_WARN:
            levelStr = "WARN";
            break;
        case LV_LOG_LEVEL_INFO:
            levelStr = "INFO";
            break;
        case LV_LOG_LEVEL_TRACE:
            levelStr = "TRACE";
            break;
        default:
            levelStr = "DEBUG";
            break;
    }
    
    LOG_I(TAG_LVGL, "[%s] %s:%d %s() %s", levelStr, file, line, func, dsc);
}