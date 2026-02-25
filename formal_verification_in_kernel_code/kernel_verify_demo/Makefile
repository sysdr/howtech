CC      = clang
CFLAGS  = -Wall -Wextra -Werror -O1 -g \
          -fsanitize=address,undefined,thread \
          -fno-omit-frame-pointer
LDFLAGS = -lpthread

# kcsan_sim uses threads; others don't need -fsanitize=thread
# We split targets to avoid TSAN+ASAN on same binary (incompatible)
ASAN_FLAGS  = -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
TSAN_FLAGS  = -Wall -Wextra -Werror -O1 -g -fsanitize=thread -fno-omit-frame-pointer

all: kasan_sim lockdep_sim bpf_verify_sim kcsan_sim monitor

kasan_sim: kasan_sim.c
	$(CC) $(ASAN_FLAGS) -o $@ $<

lockdep_sim: lockdep_sim.c
	$(CC) $(ASAN_FLAGS) -lpthread -o $@ $<

bpf_verify_sim: bpf_verify_sim.c
	$(CC) $(ASAN_FLAGS) -o $@ $<

kcsan_sim: kcsan_sim.c
	$(CC) $(TSAN_FLAGS) -lpthread -o $@ $<

monitor: monitor.c
	$(CC) -Wall -Wextra -Werror -O1 -g -lncurses -o $@ $<

clean:
	rm -f kasan_sim lockdep_sim bpf_verify_sim kcsan_sim monitor
