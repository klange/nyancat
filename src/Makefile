Deps_nyancat = src/nyancat.c
OBJECTS = nyancat.o

CC	?=
CFLAGS	 ?= -g -Wall -Wextra -std=c99 -pedantic -Wwrite-strings -MMD -MP
CPPFLAGS ?=
LDFLAGS  ?=
DEPFILES = $(OBJECTS:.o=.d)

all: nyancat

nyancat: $(OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@

clean: 
	-rm -f $(OBJECTS) nyancat $(DEPFILES)

check: all
	# Unit tests go here. None currently.
	@echo "*** ALL TESTS PASSED ***"

-include $(DEPFILES)

.PHONY: all clean check
