/********************************************************************
* file: draw.h                        date: Mon 2025-07-14 13:10:11 *
*                                                                   *
* Description:                                                      *
*                                                                   *
*                                                                   *
* Maintainer:  (yinxianglu)  <yinxianglu1993@gmail.com>             *
*                                                                   *
* This file is free software;                                       *
*   you are free to modify and/or redistribute it                   *
*   under the terms of the GNU General Public Licence (GPL).        *
*                                                                   *
* Last modified:                                                    *
*                                                                   *
* No warranty, no liability, use this at your own risk!             *
********************************************************************/
#ifndef  __DRAW_DEF_H__
#define  __DRAW_DEF_H__

#include <stdint.h>
#include "widget.h"

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 376
#define PIXEL_SIZE 2
#define BLOCK_SIZE 47

// RGB888 → RGB565
#define rgb565(r, g, b)  ( ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b) >> 3) )
//#define rgb565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

void draw_str(struct widget *w);
void restore_region(struct widget *w);
void draw_str_to_bg(uint16_t *bg, int x, int y, const char *utf8_text, const char *font_path, int font_size, uint16_t color_rgb565);
#endif/* __DRAW_DEF_H__ */
/********************* End Of File: draw.h *********************/
