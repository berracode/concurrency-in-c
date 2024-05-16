CC=gcc
CFLAGS=-O0 -Wall -g
BINS=demo

.PHONY: all clean

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) $^ -c

demo: main.o
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -rf *.o $(BINS) $(TESTS)
