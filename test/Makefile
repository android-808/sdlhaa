CFLAGS:=-g -O0 -Wall

TEST_LDLIBS:=$(shell sdl-config --libs) -lSDL_haa
TEST_CFLAGS:=$(shell sdl-config --cflags)

TESTS:=basic multi alpha fullscreen switch

all: $(TESTS)

$(TESTS): %: %.o
	$(CC) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) $(TEST_LDLIBS) -o $@ $^
	
%.o: %.c
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -c -o $@ $^
	
clean:
	rm -f *.o $(TESTS)

