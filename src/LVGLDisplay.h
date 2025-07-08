#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>

/**
 * @brief LVGL显示驱动适配器
 * @details 将现有的Arduino_GFX系统桥接到LVGL
 */
class LVGLDisplay {
public:
    LVGLDisplay(Arduino_GFX* gfx, uint16_t width, uint16_t height);
    ~LVGLDisplay();
    
    bool begin();
    void tick();
    void flush();
    
private:
    Arduino_GFX* _gfx;
    uint16_t _width;
    uint16_t _height;
    
    lv_disp_t* _disp;
    lv_color_t* _buf1;
    lv_color_t* _buf2;
    
    // LVGL回调函数
    static void display_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p);
    static void rounder_cb(lv_disp_drv_t* disp_drv, lv_area_t* area);
    static void set_px_cb(lv_disp_drv_t* disp_drv, uint8_t* buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa);
    
    // 实例方法
    void display_flush(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p);
    void rounder(lv_disp_drv_t* disp_drv, lv_area_t* area);
    void set_px(lv_disp_drv_t* disp_drv, uint8_t* buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa);
    
    static LVGLDisplay* _instance;
};