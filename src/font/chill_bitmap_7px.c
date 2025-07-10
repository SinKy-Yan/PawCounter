/*******************************************************************************
 * Size: 7 px
 * Bpp: 4
 * Opts: --font ChillBitmap_7px.ttf --size 7 --bpp 4 --format lvgl --symbols 0123456789+-*div=.C%~sqrt^2()M ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz --output chill_bitmap_7px.c --lv-font-name chill_bitmap_7px
 ******************************************************************************/

#include "lvgl.h"

#ifndef CHILL_BITMAP_7PX
#define CHILL_BITMAP_7PX 1
#endif

#if CHILL_BITMAP_7PX

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0025 "%" */
    0x2c, 0x0, 0x52, 0x6, 0x45, 0x46, 0x20, 0x64,
    0x60, 0x30, 0x0, 0xb0, 0x1, 0x48, 0x0, 0x4c,
    0xc, 0x82, 0x24, 0x54, 0x64, 0x10,

    /* U+0028 "(" */
    0x4, 0xa2, 0x8a, 0x0, 0xff, 0xe0, 0x14, 0x50,

    /* U+0029 ")" */
    0xe0, 0xcc, 0x0, 0x7e, 0xcc, 0x0,

    /* U+002A "*" */
    0x2c, 0xd, 0x3a, 0xd3, 0xac, 0x8a,

    /* U+002B "+" */
    0x2c, 0xd, 0x3a, 0xd3, 0xa0,

    /* U+002D "-" */
    0xff, 0x50,

    /* U+002E "." */
    0xe0,

    /* U+002F "/" */
    0x4, 0xa0, 0x8, 0xa2, 0x80, 0x2c, 0xc0, 0x6,

    /* U+0030 "0" */
    0x2f, 0xac, 0xb0, 0xf, 0x86, 0x28,

    /* U+0031 "1" */
    0x2c, 0xd0, 0xd0, 0xe,

    /* U+0032 "2" */
    0xfc, 0xf, 0x8a, 0x28, 0xac, 0xc0, 0xf, 0xd0,

    /* U+0033 "3" */
    0xfc, 0xf, 0x8a, 0x28, 0xa2, 0x8a, 0xf8, 0xa0,

    /* U+0034 "4" */
    0x2c, 0xc, 0xc0, 0x1, 0x28, 0x6c, 0x3e, 0xc0,

    /* U+0035 "5" */
    0xff, 0x50, 0xfd, 0xf, 0xd7, 0xd8, 0x7c, 0x50,

    /* U+0036 "6" */
    0x2c, 0xc, 0xc0, 0xf, 0xd0, 0xd8, 0xc, 0x50,

    /* U+0037 "7" */
    0xff, 0x57, 0xd8, 0x14, 0x50, 0x5, 0x98, 0x0,

    /* U+0038 "8" */
    0x2f, 0xac, 0xb0, 0xc8, 0xac, 0x8a, 0x18, 0xa0,

    /* U+0039 "9" */
    0x2f, 0xac, 0xb0, 0x1b, 0xf, 0xb0, 0x28, 0xa0,

    /* U+003D "=" */
    0xff, 0x57, 0xfa, 0xbf, 0xd4,

    /* U+0041 "A" */
    0x4, 0xa0, 0x28, 0xc8, 0xcc, 0x0, 0x7, 0xe4,
    0x7, 0xe4, 0x0,

    /* U+0042 "B" */
    0xff, 0x50, 0xf, 0xe4, 0xf, 0xe4, 0xf, 0xe4,
    0xf, 0xe4, 0x0,

    /* U+0043 "C" */
    0x2f, 0xa0, 0xcf, 0xc8, 0x0, 0x34, 0x0, 0x1a,
    0x33, 0xf2, 0x0,

    /* U+0044 "D" */
    0xff, 0x50, 0xf, 0xe4, 0x0, 0x72, 0x64, 0xc,
    0x50, 0x0,

    /* U+0045 "E" */
    0xff, 0x50, 0xfd, 0xf, 0xd0, 0xfd, 0xf, 0xd0,

    /* U+0046 "F" */
    0xff, 0x50, 0xfd, 0xf, 0xd0, 0xfd, 0x0, 0x40,

    /* U+0047 "G" */
    0x2f, 0xf4, 0x67, 0xfa, 0x1, 0x3e, 0x1, 0x24,
    0x33, 0xf2, 0x0,

    /* U+0048 "H" */
    0xe0, 0x68, 0x0, 0xc3, 0xf2, 0x3, 0xf2, 0x1,
    0xc0,

    /* U+0049 "I" */
    0xe0, 0xc,

    /* U+004A "J" */
    0x2f, 0xf4, 0x15, 0xac, 0x0, 0x6e, 0x0, 0xb2,
    0x28, 0x0,

    /* U+004B "K" */
    0xe0, 0x68, 0x4, 0xc8, 0x18, 0xa0, 0x18, 0xa0,
    0x2, 0x64, 0x0,

    /* U+004C "L" */
    0xe0, 0xf, 0xf8, 0x7e, 0x80,

    /* U+004D "M" */
    0xe0, 0x4, 0x30, 0xe3, 0x38, 0xc, 0x63, 0x80,
    0x7c, 0x94, 0x0,

    /* U+004E "N" */
    0xe0, 0x68, 0x1c, 0x0, 0xc, 0x48, 0x1, 0x24,
    0x3, 0x80,

    /* U+004F "O" */
    0x2f, 0xa0, 0xcf, 0xc8, 0x0, 0xfd, 0x9f, 0x90,

    /* U+0050 "P" */
    0xff, 0x50, 0xf, 0xe4, 0xf, 0xc8, 0xf, 0xfa,
    0x0, 0x30,

    /* U+0051 "Q" */
    0x2f, 0xa0, 0xcf, 0xc8, 0x1c, 0x0, 0xc, 0x64,
    0x64, 0x64, 0x0,

    /* U+0052 "R" */
    0xff, 0x50, 0xf, 0xe4, 0xf, 0xe4, 0xd, 0x80,
    0x49, 0x90,

    /* U+0053 "S" */
    0x2f, 0xf4, 0x67, 0xfa, 0x33, 0xe8, 0xb, 0xf2,
    0x3f, 0xd9, 0x0,

    /* U+0054 "T" */
    0xff, 0x56, 0x9d, 0x0, 0x7f, 0x0,

    /* U+0055 "U" */
    0xe0, 0x68, 0x0, 0xff, 0xe0, 0x67, 0xe4, 0x0,

    /* U+0056 "V" */
    0xe0, 0x68, 0x0, 0xfe, 0x4c, 0x8c, 0x8a, 0x0,

    /* U+0057 "W" */
    0xe0, 0x4, 0x30, 0x25, 0x0, 0x7d, 0x91, 0x80,
    0xc0, 0x1c,

    /* U+0058 "X" */
    0xe0, 0x68, 0x0, 0xd9, 0xf9, 0x19, 0xf9, 0x0,
    0x18,

    /* U+0059 "Y" */
    0xe0, 0x68, 0x0, 0xd9, 0xf9, 0x5, 0x60, 0xf,
    0x8a, 0x0,

    /* U+005A "Z" */
    0xff, 0x57, 0xd8, 0x14, 0x56, 0x60, 0x7, 0xe8,

    /* U+0061 "a" */
    0x2f, 0xac, 0xb0, 0xd, 0x96, 0x0,

    /* U+0062 "b" */
    0xe0, 0xe, 0x1c, 0x1, 0x8a, 0x0, 0x86, 0x28,

    /* U+0063 "c" */
    0x2f, 0xac, 0xfa, 0x0, 0xb3, 0xe8,

    /* U+0064 "d" */
    0x4, 0xa0, 0x8, 0xac, 0x32, 0xc0, 0x36, 0x58,
    0x0,

    /* U+0065 "e" */
    0x2f, 0xac, 0xb0, 0x18, 0xad, 0x3a,

    /* U+0066 "f" */
    0x2c, 0xc8, 0x70, 0x70, 0x3, 0x0,

    /* U+0067 "g" */
    0x2f, 0xac, 0xb0, 0x1b, 0xf, 0xb0, 0xf8, 0xa0,

    /* U+0068 "h" */
    0xe0, 0xe, 0x1c, 0x1, 0x8a, 0x0, 0xf0,

    /* U+0069 "i" */
    0xee, 0x80, 0x20,

    /* U+006A "j" */
    0x2c, 0x2c, 0x2c, 0x0, 0xf6, 0x60,

    /* U+006B "k" */
    0xe0, 0xf, 0x25, 0xc, 0x50, 0xc5, 0x0, 0x40,

    /* U+006C "l" */
    0xe0, 0xe,

    /* U+006D "m" */
    0xfc, 0x68, 0x1, 0x8c, 0x6, 0x0, 0xf2, 0x50,
    0x0,

    /* U+006E "n" */
    0xfc, 0x1, 0x8a, 0x0, 0xf0,

    /* U+006F "o" */
    0x2c, 0xc, 0x8a, 0x0, 0xb2, 0x28,

    /* U+0070 "p" */
    0xfc, 0x1, 0x8a, 0x0, 0x86, 0x28, 0x70, 0x0,

    /* U+0071 "q" */
    0x2f, 0xac, 0xb0, 0xd, 0x96, 0x5, 0x60,

    /* U+0072 "r" */
    0x2c, 0xc8, 0x3, 0x0,

    /* U+0073 "s" */
    0x2f, 0xad, 0x3a, 0xd3, 0xad, 0x3a,

    /* U+0074 "t" */
    0xe0, 0x1c, 0x1c, 0x0, 0x66, 0x0,

    /* U+0075 "u" */
    0xe4, 0xa0, 0xf, 0x65, 0x80,

    /* U+0076 "v" */
    0xe4, 0xa0, 0x8, 0x62, 0x87, 0x0,

    /* U+0077 "w" */
    0xe0, 0x4, 0x30, 0x25, 0x0, 0x6, 0xd4, 0x18,
    0x6d, 0x60, 0x0,

    /* U+0078 "x" */
    0xe4, 0xac, 0x8a, 0xc8, 0xa0, 0x8,

    /* U+0079 "y" */
    0xe4, 0xa0, 0xb, 0x2c, 0xa, 0xc3, 0xe2, 0x80,

    /* U+007A "z" */
    0xff, 0x57, 0xd8, 0x14, 0x56, 0x9d, 0x0,

    /* U+00B1 "±" */
    0x0, 0x34, 0x0, 0x7f, 0xbf, 0xd2, 0xff, 0xe2,
    0xff, 0x4b, 0xff, 0x88, 0x3, 0xf9, 0xa0, 0x2,
    0xff, 0xf1, 0x0,

    /* U+221A "√" */
    0x0, 0x27, 0xf7, 0x4, 0x7, 0xa7, 0xb8, 0x20,
    0x62, 0xc0, 0x16, 0xf8, 0x7, 0x69, 0xd8, 0x7,
    0xfc
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 42, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 112, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 22, .adv_w = 56, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 30, .adv_w = 56, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 36, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 42, .adv_w = 56, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 47, .adv_w = 56, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 49, .adv_w = 28, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 50, .adv_w = 56, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 58, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 42, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 68, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 76, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 84, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 92, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 108, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 124, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 56, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 137, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 148, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 159, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 170, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 196, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 216, .adv_w = 28, .box_w = 1, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 218, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 228, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 84, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 255, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 273, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 294, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 304, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 315, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 321, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 329, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 337, .adv_w = 84, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 70, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 366, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 374, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 56, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 388, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 394, .adv_w = 56, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 403, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 409, .adv_w = 42, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 415, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 423, .adv_w = 56, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 430, .adv_w = 28, .box_w = 1, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 433, .adv_w = 42, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 439, .adv_w = 56, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 447, .adv_w = 28, .box_w = 1, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 449, .adv_w = 84, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 458, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 463, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 469, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 477, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 484, .adv_w = 42, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 488, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 494, .adv_w = 42, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 500, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 505, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 511, .adv_w = 84, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 522, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 528, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 536, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 543, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 562, .adv_w = 112, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = -1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint8_t glyph_id_ofs_list_0[] = {
    0, 0, 0, 0, 0, 1, 0, 0,
    2, 3, 4, 5, 0, 6, 7, 8,
    9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 0, 0, 0, 19
};

static const uint16_t unicode_list_3[] = {
    0x0, 0x2169
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 30, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = glyph_id_ofs_list_0, .list_length = 30, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL
    },
    {
        .range_start = 65, .range_length = 26, .glyph_id_start = 21,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 97, .range_length = 26, .glyph_id_start = 47,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 177, .range_length = 8554, .glyph_id_start = 73,
        .unicode_list = unicode_list_3, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 1, 2, 2, 2, 2, 3, 3,
    3, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 3, 0, 0, 0,
    4, 0, 5, 0, 0, 0, 6, 0,
    7, 0, 0, 0, 0, 0, 0, 0,
    8, 0, 0, 0, 0, 0, 0, 4,
    9, 4, 0, 4, 0, 4, 4, 0,
    0, 4, 0, 4, 4, 9, 9, 4,
    9, 9, 4, 4, 9, 9, 4, 4,
    4, 0, 0
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 1, 2, 2, 2, 2, 2, 2,
    2, 2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 2, 3, 0, 4,
    0, 0, 0, 3, 0, 0, 0, 0,
    0, 0, 0, 3, 0, 3, 0, 0,
    5, 0, 0, 6, 0, 0, 0, 3,
    0, 3, 3, 3, 0, 3, 0, 0,
    7, 0, 0, 4, 3, 3, 4, 3,
    3, 3, 0, 4, 4, 4, 4, 4,
    4, 0, 0
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, -14, 0, 0, -14, 0, -14, -14,
    0, 0, 0, 0, 0, 0, -14, 0,
    0, 0, -14, 0, 0, 0, 0, 0,
    0, -14, 0, 0, 0, 0, 0, 0,
    0, 0, -14, 0, 0, -14, 0, 0,
    0, -14, 0, 0, 0, 0, -14, -14,
    0, 0, 0, -14, -14, 0, 0, -14,
    0, 0, 0, 0, -14, 0, -14
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 9,
    .right_class_cnt     = 7,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 4,
    .bpp = 4,
    .kern_classes = 1,
    .bitmap_format = 1,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t chill_bitmap_7px = {
#else
lv_font_t chill_bitmap_7px = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 7,          /*The maximum line height required by the font*/
    .base_line = 1,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if CHILL_BITMAP_7PX*/

