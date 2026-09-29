CC     = cc
CFLAGS = -Wall -Wextra -std=gnu11 -g
SRC    = $(wildcard src/*.c)

all: seesh

seesh: $(SRC) $(wildcard src/*.h)
	$(CC) $(CFLAGS) $(SRC) -o seesh

run: seesh
	./seesh

clean:
	rm -rf seesh seesh.dSYM

.PHONY: all run clean