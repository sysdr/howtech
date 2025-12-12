CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -O2 -g
LDFLAGS = -ldl

all: elf_parser test_program test_program_stripped symbol_demo

elf_parser: src/elf_parser.c
	$(CC) $(CFLAGS) $< -o build/$@ $(LDFLAGS)

test_program: src/test_program.c
	$(CC) $(CFLAGS) $< -o build/$@

test_program_stripped: test_program
	cp build/test_program build/test_program_stripped
	strip --strip-all build/test_program_stripped

symbol_demo: src/symbol_demo.c
	$(CC) $(CFLAGS) $< -o build/$@ $(LDFLAGS)

plt_monitor.so: src/plt_monitor.c
	$(CC) $(CFLAGS) -shared -fPIC $< -o build/$@ $(LDFLAGS)

clean:
	rm -f build/*

.PHONY: all clean
