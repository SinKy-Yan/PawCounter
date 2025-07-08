#include "LVGLDisplay.h"
#include "config.h"

LVGLDisplay* LVGLDisplay::_instance = nullptr;

LVGLDisplay::LVGLDisplay(Arduino_GFX* gfx, uint16_t width, uint16_t height) 
    : _gfx(gfx), _width(width), _height(height), _disp(nullptr), _buf1(nullptr), _buf2(nullptr) {
    _instance = this;
}

LVGLDisplay::~LVGLDisplay() {
    if (_buf1) {
        delete[] _buf1;
    }
    if (_buf2) {
        delete[] _buf2;
    }
    _instance = nullptr;
}

bool LVGLDisplay::begin() {
    if (!_gfx) {
        return false;
    }
    
    // 初始化LVGL
    lv_init();
    
    // 计算缓冲区大小 (1/10屏幕大小)
    uint32_t buf_size = _width * _height / 10;
    
    // 分配缓冲区
    _buf1 = new lv_color_t[buf_size];
    _buf2 = new lv_color_t[buf_size];
    
    if (!_buf1 || !_buf2) {
        return false;
    }
    
    // 初始化显示驱动
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    
    // 设置显示驱动参数
    disp_drv.hor_res = _width;
    disp_drv.ver_res = _height;
    disp_drv.flush_cb = display_flush_cb;
    disp_drv.rounder_cb = rounder_cb;
    disp_drv.set_px_cb = set_px_cb;
    
    // 设置缓冲区
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, _buf1, _buf2, buf_size);
    disp_drv.draw_buf = &draw_buf;
    
    // 注册显示驱动
    _disp = lv_disp_drv_register(&disp_drv);
    
    if (!_disp) {
        return false;
    }
    
    // 设置默认主题
    lv_theme_t* theme = lv_theme_default_init(_disp, lv_palette_main(LV_PALETTE_BLUE), 
                                             lv_palette_main(LV_PALETTE_RED), 
                                             LV_THEME_DEFAULT_DARK, 
                                             LV_FONT_DEFAULT);
    lv_disp_set_theme(_disp, theme);
    
    return true;
}

void LVGLDisplay::tick() {
    // 提供时间源给LVGL
    lv_tick_inc(1);
    
    // 处理LVGL任务
    lv_timer_handler();
}

void LVGLDisplay::flush() {
    if (_gfx && _disp) {
        // 强制刷新当前缓冲区 - 简化实现
        lv_refr_now(_disp);
    }
}

// 静态回调函数
void LVGLDisplay::display_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    if (_instance) {
        _instance->display_flush(disp_drv, area, color_p);
    }
}

void LVGLDisplay::rounder_cb(lv_disp_drv_t* disp_drv, lv_area_t* area) {
    if (_instance) {
        _instance->rounder(disp_drv, area);
    }
}

void LVGLDisplay::set_px_cb(lv_disp_drv_t* disp_drv, uint8_t* buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa) {
    if (_instance) {
        _instance->set_px(disp_drv, buf, buf_w, x, y, color, opa);
    }
}

// 实例方法实现
void LVGLDisplay::display_flush(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    if (!_gfx) {
        lv_disp_flush_ready(disp_drv);
        return;
    }
    
    // 计算区域尺寸
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    
    // 刷新到显示器
    _gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)color_p, w, h);
    
    // 通知LVGL刷新完成
    lv_disp_flush_ready(disp_drv);
}

void LVGLDisplay::rounder(lv_disp_drv_t* disp_drv, lv_area_t* area) {
    // 对于16位色深，不需要特殊舍入
    // 保持原始区域
}

void LVGLDisplay::set_px(lv_disp_drv_t* disp_drv, uint8_t* buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa) {
    // 直接像素设置（如果需要）
    if (buf && x >= 0 && y >= 0 && x < buf_w) {
        lv_color_t* color_buf = (lv_color_t*)buf;
        color_buf[y * buf_w + x] = color;
    }
}