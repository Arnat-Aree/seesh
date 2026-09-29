CC     = cc
CFLAGS = -Wall -Wextra -std=gnu11 -g
SRC    = $(wildcard src/*.c)

all: seesh

seesh: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o seesh

run: seesh
	./seesh

clean:
	rm -f seesh

.PHONY: all run clean