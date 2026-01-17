CC = gcc

CFLAGS = -g -O2 -I./inc -I/usr/include/freetype2 -I./inc/stb/ #-fsanitize=address,undefined
LDFLAGS = lib/libryzenadj.a -lfreetype -lsensors -lm -lpci -lcjson -lcurl -s
TARGET = tbs

SRCS = src/draw.c src/func.c src/image.c src/main.c src/serial.c src/trans.c src/weather.c src/timer.c src/conf.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

