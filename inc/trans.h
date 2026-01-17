/********************************************************************
* file: trans.h                       date: Fri 2025-07-11 14:29:46 *
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
#ifndef  __TRANS_DEF_H__
#define  __TRANS_DEF_H__

void send_bytes(const void *buf, signed int len);
void send_cmd(unsigned int cmd, void *buf, int len);
void open_screen(void);
void start_drawing(void);
void send_image_block(unsigned int addr, unsigned char *buf);
void show_image(void);
void close_screen(void);
void draw_background(unsigned char *bg);

#endif/* __TRANS_DEF_H__ */
/********************* End Of File: trans.h *********************/
