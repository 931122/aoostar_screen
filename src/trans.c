/********************************************************************
* file: trans.c                       date: Fri 2025-07-11 14:11:26 *
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
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "serial.h"

/*
 * 
 +--------------------+---------------------+------------------------+-----------------------------------------------+
|   Header (4B)       |    cmd (4B, LE)     |   addr (4B, LE)        |              Data Payload (LE, 47B)           |
|   0xAA55AA55        |    0x08 00 00 00    |   e.g. 0x56 34 12 00   |     d0     d1     ...     d45    d46          |
 +--------------------+---------------------+------------------------+-----------------------------------------------+
  Byte 0           3  Byte 4            7   Byte 8             11   Byte 12                                    Byte 58

cmd:
	0x5: start trans img data
	0x6: refresh
	0x8: img data
	0xa: power off
	0xb: powr on
 */

static int fd;
static pthread_mutex_t mutex;

void send_bytes(const void *buf, size_t len)
{

	pthread_mutex_lock(&mutex);
	if (-1 == fd) return;
	(void)write(fd, buf, len);
	pthread_mutex_unlock(&mutex);
}

void send_cmd(uint32_t cmd, const void *payload, size_t len)
{
	uint8_t buf[64] = {0};
	memcpy(buf, "\xAA\x55\xAA\x55", 4);
	memcpy(buf + 4, &cmd, 4);
	if (len > 0 && payload) {
		memcpy(buf + 8, payload, len);
	}
	send_bytes(buf, 8 + len);
}

void open_screen()
{
	fd = tb_serial_init();
	if (fd < 0) {
		printf("not fount!!\n");
		exit(0);
	}
	send_cmd(0x0B, NULL, 0);
}

void start_drawing(void)
{
	uint8_t payload[8] = {0x04, 0x00, 0x0F, 0x2F, 0x00, 0x04, 0x0B, 0x00};
	send_cmd(0x05, payload, 8);
}

void send_image_block(uint32_t address, const uint8_t *data47)
{
	uint8_t buf[4 + 47];
	memcpy(buf, &address, 4);
	memcpy(buf + 4, data47, 47);
	send_cmd(0x8, buf, sizeof(buf));
}

void show_image(void)
{
	send_cmd(0x6, NULL, 0);
}

void close_screen(void)
{
	if (-1 == fd) return;
	send_cmd(0xa, NULL, 0);
	close(fd);
	fd = -1;
}

void draw_background(unsigned char *bg)
{
	start_drawing();
	for (int i = 0; i < 360960*2+1; i += 47) {
		send_image_block(i, bg+i);
	}
	show_image();
}

__attribute__((constructor))
void trans_init() {
	pthread_mutex_init(&mutex, NULL);
}
#ifdef __cplusplus
};
#endif
/********************* End Of File: trans.c *********************/
