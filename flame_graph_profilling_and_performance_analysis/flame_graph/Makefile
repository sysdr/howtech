CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -fno-omit-frame-pointer
TARGET = hotspot_demo

all: $(TARGET)

$(TARGET): src/hotspot_demo.c
	$(CC) $(CFLAGS) -o build/$(TARGET) src/hotspot_demo.c -lm

clean:
	rm -f build/$(TARGET)

.PHONY: all clean
