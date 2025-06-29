#ifndef LV_FONT_ZPIX_H
#define LV_FONT_ZPIX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// ZPIX 字体声明（计算器专用）
extern const lv_font_t lv_font_zpix_20;    // 表达式显示字体
extern const lv_font_t lv_font_zpix_60;    // 主数字显示字体

// 计算器字体宏定义
#define CALCULATOR_FONT_EXPRESSION  &lv_font_zpix_20   // 表达式字体（20px）
#define CALCULATOR_FONT_MAIN_NUMBER &lv_font_zpix_60   // 主数字字体（60px）

// 备用字体定义（如果 zpix 字体不可用）
#define CALCULATOR_FONT_FALLBACK_SMALL  &lv_font_montserrat_14
#define CALCULATOR_FONT_FALLBACK_LARGE  &lv_font_montserrat_24

#ifdef __cplusplus
}
#endif

#endif /* LV_FONT_ZPIX_H */
