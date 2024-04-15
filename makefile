CC = gcc
CFLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC -O2
LDFLAGS = -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup

.PHONY: all clean

all: libnand.so nand_examplek

libnand.so : nand.o memory_tests.o
	$(CC) -shared $(LDFLAGS) -o $@ $^

nand_example: nand_example.o
	$(CC) -o $@ $^ -L. -lnand -Wl,-rpath='.'

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o
