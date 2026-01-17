/* serial.c */
int read_file(const char *path, char *buf, size_t len);
int check_usb_vid_pid(const char *path, const char *vid, const char *pid);
int find_usb_device_path(const char *tty_device_path, const char *vid, const char *pid, char *result, size_t result_len);
int find_tty_byid(const char *vid, const char *pid, char *result, size_t result_len);
int set_serial_baudrate(const char *dev, int speed);
int tb_serial_init(void);
