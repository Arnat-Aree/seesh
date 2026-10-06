CC     = cc
CFLAGS = -Wall -Wextra -std=gnu11 -g
SRC    = $(wildcard src/*.c)

all: seesh seesh-viewer

seesh: $(SRC) $(wildcard src/*.h)
	$(CC) $(CFLAGS) $(SRC) -o seesh

seesh-viewer: viewer/server.c
	$(CC) $(CFLAGS) viewer/server.c -o seesh-viewer

run: seesh
	./seesh

viewer: seesh-viewer
	./seesh-viewer

clean:
	rm -rf seesh seesh.dSYM seesh-viewer seesh-viewer.dSYM

.PHONY: all run viewer clean