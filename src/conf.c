/********************************************************************
* file: 2.c                           date: Tue 2025-07-29 11:55:17 *
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
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "conf.h"

static void parse_time_str(const char *str, struct time_info *t)
{
	if (sscanf(str, "%d:%d", &t->hour, &t->min) != 2) {
		t->hour = 0;
		t->min = 0;
	}
}

int parse_config(const char *filename, struct sys_config *cfg)
{
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "open %s error\n", filename);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	rewind(fp);

	char *data = malloc(len + 1);
	if (!data) {
		fclose(fp);
		return -1;
	}

	fread(data, 1, len, fp);
	data[len] = '\0';
	fclose(fp);

	cJSON *root = cJSON_Parse(data);
	free(data);
	if (!root) {
		fprintf(stderr, "cJSON_Parse error\n");
		return -1;
	}

	// weather
	cJSON *weather = cJSON_GetObjectItem(root, "weather");
	if (weather) {
		cJSON *token = cJSON_GetObjectItem(weather, "token");
		cJSON *city = cJSON_GetObjectItem(weather, "city");
		cJSON *update_array = cJSON_GetObjectItem(weather, "update_time");

		if (cJSON_IsString(token))
			strncpy(cfg->weather.token, token->valuestring, sizeof(cfg->weather.token) - 1);
		if (cJSON_IsString(city))
			strncpy(cfg->weather.city, city->valuestring, sizeof(cfg->weather.city) - 1);

		cfg->weather.update_count = 0;
		if (cJSON_IsArray(update_array)) {
			int count = cJSON_GetArraySize(update_array);
			for (int i = 0; i < count && i < 5; i++) {
				cJSON *time_item = cJSON_GetArrayItem(update_array, i);
				if (cJSON_IsString(time_item)) {
					parse_time_str(time_item->valuestring, &cfg->weather.update[i]);
					cfg->weather.update_count++;
				}
			}
		}
	}

	// lcd
	cJSON *lcd = cJSON_GetObjectItem(root, "lcd");
	if (lcd) {
		cJSON *on_time = cJSON_GetObjectItem(lcd, "lcd_on_time");
		cJSON *off_time = cJSON_GetObjectItem(lcd, "lcd_off_time");

		if (cJSON_IsString(on_time))
			parse_time_str(on_time->valuestring, &cfg->lcd.on);
		if (cJSON_IsString(off_time))
			parse_time_str(off_time->valuestring, &cfg->lcd.off);
	}

	cJSON_Delete(root);
	return 0;
}

#ifdef __cplusplus
};
#endif
/*********************** End Of File: 2.c ***********************/
