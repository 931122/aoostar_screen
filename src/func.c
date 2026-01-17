/********************************************************************
* file: func.c                        date: Tue 2025-07-22 08:32:42 *
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
#include <ifaddrs.h>
#include <time.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sensors/sensors.h>
#include "ryzenadj.h"
#include "widget.h"
#include "draw.h"

#define PERFORM_REFRESH_TIME (30)

void time_update(struct widget *w)
{
	char date_str[32] = {0};
	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);
	strftime(w->txt, sizeof(w->txt), "%H:%M:%S", tm_info);
	strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
	w->refresh = 1;
	if (0 != strcmp(date_str, w->f->txt)) {
		strcpy(w->f->txt, date_str);
		w->f->refresh = 1;
	}
}

void get_ethx_addrs(struct widget *w)
{
	struct ifaddrs *ifaddr, *ifa;
	char ip[128] = {0};

	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		return;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) {
			continue;
		}

		if (strcmp(ifa->ifa_name, "vmbr0") != 0) {
			continue;
		}

		if (ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
			inet_ntop(AF_INET, &(sa->sin_addr), ip+strlen(ip), sizeof(ip));
			ip[strlen(ip)] = '\n';
		}
	}
	freeifaddrs(ifaddr);
	if (0 != strcmp(ip, w->txt)) {
		strcpy(w->txt, ip);
		w->refresh = 1;
	}
}

void get_perform(struct widget *w)
{

	static time_t last = 0;
	static time_t start =0; 
	static int last_val=0;
	int val;
	ryzen_access ry;

	start = time(NULL);
	if ((start - last) < PERFORM_REFRESH_TIME) {
		return;
	}
	last = start;

	ry = init_ryzenadj();

	if(!ry){
		printf("Unable to init ryzenadj\n");
		return;
	}
	init_table(ry);
	val = (int)get_stapm_limit(ry);
	cleanup_ryzenadj(ry);
	if (val == last_val) {
		return;
	}

	w->refresh = 1;
	if (val >=65) {
		sprintf(w->txt, "高性能\n  模式");
	}
	else if (val >= 45) {
		sprintf(w->txt, "均 衡\n模 式");
	}
	else {
		sprintf(w->txt, "省 电\n模 式");
	}
	last_val = val;
}

void get_cpu_temp(struct widget *w)
{
	static const sensors_chip_name *chip;
	const sensors_feature *feature;
	const sensors_subfeature *sub;
	static int chip_nr = 0;
	double value;
	static int found = 0;

	if (!found) {
		if (sensors_init(NULL) != 0)
			return;
		while ((chip = sensors_get_detected_chips(NULL, &chip_nr)) != NULL) {
			if (0 == strcmp(chip->prefix, "k10temp")) {
				found = 1;
				break;
			}
		}
	}
	chip_nr = 0;
	feature = sensors_get_features(chip, &chip_nr);
	sub = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_TEMP_INPUT);
	if (sub && sub->flags & SENSORS_MODE_R) {
		if (sensors_get_value(chip, sub->number, &value) == 0) {
			sprintf(w->txt, "%.1f°C", value);
			w->refresh = 1;
		}
	}
	return;

	//sensors_cleanup();
}

int get_network_bytes(const char *iface, uint64_t *rx_bytes, uint64_t *tx_bytes)
{
	FILE *fp = fopen("/proc/net/dev", "r");
	if (!fp) return -1;

	char line[512];
	while (fgets(line, sizeof(line), fp)) {
		char name[32];
		uint64_t rbytes, tbytes;

		// 过滤掉前两行（标题）
		if (strstr(line, "|") || strstr(line, "face")) continue;

		// 提取接口名和字段
		sscanf(line, " %[^:]:", name);
		if (strcmp(name, iface) == 0) {
			// 接口名匹配，解析字段
			sscanf(line,
					" %[^:]: %lu %*u %*u %*u %*u %*u %*u %*u %lu",
					name, &rbytes, &tbytes);
			*rx_bytes = rbytes;
			*tx_bytes = tbytes;
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
	return -1;
}

// 格式化速率显示
ssize_t format_speed(uint64_t bytes, char *out, size_t out_len, int is_up)
{
	const char *flag = NULL;
	const char *up = "↑";
	const char *down = "↓";
	if (is_up) {
		flag = up;
	}
	else flag = down;
	if (bytes > (1024 * 1024))
		return snprintf(out, out_len, "%s %.1f MB/s\n",flag, bytes / 1024.0 / 1024.0);
	else if (bytes > 1024)
		return snprintf(out, out_len, "%s %.1f KB/s\n", flag, bytes / 1024.0);
	else
		return snprintf(out, out_len, "%s %lu B/s\n", flag, bytes);
}

void get_txrx(struct widget *w)
{
	int ret = 0;
	static uint64_t last_rx = 0, last_tx = 0;
	static uint64_t rx, tx;
	get_network_bytes("vmbr0", &rx, &tx);
	uint64_t delta_rx = rx - last_rx;
	uint64_t delta_tx = tx - last_tx;

	last_rx = rx;
	last_tx = tx;

	ret = format_speed(delta_tx, w->txt+ret, 128-ret,1);
	ret = format_speed(delta_rx, w->txt+ret, 128-ret, 0);
	w->refresh = 1;
}

void get_weather(struct widget *w)
{
	//_get_weather(w->txt);
	//w->refresh = 1;
}

void get_usage(struct widget *w)
{
	int ret;
	FILE *fp = fopen("/proc/stat", "r");
	if (!fp) return;

	char line[256];
	fgets(line, sizeof(line), fp);
	fclose(fp);

	unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
	sscanf(line, "cpu  %lu %lu %lu %lu %lu %lu %lu %lu",
			&user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
	static unsigned long last_total = 0, last_idle = 0;

	unsigned long total = user + nice + system + idle + iowait + irq + softirq + steal;
	unsigned long delta_total = total - last_total;
	unsigned long delta_idle = idle - last_idle;

	last_total = total;
	last_idle = idle;

	ret = sprintf(w->txt, "%.1f%% ", 100.0f * (delta_total - delta_idle) / delta_total);
	fp = fopen("/proc/meminfo", "r");
	if (!fp) return;

	total = 1;
	long available = 0;
	while (fgets(line, sizeof(line), fp)) {
		if (sscanf(line, "MemTotal: %ld kB", &total) == 1) continue;
		if (sscanf(line, "MemAvailable: %ld kB", &available) == 1) break;
	}
	fclose(fp);
	ret = sprintf(w->txt+ret, "%.1f%% ", (float)(total - available) * 100 / total);
	w->refresh = 1;
}

void get_runtime(struct widget *w)
{
	char buf[128] = {0};
	struct timespec ts;
	clock_gettime(CLOCK_BOOTTIME, &ts);
	long total_seconds = ts.tv_sec;

	int days = total_seconds / (24 * 3600);
	total_seconds %= (24 * 3600);
	int hours = total_seconds / 3600;
	total_seconds %= 3600;
	int minutes = total_seconds / 60;

	sprintf(buf, "%dd %dh %dm\n", days, hours, minutes);
	if (0 != strcmp(buf, w->txt)) {
		strcpy(w->txt, buf);
		w->refresh = 1;
	}

	return;
}



static int first = 1;
void refresh_pve_vm(int signo)
{
	first = 1;
}

void get_pve_vm_status(struct widget *w)
{
	if (!first) return;
	FILE *fp = popen("qm list", "r");
	if (!fp) {
		perror("popen");
		return;
	}

	char line[128];
	char pve_name[128] = {0};
	unsigned short pve_st[12] = {0};
	int count = 0;

	if (w->f->multi_color == NULL) {
		w->f->multi_color = malloc(sizeof(unsigned short)*12);
		memset(w->f->multi_color, 0, sizeof(unsigned short)*12);
	}

	// 先跳过标题行
	if (!fgets(line, sizeof(line), fp)) {
		pclose(fp);
		return;
	}

	// 逐行读取
	int len = 0;
	int len2 = 0;
	while (fgets(line, sizeof(line), fp)) {
		// 行格式示例：
		// 100  Ubuntu     running    2048       32.00        1234
		int vmid;
		char name[64];
		char status[32];
		// 用 sscanf 解析前三个字段
		int ret = sscanf(line, "%d %63s %31s", &vmid, name, status);
		if (ret == 3) {
			ret = sprintf(pve_name+len, "%d-%s\n", vmid, name);
			len += ret;
			ret = sprintf(w->f->txt+len2, "●\n");
			len2 += ret;
			if (0 == strncmp(status, "r", 1)) {
				pve_st[count] = rgb565(0, 0, 255);
			}
			else {
				pve_st[count] = rgb565(0, 255, 0);
			}
			count++;
		}
	}
	pclose(fp);
	if (0 != strcmp(pve_name, w->txt)) {
		strcpy(w->txt, pve_name);
		w->refresh = 1;
	}
	if (0 != memcmp(pve_st, w->f->multi_color, sizeof(pve_st))) {
		w->f->refresh = 1;
		memcpy(w->f->multi_color, pve_st, sizeof(pve_st));
	}
	first = 0;
}

#ifdef __cplusplus
};
#endif
/********************* End Of File: func.c *********************/
