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
    
    // 设置0度旋转（正常方向）
    setRotation(0);
    
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
    // 旋转设置将在初始化完成后动态设置
    
    // 注册显示驱动
    _display = lv_disp_drv_register(&disp_drv);
    if (!_display) {
        LVGL_LOG_E("显示驱动注册失败");
        return false;
    }
    
    LVGL_LOG_I("LVGL核心初始化完成 (缓冲区: %d bytes)", bufSize * sizeof(lv_color_t) * 2);
    return true;
}

void LVGLDisplay::createUI() {
    LVGL_LOG_D("创建UI组件...");
    
    // 创建主屏幕
    _screen = lv_obj_create(NULL);
    lv_scr_load(_screen);
    
    // 创建主容器
    _mainContainer = lv_obj_create(_screen);
    lv_obj_set_size(_mainContainer, _width, _height);
    lv_obj_align(_mainContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(_mainContainer, LV_OBJ_FLAG_SCROLLABLE);
    
    // 创建主数字标签
    _mainNumberLabel = lv_label_create(_mainContainer);
    lv_label_set_text(_mainNumberLabel, "0");
    lv_obj_set_style_text_align(_mainNumberLabel, LV_TEXT_ALIGN_RIGHT, 0);
    
    // 创建表达式标签
    _expressionLabel = lv_label_create(_mainContainer);
    lv_label_set_text(_expressionLabel, "");
    lv_obj_set_style_text_align(_expressionLabel, LV_TEXT_ALIGN_RIGHT, 0);
    
    // 创建状态标签
    _statusLabel = lv_label_create(_mainContainer);
    lv_label_set_text(_statusLabel, "Ready");  // 暂时使用英文
    lv_obj_set_style_text_align(_statusLabel, LV_TEXT_ALIGN_LEFT, 0);
    
    // 创建模式标签
    _modeLabel = lv_label_create(_mainContainer);
    lv_label_set_text(_modeLabel, "Basic");   // 暂时使用英文
    lv_obj_set_style_text_align(_modeLabel, LV_TEXT_ALIGN_CENTER, 0);
    
    // 创建历史记录容器
    _historyContainer = lv_obj_create(_mainContainer);
    lv_obj_set_size(_historyContainer, 120, 80);
    lv_obj_set_style_bg_opa(_historyContainer, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(_historyContainer, LV_OBJ_FLAG_SCROLLABLE);
    
    // 创建错误面板（初始隐藏）
    _errorPanel = lv_obj_create(_mainContainer);
    lv_obj_set_size(_errorPanel, _width - 20, 60);
    lv_obj_add_flag(_errorPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(_errorPanel, lv_color_hex(0xFF4444), 0);
    lv_obj_set_style_border_width(_errorPanel, 2, 0);
    lv_obj_set_style_border_color(_errorPanel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(_errorPanel, 5, 0);
    
    // 创建单位显示面板（初始隐藏）
    _unitPanel = lv_obj_create(_mainContainer);
    lv_obj_set_size(_unitPanel, 100, 80);
    lv_obj_add_flag(_unitPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_opa(_unitPanel, LV_OPA_80, 0);
    
    LVGL_LOG_D("UI组件创建完成");
}

void LVGLDisplay::initStyles() {
    LVGL_LOG_D("初始化UI样式...");
    
    // 初始化主样式
    lv_style_init(&_mainStyle);
    lv_style_set_bg_color(&_mainStyle, lv_color_black());
    lv_style_set_text_color(&_mainStyle, lv_color_white());
    lv_style_set_border_width(&_mainStyle, 0);
    lv_style_set_pad_all(&_mainStyle, 5);
    
    // 初始化数字样式
    lv_style_init(&_numberStyle);
    lv_style_set_text_color(&_numberStyle, lv_color_white());
    lv_style_set_text_font(&_numberStyle, LV_FONT_DEFAULT);  // 使用默认字体
    
    // 初始化表达式样式
    lv_style_init(&_expressionStyle);
    lv_style_set_text_color(&_expressionStyle, lv_color_hex(0xCCCCCC));
    lv_style_set_text_font(&_expressionStyle, LV_FONT_DEFAULT);
    
    // 初始化状态样式（暂时使用默认字体）
    lv_style_init(&_statusStyle);
    lv_style_set_text_color(&_statusStyle, lv_color_hex(0x88FF88));
    lv_style_set_text_font(&_statusStyle, LV_FONT_DEFAULT);  // 使用默认字体
    
    // 初始化错误样式
    lv_style_init(&_errorStyle);
    lv_style_set_text_color(&_errorStyle, lv_color_white());
    lv_style_set_text_font(&_errorStyle, LV_FONT_DEFAULT);  // 使用默认字体
    lv_style_set_bg_color(&_errorStyle, lv_color_hex(0xFF4444));
    
    // 初始化单位样式
    lv_style_init(&_unitStyle);
    lv_style_set_text_color(&_unitStyle, lv_color_hex(0xFFDD44));
    lv_style_set_text_font(&_unitStyle, LV_FONT_DEFAULT);  // 使用默认字体
    
    LVGL_LOG_D("UI样式初始化完成");
}

void LVGLDisplay::applyTheme() {
    // 应用主样式到容器
    lv_obj_add_style(_mainContainer, &_mainStyle, 0);
    lv_obj_add_style(_screen, &_mainStyle, 0);
    
    // 应用样式到各个组件
    lv_obj_add_style(_mainNumberLabel, &_numberStyle, 0);
    lv_obj_add_style(_expressionLabel, &_expressionStyle, 0);
    lv_obj_add_style(_statusLabel, &_statusStyle, 0);
    lv_obj_add_style(_modeLabel, &_statusStyle, 0);
}

void LVGLDisplay::layoutUI() {
    LVGL_LOG_D("布局UI组件...");
    
    // 主数字显示区域（右下角）
    lv_obj_set_size(_mainNumberLabel, _width - 140, 40);
    lv_obj_align(_mainNumberLabel, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    
    // 表达式显示区域（主数字上方）
    lv_obj_set_size(_expressionLabel, _width - 140, 25);
    lv_obj_align_to(_expressionLabel, _mainNumberLabel, LV_ALIGN_OUT_TOP_RIGHT, 0, -5);
    
    // 状态标签（左下角）
    lv_obj_set_size(_statusLabel, 120, 20);
    lv_obj_align(_statusLabel, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    
    // 模式标签（状态标签上方）
    lv_obj_set_size(_modeLabel, 120, 20);
    lv_obj_align_to(_modeLabel, _statusLabel, LV_ALIGN_OUT_TOP_LEFT, 0, -5);
    
    // 历史记录容器（左上角）
    lv_obj_align(_historyContainer, LV_ALIGN_TOP_LEFT, 10, 10);
    
    // 错误面板（居中）
    lv_obj_align(_errorPanel, LV_ALIGN_CENTER, 0, 0);
    
    // 单位面板（右上角）
    lv_obj_align(_unitPanel, LV_ALIGN_TOP_RIGHT, -10, 10);
    
    LVGL_LOG_D("UI布局完成");
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
    
    LVGL_LOG_V("主数字更新: %s", formatted.c_str());
}

void LVGLDisplay::updateExpression(const String& expression) {
    if (!_expressionLabel) return;
    
    lv_label_set_text(_expressionLabel, expression.c_str());
    
    LVGL_LOG_V("表达式更新: %s", expression.c_str());
}

void LVGLDisplay::updateHistory(const std::vector<CalculationHistory>& history) {
    if (!_historyContainer) return;
    
    // 清空现有历史
    lv_obj_clean(_historyContainer);
    
    // 显示最近的3条历史记录
    int count = 0;
    for (auto it = history.rbegin(); it != history.rend() && count < 3; ++it, ++count) {
        lv_obj_t* historyLabel = lv_label_create(_historyContainer);
        
        String historyText = it->expression + " = " + it->result;
        lv_label_set_text(historyLabel, historyText.c_str());
        
        lv_obj_set_style_text_font(historyLabel, LV_FONT_DEFAULT, 0);
        lv_obj_set_style_text_color(historyLabel, lv_color_hex(0x888888), 0);
        
        lv_obj_set_size(historyLabel, 110, 20);
        lv_obj_align(historyLabel, LV_ALIGN_TOP_LEFT, 5, 5 + count * 22);
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
    if (!_unitPanel || !_unitDisplay.enabled) return;
    
    // 清空单位面板
    lv_obj_clean(_unitPanel);
    
    // 解析数字
    double value = number.toDouble();
    if (value == 0) {
        lv_obj_add_flag(_unitPanel, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    
    // 显示单位面板
    lv_obj_clear_flag(_unitPanel, LV_OBJ_FLAG_HIDDEN);
    
    // 创建单位标签
    int yOffset = 5;
    for (int i = _unitDisplay.unitValues.size() - 1; i >= 0; i--) {
        uint32_t unitValue = _unitDisplay.unitValues[i];
        int unitCount = (int)(value / unitValue);
        value = fmod(value, unitValue);
        
        if (unitCount > 0 && i < _unitDisplay.unitLabels.size()) {
            lv_obj_t* unitLabel = lv_label_create(_unitPanel);
            
            String unitText = String(unitCount) + " " + _unitDisplay.unitLabels[i];
            lv_label_set_text(unitLabel, unitText.c_str());
            
            lv_obj_add_style(unitLabel, &_unitStyle, 0);
            lv_obj_set_size(unitLabel, 90, 15);
            lv_obj_align(unitLabel, LV_ALIGN_TOP_LEFT, 5, yOffset);
            
            yOffset += 18;
        }
    }
    
    LVGL_LOG_V("单位标识更新完成");
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
    lv_timer_handler();
}

void LVGLDisplay::setRotation(uint16_t rotation) {
    if (!_display) return;
    
    lv_disp_rot_t lvglRotation;
    switch (rotation) {
        case 0:
            lvglRotation = LV_DISP_ROT_NONE;
            break;
        case 90:
            lvglRotation = LV_DISP_ROT_90;
            break;
        case 180:
            lvglRotation = LV_DISP_ROT_180;
            break;
        case 270:
            lvglRotation = LV_DISP_ROT_270;
            break;
        default:
            LVGL_LOG_W("无效的旋转角度: %d", rotation);
            return;
    }
    
    lv_disp_set_rotation(_display, lvglRotation);
    LVGL_LOG_I("显示旋转设置为: %d度", rotation);
}

void LVGLDisplay::showTestGrid() {
    LVGL_LOG_I("showTestGrid() 被调用");
    Serial.println("DEBUG: showTestGrid() 函数开始执行");
    
    if (!_screen) {
        LVGL_LOG_E("错误: _screen 对象为空");
        Serial.println("DEBUG: _screen 对象为空，退出函数");
        return;
    }
    
    Serial.println("DEBUG: _screen 对象存在，开始清空屏幕");
    // 清空屏幕
    lv_obj_clean(_screen);
    
    // 确保这个屏幕被设置为活动屏幕
    lv_scr_load(_screen);
    Serial.println("DEBUG: 重新加载屏幕为活动屏幕");
    
    LVGL_LOG_I("显示测试网格 - 分辨率: %dx%d", _width, 126);
    Serial.printf("DEBUG: 创建测试网格，分辨率: %dx%d\n", _width, 126);
    
    // 设置屏幕背景色为黑色
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);  // 禁用屏幕滚动
    
    Serial.println("DEBUG: 设置屏幕背景为黑色");
    
    // 四个角落的彩色方块 - 使用绝对坐标真正贴边
    // 左上角 - 红色 (0,0)
    lv_obj_t* corner_tl = lv_obj_create(_screen);
    lv_obj_set_size(corner_tl, 40, 25);
    lv_obj_set_pos(corner_tl, 0, 0);  // 绝对坐标：真正的左上角
    lv_obj_set_style_bg_color(corner_tl, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(corner_tl, 0, 0);
    lv_obj_set_style_pad_all(corner_tl, 0, 0);
    lv_obj_clear_flag(corner_tl, LV_OBJ_FLAG_SCROLLABLE);
    
    // 右上角 - 绿色 (440,0)
    lv_obj_t* corner_tr = lv_obj_create(_screen);
    lv_obj_set_size(corner_tr, 40, 25);
    lv_obj_set_pos(corner_tr, 440, 0);  // 绝对坐标：440+40=480，真正的右上角
    lv_obj_set_style_bg_color(corner_tr, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_border_width(corner_tr, 0, 0);
    lv_obj_set_style_pad_all(corner_tr, 0, 0);
    lv_obj_clear_flag(corner_tr, LV_OBJ_FLAG_SCROLLABLE);
    
    // 左下角 - 蓝色 (0,101) - 调整到安全区域
    lv_obj_t* corner_bl = lv_obj_create(_screen);
    lv_obj_set_size(corner_bl, 40, 25);
    lv_obj_set_pos(corner_bl, 0, 101);  // 绝对坐标：101+25=126，避开花屏区域
    lv_obj_set_style_bg_color(corner_bl, lv_color_hex(0x0000FF), 0);
    lv_obj_set_style_border_width(corner_bl, 0, 0);
    lv_obj_set_style_pad_all(corner_bl, 0, 0);
    lv_obj_clear_flag(corner_bl, LV_OBJ_FLAG_SCROLLABLE);
    
    // 右下角 - 黄色 (440,101) - 调整到安全区域
    lv_obj_t* corner_br = lv_obj_create(_screen);
    lv_obj_set_size(corner_br, 40, 25);
    lv_obj_set_pos(corner_br, 440, 101);  // 绝对坐标：避开花屏区域
    lv_obj_set_style_bg_color(corner_br, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_border_width(corner_br, 0, 0);
    lv_obj_set_style_pad_all(corner_br, 0, 0);
    lv_obj_clear_flag(corner_br, LV_OBJ_FLAG_SCROLLABLE);
    
    Serial.println("DEBUG: 创建了四个角落的彩色方块");
    
    // 中心测试标签 - 带方向指示
    lv_obj_t* test_label = lv_label_create(_screen);
    lv_label_set_text_fmt(test_label, "Test Grid %dx%d\nTOP", _width, _height);
    lv_obj_set_style_text_color(test_label, lv_color_hex(0x000000), 0);  // 黑色文字
    lv_obj_set_style_text_opa(test_label, LV_OPA_COVER, 0);
    lv_obj_center(test_label);
    
    // 顶部指示器
    lv_obj_t* top_indicator = lv_label_create(_screen);
    lv_label_set_text(top_indicator, "▲ TOP ▲");
    lv_obj_set_style_text_color(top_indicator, lv_color_hex(0x000000), 0);
    lv_obj_align(top_indicator, LV_ALIGN_TOP_MID, 0, 8);
    
    // 底部指示器  
    lv_obj_t* bottom_indicator = lv_label_create(_screen);
    lv_label_set_text(bottom_indicator, "▼ BOTTOM ▼");
    lv_obj_set_style_text_color(bottom_indicator, lv_color_hex(0x000000), 0);
    lv_obj_align(bottom_indicator, LV_ALIGN_BOTTOM_MID, 0, -25);
    
    // 坐标标注
    lv_obj_t* coord_tl = lv_label_create(_screen);
    lv_label_set_text(coord_tl, "(0,0)");
    lv_obj_set_style_text_color(coord_tl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(coord_tl, LV_ALIGN_TOP_LEFT, 45, 5);
    
    lv_obj_t* coord_br = lv_label_create(_screen);
    lv_label_set_text_fmt(coord_br, "(479,127)");  // 显示完整分辨率
    lv_obj_set_style_text_color(coord_br, lv_color_hex(0x000000), 0);  // 黑色文字在白色背景上
    lv_obj_align(coord_br, LV_ALIGN_BOTTOM_RIGHT, -45, -2);   // 靠近底部边缘
    
    // 创建分层测试条带，找到确切的可用边界
    // 测试条1: Y=122-125 (尝试更保守的位置)
    lv_obj_t* test_bar_0 = lv_obj_create(_screen);
    lv_obj_set_size(test_bar_0, 480, 4);
    lv_obj_set_pos(test_bar_0, 0, 122);  // 绝对坐标：Y=122
    lv_obj_set_style_bg_color(test_bar_0, lv_color_hex(0xFF0000), 0);  // 红色
    lv_obj_set_style_border_width(test_bar_0, 0, 0);
    lv_obj_set_style_pad_all(test_bar_0, 0, 0);
    lv_obj_clear_flag(test_bar_0, LV_OBJ_FLAG_SCROLLABLE);
    
    // 测试条2: 绝对位置（第116-119行）
    lv_obj_t* test_bar_8 = lv_obj_create(_screen);
    lv_obj_set_size(test_bar_8, 480, 4);
    lv_obj_set_pos(test_bar_8, 0, 116);  // 绝对坐标：Y=116
    lv_obj_set_style_bg_color(test_bar_8, lv_color_hex(0x00FF00), 0);  // 绿色
    lv_obj_set_style_border_width(test_bar_8, 0, 0);
    lv_obj_set_style_pad_all(test_bar_8, 0, 0);
    lv_obj_clear_flag(test_bar_8, LV_OBJ_FLAG_SCROLLABLE);
    
    // 测试条3: 绝对位置（第108-111行）
    lv_obj_t* test_bar_16 = lv_obj_create(_screen);
    lv_obj_set_size(test_bar_16, 480, 4);
    lv_obj_set_pos(test_bar_16, 0, 108);  // 绝对坐标：Y=108
    lv_obj_set_style_bg_color(test_bar_16, lv_color_hex(0x0000FF), 0);  // 蓝色
    lv_obj_set_style_border_width(test_bar_16, 0, 0);
    lv_obj_set_style_pad_all(test_bar_16, 0, 0);
    lv_obj_clear_flag(test_bar_16, LV_OBJ_FLAG_SCROLLABLE);
    
    // 测试条4: 绝对位置（第100-103行）
    lv_obj_t* test_bar_24 = lv_obj_create(_screen);
    lv_obj_set_size(test_bar_24, 480, 4);
    lv_obj_set_pos(test_bar_24, 0, 100);  // 绝对坐标：Y=100
    lv_obj_set_style_bg_color(test_bar_24, lv_color_hex(0xFFFF00), 0);  // 黄色
    lv_obj_set_style_border_width(test_bar_24, 0, 0);
    lv_obj_set_style_pad_all(test_bar_24, 0, 0);
    lv_obj_clear_flag(test_bar_24, LV_OBJ_FLAG_SCROLLABLE);
    
    Serial.println("DEBUG: 创建了多个底部测试条带（红绿蓝黄从底到上）");
    
    Serial.println("DEBUG: 创建了中心白色文本标签");
    
    // 强制触发LVGL刷新
    Serial.println("DEBUG: 开始强制触发LVGL刷新");
    
    // 标记屏幕为需要重绘
    lv_obj_invalidate(_screen);
    Serial.println("DEBUG: 标记屏幕为需要重绘");
    
    // 标记所有子对象为需要重绘
    lv_obj_invalidate(corner_tl);
    lv_obj_invalidate(corner_tr);
    lv_obj_invalidate(corner_bl);
    lv_obj_invalidate(corner_br);
    lv_obj_invalidate(test_label);
    lv_obj_invalidate(top_indicator);
    lv_obj_invalidate(bottom_indicator);
    lv_obj_invalidate(coord_tl);
    lv_obj_invalidate(coord_br);
    lv_obj_invalidate(test_bar_0);
    lv_obj_invalidate(test_bar_8);
    lv_obj_invalidate(test_bar_16);
    lv_obj_invalidate(test_bar_24);
    Serial.println("DEBUG: 标记所有子对象为需要重绘");
    
    // 强制刷新显示
    lv_refr_now(_display);
    Serial.println("DEBUG: 强制刷新显示完成");
    
    lv_timer_handler();  // 立即处理LVGL任务
    Serial.println("DEBUG: LVGL任务处理完成");
    
    LVGL_LOG_I("测试网格显示完成");
    Serial.println("DEBUG: showTestGrid() 函数执行完成");
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