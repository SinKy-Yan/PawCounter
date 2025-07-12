#pragma once

#define LV_CONF_INCLUDE_SIMPLE
#define LV_LVGL_H_INCLUDE_SIMPLE

// 启用LVGL
#define LV_CONF_SKIP 0
#define LV_CONF_MINIMAL 1

// 屏幕分辨率
#define LV_HOR_RES_MAX (480)
#define LV_VER_RES_MAX (135)

// 内存配置
#define LV_MEM_SIZE (64 * 1024U)  // 64KB内存池
#define LV_MEM_ADR 0

// 显示刷新配置
#define LV_DISP_DEF_REFR_PERIOD 30  // 30ms刷新周期
#define LV_INDEV_DEF_READ_PERIOD 30  // 30ms输入设备读取周期

// 字体配置
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_DEFAULT &lv_font_montserrat_16


// 颜色配置
#define LV_COLOR_DEPTH 16  // 16位颜色深度
#define LV_COLOR_16_SWAP 0  // 不交换字节序

// 其他配置
#define LV_TICK_CUSTOM 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_USE_LOG 1

// 组件配置
#define LV_USE_LABEL 1
#define LV_USE_BTN 1
#define LV_USE_IMG 1

// 动画配置
#define LV_USE_ANIMATION 1
#define LV_ANIM_DEF_DURATION 200

// 样式配置
#define LV_USE_STYLE_CONST_PROPS 1
#define LV_USE_STYLE_CACHE 1

// 禁用不需要的组件
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_BASIC 0
#define LV_USE_THEME_MONO 0

// 调试配置
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

// 性能配置
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0
#define LV_USE_REFR_DEBUG 0