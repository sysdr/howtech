CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=
LDLIBS ?= -lncurses

BIN := dsq_sim
SRC := dsq_sim.c

.PHONY: all clean test

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

test: $(BIN)
	./$(BIN) --smoke
	./$(BIN) --headless 5 >/dev/null

clean:
	rm -f $(BIN)
