CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -O2 -g
LDFLAGS = -lpthread

BINS = uio_sim iommu_inspect latency_compare qualification_scope

.PHONY: all clean

all: $(BINS)

uio_sim: src/uio_sim.c
	$(CC) $(CFLAGS) -o $@ $<

iommu_inspect: src/iommu_inspect.c
	$(CC) $(CFLAGS) -o $@ $<

latency_compare: src/latency_compare.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

qualification_scope: src/qualification_scope.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BINS)
