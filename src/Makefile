OBJECTS = nyancat.o

all: nyancat

nyancat: $(OBJECTS)
	$(CC) $(LFLAGS) $(OBJECTS) -o $@

clean:
	-rm -f $(OBJECTS) nyancat

check: all
	# Unit tests go here. None currently.
	@echo "*** ALL TESTS PASSED ***"

.PHONY: all clean check
