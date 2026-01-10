CC_DYNAMIC = gcc
CC_STATIC_GLIBC = gcc
CC_STATIC_MUSL = musl-gcc
CFLAGS = -Wall -Wextra -Werror -O2
LDFLAGS_STATIC = -static -s

.PHONY: all dynamic static-glibc static-musl clean

all: dynamic static-musl

dynamic: httpserver.c
	$(CC_DYNAMIC) $(CFLAGS) httpserver.c -o ../build/httpserver-dynamic
	@echo "Built: httpserver-dynamic"

static-glibc: httpserver.c
	$(CC_STATIC_GLIBC) $(CFLAGS) $(LDFLAGS_STATIC) httpserver.c -o ../build/httpserver-static-glibc
	@echo "Built: httpserver-static-glibc"

static-musl: httpserver.c
	$(CC_STATIC_MUSL) $(CFLAGS) $(LDFLAGS_STATIC) httpserver.c -o ../build/httpserver-static-musl
	@echo "Built: httpserver-static-musl"

tools: memanalyzer.c monitor.c
	$(CC_STATIC_MUSL) $(CFLAGS) $(LDFLAGS_STATIC) memanalyzer.c -o ../build/memanalyzer
	$(CC_STATIC_MUSL) $(CFLAGS) $(LDFLAGS_STATIC) monitor.c -o ../build/monitor
	@echo "Built: analysis tools"

clean:
	rm -f ../build/*
