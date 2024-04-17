CC = gcc
CFLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC -g
LDFLAGS = -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup

.PHONY: clean all

all: libnand.so nand_example

libnand.so : nand.o memory_tests.o
	$(CC) -shared $(LDFLAGS) -o $@ $^

nand_example: nand_example.o
	$(CC) -o $@ $^ -L. -lnand -Wl,-rpath='.'

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o
