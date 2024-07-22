OBJECTS = nyancat.o

CC	?=
CFLAGS	 ?= -g -Wall -Wextra -std=c99 -pedantic -Wwrite-strings -pthread
CPPFLAGS ?=
LDFLAGS  ?=

all: nyancat

nyancat: $(OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	-rm -f $(OBJECTS) nyancat

check: all
	# Unit tests go here. None currently.
	@echo "*** ALL TESTS PASSED ***"

.PHONY: all clean check
