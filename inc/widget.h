/********************************************************************
* file: widget.h                      date: Sat 2025-07-19 11:28:56 *
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
#ifndef  __WIDGET_DEF_H__
#define  __WIDGET_DEF_H__

struct widget;

typedef void (*func)(struct widget *);

struct widget {
	char txt[128];
	void *img;
	char *font;
	struct widget *f;
	unsigned char *bg;
	void *pri;
	int font_size;
	unsigned short color;
	unsigned short *multi_color;
	int refresh;
	int x,y,w,h;
	func reset;
	func update;
	func draw;
};

#define INIT_WIDGET(_font, _size, _x, _y, _reset, _draw, _update, _f) \
	{                                  \
		.font = (_font),             \
		.font_size = (_size),        \
		.x = (_x),                   \
		.y = (_y),                   \
		.reset = (_reset),           \
		.draw = (_draw),             \
		.update = (_update),         \
		.f = (_f)                    \
	}
#endif/* __WIDGET_DEF_H__ */
/******************** End Of File: widget.h ********************/
