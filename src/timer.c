/********************************************************************
* file: timer.c                       date: Mon 2025-07-28 18:11:43 *
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
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "timer.h"
#include "list.h"


static timer_manager manager;

static time_t calc_next_interval(const timer_task *task)
{
	time_t now = time(NULL);
	struct tm tm_now = *localtime(&now);

	tm_now.tm_sec = 0;
	tm_now.tm_min = task->minute;
	tm_now.tm_hour = task->hour;
	tm_now.tm_sec = task->sec;

	time_t target = mktime(&tm_now);

	if (task->type == TASK_ONCE) {
		if (target <= now) return -1; // 已过期
		return target - now;
	}

	if (task->type == TASK_DAILY) {
		if (target <= now) target += 86400;
		return target - now;
	}

	if (task->type == TASK_WEEKLY) {
		for (int i = 0; i <= 7; i++) {
			time_t candidate = target + i * 86400;
			struct tm *tm_cand = localtime(&candidate);
			int wday = tm_cand->tm_wday;
			if (task->weekdays[wday]) {
				if (candidate > now)
					return candidate - now;
			}
		}
		return -1;
	}

	return -1;
}

static int set_timerfd(timer_task *task)
{
	time_t interval = calc_next_interval(task);
	if (interval < 0) return -1;

	struct itimerspec its = {0};
	its.it_value.tv_sec = interval;
	its.it_interval.tv_sec = 0;
	return timerfd_settime(task->tfd, 0, &its, NULL);
}

void add_task_to_list(timer_task *task)
{
	pthread_mutex_lock(&manager.mutex);
	list_add_tail(&task->list, &manager.task_list);
	pthread_mutex_unlock(&manager.mutex);
}

void remove_task_from_list(timer_task *task)
{
	pthread_mutex_lock(&manager.mutex);
	list_del(&task->list);
	pthread_mutex_unlock(&manager.mutex);
}

int add_task(task_type type, int hour, int minute,int sec, const uint8_t weekdays[7], task_callback_t cb, void *arg)
{
	timer_task *task = calloc(1, sizeof(*task));
	if (!task) return -1;

	task->type = type;
	task->hour = hour;
	task->minute = minute;
	task->sec = sec;
	task->callback = cb;
	task->arg = arg;
	if (weekdays && type == TASK_WEEKLY)
		memcpy(task->weekdays, weekdays, 7);

	task->tfd = timerfd_create(CLOCK_REALTIME, 0);
	if (task->tfd < 0) {
		free(task);
		return -1;
	}

	if (set_timerfd(task) < 0) {
		close(task->tfd);
		free(task);
		return -1;
	}

	struct epoll_event ev = {0};
	ev.events = EPOLLIN;
	ev.data.ptr = task;

	pthread_mutex_lock(&manager.mutex);
	if (epoll_ctl(manager.epfd, EPOLL_CTL_ADD, task->tfd, &ev) < 0) {
		pthread_mutex_unlock(&manager.mutex);
		close(task->tfd);
		free(task);
		return -1;
	}
	list_add_tail(&task->list, &manager.task_list);
	pthread_mutex_unlock(&manager.mutex);

	return 0;
}

void free_task(timer_task *task)
{
	if (!task) return;
	epoll_ctl(manager.epfd, EPOLL_CTL_DEL, task->tfd, NULL);
	close(task->tfd);
	free(task);
}

static void * run_loop(void *args)
{
	const int MAX_EVENTS = 10;
	struct epoll_event events[MAX_EVENTS];

	while (1) {
		int n = epoll_wait(manager.epfd, events, MAX_EVENTS, -1);
		if (n < 0) {
			if (errno == EINTR) continue;
			perror("epoll_wait");
			break;
		}

		for (int i = 0; i < n; i++) {
			timer_task *task = (timer_task *)events[i].data.ptr;
			uint64_t expirations;

			if (read(task->tfd, &expirations, sizeof(expirations)) != sizeof(expirations)) {
				perror("read timerfd");
				continue;
			}

			if (task->callback)
				task->callback(task->arg);

			if (task->type == TASK_ONCE) {
				pthread_mutex_lock(&manager.mutex);
				list_del(&task->list);
				pthread_mutex_unlock(&manager.mutex);
				free_task(task);
			} else {
				if (set_timerfd(task) < 0) {
					pthread_mutex_lock(&manager.mutex);
					list_del(&task->list);
					pthread_mutex_unlock(&manager.mutex);
					free_task(task);
				}
			}
		}
	}
}

static void manager_init(void)
{
	INIT_LIST_HEAD(&manager.task_list);
	pthread_mutex_init(&manager.mutex, NULL);

	manager.epfd = epoll_create1(0);
	if (manager.epfd < 0) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}
}

int timer_init(void)
{
	pthread_t tid;
	if (manager.epfd > 0) {
		return 0;
	}
	manager_init();
	pthread_create(&tid, NULL, run_loop, NULL);
	pthread_detach(tid);

	return 0;
}

#ifdef __cplusplus
};
#endif
/********************* End Of File: timer.c *********************/
