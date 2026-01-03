CC = gcc
RUSTC = rustc
CFLAGS = -Wall -Wextra -Werror -O2 -g
RUSTFLAGS = --crate-type=staticlib -C opt-level=2

.PHONY: all clean dangerous safe monitor

all: dangerous safe monitor

# Dangerous example - signal handler calling Rust
dangerous: build/dangerous

build/dangerous: src/dangerous.c build/libdangerous_rust.a
	$(CC) $(CFLAGS) -o $@ src/dangerous.c -L./build -ldangerous_rust -lpthread -ldl

build/libdangerous_rust.a: src/dangerous_rust.rs
	$(RUSTC) $(RUSTFLAGS) -o $@ src/dangerous_rust.rs

# Safe example - signalfd approach
safe: build/safe_signalfd

build/safe_signalfd: src/safe_signalfd.c build/libsafe_rust.a
	$(CC) $(CFLAGS) -o $@ src/safe_signalfd.c -L./build -lsafe_rust -lpthread -ldl

build/libsafe_rust.a: src/safe_rust.rs
	$(RUSTC) $(RUSTFLAGS) -o $@ src/safe_rust.rs

# Monitor tool
monitor: build/monitor

build/monitor: src/monitor.c
	$(CC) $(CFLAGS) -o $@ src/monitor.c

clean:
	rm -rf build/*
