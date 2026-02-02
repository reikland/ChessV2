CC=gcc
CFLAGS=-std=c11 -O3 -march=native -flto -Wall -Wextra -Wshadow -Wconversion -DNDEBUG
LDFLAGS=-flto

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

engine: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f src/*.o engine
