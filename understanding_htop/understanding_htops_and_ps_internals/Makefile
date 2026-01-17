CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g -std=c11
LDFLAGS = -pthread -lncurses

SRCS = src/process_info.c src/monitor.c
STRESS_SRC = src/stress.c
INSPECTOR_SRC = src/proc_inspector.c

OBJS = $(SRCS:src/%.c=build/%.o)
TARGET = build/monitor
STRESS_TARGET = build/stress
INSPECTOR_TARGET = build/proc_inspector

all: $(TARGET) $(STRESS_TARGET) $(INSPECTOR_TARGET)

build/process_info.o: src/process_info.c src/process_info.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/monitor.o: src/monitor.c src/process_info.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): build/process_info.o build/monitor.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(STRESS_TARGET): $(STRESS_SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(INSPECTOR_TARGET): $(INSPECTOR_SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf build

.PHONY: all clean
