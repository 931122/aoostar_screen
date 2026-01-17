/********************************************************************
* file: t.c                           date: Mon 2025-07-28 15:45:31 *
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
#include <unistd.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "widget.h"
#include "conf.h"

struct memory {
	char *response;
	size_t size;
};

static size_t write_callback(void *data, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	struct memory *mem = (struct memory *)userp;

	char *ptr = realloc(mem->response, mem->size + realsize + 1);
	if (ptr == NULL) return 0; // out of memory

	mem->response = ptr;
	memcpy(&(mem->response[mem->size]), data, realsize);
	mem->size += realsize;
	mem->response[mem->size] = 0;

	return realsize;
}

void _get_weather(void *args)
{
	int ret = 0;
	char url[512];
	struct memory chunk = {0};
	struct widget *w = (struct widget *)args;
	struct weather *wh = (struct weather *)w->pri;

	if (strlen(wh->token)==0) {
		fprintf(stderr, "token not found\n");
		sprintf(w->txt+ret, "   请添加高德\ntoken\n");
		w->refresh = 1;
		return;
	}
	if (strlen(wh->city)==0) {
		fprintf(stderr, "city not found\n");
		sprintf(w->txt+ret, "   请添加高德\n城市码\n");
		w->refresh = 1;
		return;
	}

	snprintf(url, sizeof(url),
			"https://restapi.amap.com/v3/weather/weatherInfo?city=%s&key=%s",
			wh->city, wh->token);

	CURL *curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "curl 初始化失败\n");
		return;
	}
	sprintf(w->txt, "天气获取中...");
	w->refresh = 1;

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	CURLcode res = curl_easy_perform(curl);
	sleep(1);
	if (res != CURLE_OK) {
		sprintf(w->txt, "获取失败\n%s",curl_easy_strerror(res));
		w->refresh = 1;
		return;
	}

	curl_easy_cleanup(curl);

	/* printf("%s\n", chunk.response); */
	cJSON *json = cJSON_Parse(chunk.response);
	if (json) {
		cJSON *lives = cJSON_GetObjectItem(json, "lives");
		if (cJSON_IsArray(lives) && cJSON_GetArraySize(lives) > 0) {
			cJSON *weather = cJSON_GetArrayItem(lives, 0);
			const char *reporttime = cJSON_GetObjectItem(weather, "reporttime")->valuestring;
			const char *weather_str = cJSON_GetObjectItem(weather, "weather")->valuestring;
			const char *temp = cJSON_GetObjectItem(weather, "temperature")->valuestring;
			const char *humidity = cJSON_GetObjectItem(weather, "humidity")->valuestring;
			const char *wind = cJSON_GetObjectItem(weather, "winddirection")->valuestring;

			ret += sprintf(w->txt+ret, "天气: %s ", weather_str);
			ret += sprintf(w->txt+ret, "风向: %s\n", wind);
			sprintf(w->txt+ret, "温度: %s°C 湿度: %s%\n", temp, humidity);
			w->refresh = 1;
		}
		else {
			printf("未获取到天气信息\n");
		}
		cJSON_Delete(json);
	}
	free(chunk.response);
	return;
}

#ifdef __cplusplus
};
#endif
/*********************** End Of File: t.c ***********************/
