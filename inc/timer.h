/********************************************************************
* file: timer.h                       date: Mon 2025-07-28 18:27:32 *
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
#ifndef  __TIMER_DEF_H__
#define  __TIMER_DEF_H__

#include "list.h"

typedef enum {
	TASK_DAILY,
	TASK_WEEKLY,
	TASK_ONCE
} task_type;

typedef void (*task_callback_t)(void *arg);

typedef struct timer_task
{
	struct list_head list;  // 链表节点

	int tfd;
	task_type type;
	int hour, minute,sec;
	uint8_t weekdays[7]; // 周日=0，周一=1，... 周六=6

	task_callback_t callback;
	void *arg;
} timer_task;

// ---------------------- 管理器 -----------------------------

typedef struct
{
	struct list_head task_list;
	pthread_mutex_t mutex;
	int epfd;
} timer_manager;

int add_task(task_type type, int hour, int minute, int sec, const uint8_t weekdays[7], task_callback_t cb, void *arg);

#endif/* __TIMER_DEF_H__ */
/********************* End Of File: timer.h *********************/
