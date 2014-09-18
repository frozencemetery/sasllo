CFLAGS += -std=gnu99 -Wall -Wextra -pedantic
CFLAGS += -O0 -ggdb
CFLAGS += $(shell pkg-config --cflags libsasl2)

LDFLAGS += $(shell pkg-config --libs libsasl2)

default: client server

clean:
	rm -f *.o
	rm -f client server
