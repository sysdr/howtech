CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -fPIC -I.
LDFLAGS = -ldl

PLUGINS = plugins/plugin_reverse.so plugins/plugin_rot13.so plugins/plugin_upper.so
MAIN = build/plugin_demo

all: $(MAIN) $(PLUGINS)

$(MAIN): src/main.c src/plugin_api.h
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ src/main.c $(LDFLAGS)

plugins/%.so: src/%.c src/plugin_api.h
	@mkdir -p plugins
	$(CC) $(CFLAGS) -shared -o $@ $<

clean:
	rm -rf build plugins

.PHONY: all clean
