CC       = gcc
CFLAGS   = -Wall -Wextra -Werror -O2 -g -pthread -lm
SUFFIXES = system tcmalloc jemalloc

all: bench_system bench_tcmalloc bench_jemalloc

bench_system: alloc_bench.c
	$(CC) $(CFLAGS) -o $@ $<

bench_tcmalloc: alloc_bench.c
	$(CC) $(CFLAGS) -o $@ $< -ltcmalloc

bench_jemalloc: alloc_bench.c
	$(CC) $(CFLAGS) -o $@ $< -ljemalloc

clean:
	rm -f bench_system bench_tcmalloc bench_jemalloc
