/********************************************************************
* file: serial.c                      date: Fri 2025-07-11 08:43:43 *
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>
#include <asm/ioctls.h>
#include <sys/stat.h>


#ifndef B1500000
#define B1500000 0x100F
#endif

#define SYS_TTY_PATH "/sys/class/tty"

int read_file(const char *path, char *buf, size_t len)
{
	FILE *f = fopen(path, "r");
	if (!f) return -1;
	if (!fgets(buf, len, f)) {
		fclose(f);
		return -1;
	}
	fclose(f);
	size_t l = strlen(buf);
	if (l > 0 && buf[l - 1] == '\n') buf[l - 1] = 0;
	return 0;
}

int check_usb_vid_pid(const char *path, const char *vid, const char *pid)
{
	char vendor_path[PATH_MAX], product_path[PATH_MAX];
	char vendor[16], product[16];

	snprintf(vendor_path, sizeof(vendor_path), "%s/idVendor", path);
	snprintf(product_path, sizeof(product_path), "%s/idProduct", path);

	if (access(vendor_path, R_OK) != 0 || access(product_path, R_OK) != 0) {
		return 0;  // 文件不存在，肯定不是USB设备根目录
	}

	if (read_file(vendor_path, vendor, sizeof(vendor)) < 0){
		return 0;
	}
	if (read_file(product_path, product, sizeof(product)) < 0) {
		return 0;
	}

	return (strcmp(vendor, vid) == 0) && (strcmp(product, pid) == 0);
}

int find_usb_device_path(const char *tty_device_path, const char *vid, const char *pid, char *result, size_t result_len)
{
	int i;
	char path[PATH_MAX] = {0};
	char abs_path[PATH_MAX];
	char parent[PATH_MAX];
	ssize_t len;

	len = readlink(tty_device_path, path, sizeof(path) - 1);
	if (len < 0) {
		return -1;
	}

	if (realpath(tty_device_path, abs_path) == NULL) {
		perror("realpath");
		return -1;
	}

	strncpy(parent, abs_path, sizeof(parent));
	for (i = 0; i < 10; i++) {
		if (check_usb_vid_pid(parent, vid, pid)) {
			strncpy(result, parent, result_len);
			return 0;
		}
		char *slash = strrchr(parent, '/');
		if (!slash) break;
		*slash = 0;
	}
	return -1;
}

int find_tty_byid(const char *vid, const char *pid, char *result, size_t result_len)
{
	char devpath[PATH_MAX];
	DIR *dir = opendir(SYS_TTY_PATH);
	if (!dir) {
		perror("opendir");
		return -1;
	}
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (strncmp(entry->d_name, "tty", 3) != 0)
			continue;

		snprintf(devpath, sizeof(devpath), SYS_TTY_PATH "/%s/device", entry->d_name);

		char usbdev_path[PATH_MAX];
		if (find_usb_device_path(devpath, vid, pid, usbdev_path, sizeof(usbdev_path)) == 0) {
			// 找到了匹配的设备
			snprintf(result, result_len, "/dev/%s", entry->d_name);
			closedir(dir);
			return 0;
		}
	}

	closedir(dir);

	return -1;
}

int set_serial_baudrate(const char *dev, int speed)
{
	int fd = open(dev, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	struct termios2 tio;
	if (ioctl(fd, TCGETS2, &tio) < 0) {
		perror("TCGETS2");
		close(fd);
		return -1;
	}

	tio.c_cflag &= ~CBAUD;
	tio.c_cflag |= BOTHER | B1500000;

	tio.c_cflag &= ~PARENB;
	tio.c_cflag &= ~PARODD;
	tio.c_cflag &= ~CSTOPB;
	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= CS8;

	tio.c_cflag &= ~CRTSCTS;
	tio.c_cflag |= CREAD | CLOCAL;
	tio.c_iflag &= ~(IXON | IXOFF | IXANY);
	tio.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
	tio.c_oflag &= ~OPOST;
	tio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 0;
	tio.c_ispeed = speed;
	tio.c_ospeed = speed;

	if (ioctl(fd, TCSETS2, &tio) < 0) {
		perror("TCSETS2");
		close(fd);
		return -1;
	}

	return fd;
}

int tb_serial_init(void)
{
	int fd;
	char dev[PATH_MAX];
	int speed = 1500000;

	find_tty_byid("0416", "90a1", dev, sizeof(dev));
	fd = set_serial_baudrate(dev, speed);
	if (fd < 0) {
		printf("set serial error\n");
	}

	return fd;
}

#ifdef __cplusplus
};
#endif
/*********************** End Of File: serial.c ***********************/
