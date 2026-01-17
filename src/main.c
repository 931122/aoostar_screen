/********************************************************************
* file: main.c                        date: Fri 2025-07-11 11:31:39 *
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
#include <limits.h>
#include <time.h>
#include <signal.h>
#include <sys/sysinfo.h>
#include "trans.h"
#include "draw.h"
#include "image.h"
#include "widget.h"
#include "timer.h"
#include "conf.h"

#define PID_FILE "/tmp/tbs.pid"
#define DEFAULT_ZH_CN "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
unsigned char *bg;

void get_perform(struct widget *w);
void time_update(struct widget *w);
void get_ethx_addrs(struct widget *w);
void get_pve_vm_status(struct widget *w);
void get_cpu_temp(struct widget *w);
void get_usage(struct widget *w);
void get_txrx(struct widget *w);
void get_runtime(struct widget *w);
void refresh_pve_vm(int signo);
void _get_weather(void *args);
void timer_init(void);

static struct widget _pve_st       = INIT_WIDGET(NULL,          26, 320, 20, restore_region, draw_str, NULL, NULL);
static struct widget _pve_name     = INIT_WIDGET(NULL,          26, 55,  20, restore_region, draw_str, get_pve_vm_status, &_pve_st);
static struct widget _date         = INIT_WIDGET(NULL,          26, 440,230, restore_region, draw_str, NULL, NULL);
static struct widget _time         = INIT_WIDGET(NULL,          26, 455,290, restore_region, draw_str, time_update, &_date);
static struct widget _cpu_temp     = INIT_WIDGET(NULL,          26, 800,230, restore_region, draw_str, get_cpu_temp, NULL);
static struct widget _usage        = INIT_WIDGET(NULL,          22, 790,290, restore_region, draw_str, get_usage, NULL);
static struct widget _ip           = INIT_WIDGET(NULL,          22, 760, 42, restore_region, draw_str, get_ethx_addrs, NULL);
static struct widget _net_speed    = INIT_WIDGET(NULL,          22, 760,100, restore_region, draw_str, get_txrx, NULL);
static struct widget _runtime      = INIT_WIDGET(DEFAULT_ZH_CN, 24, 505,110, restore_region, draw_str, get_runtime, NULL);
static struct widget _performance  = INIT_WIDGET(DEFAULT_ZH_CN, 26, 400, 50, restore_region, draw_str, get_perform, NULL);
static struct widget _weather      = INIT_WIDGET(DEFAULT_ZH_CN, 26, 80, 220, restore_region, draw_str, NULL, NULL);


struct widget *w_all[100] = {
	&_performance,
	&_pve_name,
	&_pve_st,
	&_date,
	&_ip,
	&_cpu_temp,
	&_usage,
	&_weather,
	&_net_speed,
	&_runtime,
	&_time,
	NULL,
};


void open_lcd(void *args)
{
	int *run = (int *)args;
	open_screen();
	*run = 1;
}
void close_lcd(void *args)
{
	int *run = (int *)args;
	close_screen();
	*run = 0;
}

int chdir_to_executable_dir(void)
{
	char exe_path[PATH_MAX] = {0};
	ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
	if (len == -1) {
		perror("readlink");
		return -1;
	}
	exe_path[len] = '\0';

	char *last_slash = strrchr(exe_path, '/');
	if (!last_slash) {
		fprintf(stderr, "Invalid exe path\n");
		return -1;
	}
	*last_slash = '\0';

	// 切换目录
	if (chdir(exe_path) != 0) {
		perror("chdir");
		return -1;
	}

	return 0;
}
void write_pid(const char *f_pid)
{
	FILE *fp = fopen(f_pid, "w");
	if (fp) {
		fprintf(fp, "%d\n", getpid());
		fclose(fp);
	}
}

void check_and_kill_old_instance(const char *f_pid)
{
	FILE *fp = fopen(f_pid, "r");
	int old_pid = 0;
	if (fp) {
		if (fscanf(fp, "%d", &old_pid) == 1 && old_pid > 0) {
			if (kill(old_pid, 0) == 0) {
				if (kill(old_pid, SIGKILL) == 0) {
					sleep(1);
				}
				else {
					perror("stop error");
					exit(1);
				}
			}
		}
		fclose(fp);
	}
}

int main(int argc, char *argv[])
{
	int ret;
	int i;
	const char *input_path = argv[1];
	int width = 0;
	int height = 0;
	struct sys_config cfg = {.run = 1, .lcd.on = {.hour= 0xff}, .lcd.off = {.hour= 0xff}};


	if (argc == 1) {
		bg = (unsigned char *)default_bg;
	}
	else {
		bg = malloc(960*376*2);
		ret = load_image_to_565(input_path, bg, &width, &height);
		if (ret != 0) {
			free(bg);
			return ret;
		}
	}
	draw_str_to_bg((uint16_t *)bg, 505,50, "系统已运行", DEFAULT_ZH_CN,22,rgb565(255,255,255));

	signal(SIGUSR2, refresh_pve_vm);
#if 1
	if (daemon(1, 1) < 0) {
		close_screen();
		exit(EXIT_FAILURE);
	}
#endif

	check_and_kill_old_instance(PID_FILE);
	write_pid(PID_FILE);
	chdir_to_executable_dir();
	if (parse_config("./config.json", &cfg) != 0) {
		printf("load config error\n");
		exit(EXIT_FAILURE);
	}

	timer_init();
	_weather.pri = &(cfg.weather);
	for (int i = 0; i < cfg.weather.update_count; i++) {
		add_task(TASK_DAILY, cfg.weather.update[i].hour, cfg.weather.update[i].min,0, NULL, _get_weather, &_weather);
	}
	time_t now = time(NULL)+3;
	struct tm tm_now = *localtime(&now);
	add_task(TASK_ONCE, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, NULL, _get_weather, &_weather);
	if (0xff != cfg.lcd.on.hour) {
		add_task(TASK_DAILY, cfg.lcd.on.hour, cfg.lcd.on.min, 0, NULL, open_lcd, &cfg.run);
		add_task(TASK_DAILY, cfg.lcd.off.hour, cfg.lcd.off.min, 0, NULL, close_lcd, &cfg.run);
	}

	open_screen();
	draw_background(bg);
	for (i = 0; i<100; i++) {
		struct widget *p = w_all[i];
		if (NULL == p) {
			break;
		}
		p->bg = bg;
		p->refresh = 1;
		p->color = rgb565(255,255,255);
	}

	struct timespec start, end;
	while (1) {
		if (!cfg.run) {
			sleep(1);
			continue;
		}
		clock_gettime(CLOCK_MONOTONIC, &start);
		for (i = 0; i<100; i++) {
			struct widget *p = w_all[i];
			if (NULL == p) {
				break;
			}
			if (p->update) p->update(p);
			if (p->refresh) {
				p->reset(p);
				p->draw(p);
				p->refresh = 0;
			}
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		uint32_t ms = (end.tv_sec - start.tv_sec) * 1000L +
			(end.tv_nsec - start.tv_nsec) / 1000000L;
		if (ms<1000) {
			usleep(1000*(1000-ms));
		}
	}

	return 0;
}

__attribute__((constructor))
void init() {
	//open_screen();
}

__attribute__((destructor))
void deinit()
{
	close_screen();
}


#ifdef __cplusplus
};
#endif
/********************* End Of File: main.c *********************/
