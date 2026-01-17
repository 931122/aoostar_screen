/********************************************************************
* file: conf.h                        date: Tue 2025-07-29 12:01:45 *
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
#ifndef  __CONF_DEF_H__
#define  __CONF_DEF_H__



struct time_info {
	int hour;
	int min;
};

struct weather {
	char token[128];
	char city[12];
	struct time_info update[5];
	int update_count;
};

struct lcd_info {
	struct time_info on;
	struct time_info off;
};

struct sys_config {
	int run;
	struct weather weather;
	struct lcd_info lcd;
};

int parse_config(const char *filename, struct sys_config *cfg);

#endif/* __CONF_DEF_H__ */
/********************* End Of File: conf.h *********************/
