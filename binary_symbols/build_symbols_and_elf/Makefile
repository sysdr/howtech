CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -fPIC -g
LDFLAGS = -ldl

all: build/libmylib.so build/demo build/monitor

build/libmylib.so: src/mylib.c
	@mkdir -p build
	$(CC) $(CFLAGS) -shared -o $@ $< -Wl,-soname,libmylib.so

build/demo: src/main.c build/libmylib.so
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lmylib $(LDFLAGS) -Wl,-rpath,'$$ORIGIN'

build/monitor: src/monitor.c
	$(CC) -Wall -Wextra -O2 -o $@ $< -lncurses

clean:
	rm -rf build output logs

.PHONY: all clean
