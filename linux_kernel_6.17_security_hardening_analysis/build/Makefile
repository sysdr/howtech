CC      := gcc
CFLAGS  := -Wall -Wextra -Werror -O2 -g \
           -D_FORTIFY_SOURCE=3 \
           -fstack-protector-strong \
           -fstack-clash-protection \
           -fcf-protection=full \
           -pie -fPIE \
           -Wformat -Wformat-security
LDFLAGS := -lseccomp -lncurses

TARGETS := hardening_probe seccomp_demo fortify_demo kaslr_probe monitor

.PHONY: all clean

all: $(TARGETS)

hardening_probe: src/hardening_probe.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

seccomp_demo: src/seccomp_demo.c
	$(CC) $(CFLAGS) -o $@ $< -lseccomp

fortify_demo: src/fortify_demo.c
	$(CC) $(CFLAGS) -o $@ $<

kaslr_probe: src/kaslr_probe.c
	$(CC) $(CFLAGS) -o $@ $<

monitor: src/monitor.c
	$(CC) $(CFLAGS) -o $@ $< -lncurses

clean:
	rm -f $(TARGETS)
