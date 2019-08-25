PREFIX:=$(shell sdl-config --prefix)
CFLAGS:=-g -O0 -Wall -Wextra

CC:=gcc
LIBTOOL:=libtool

RELEASE:=1.2
VERSION:=1:2:1

SDL_HAA_TARGET:=libSDL_haa.la

SDL_HAA_LDLIBS:=$(shell sdl-config --libs) $(shell pkg-config --libs x11 xext)
SDL_HAA_CFLAGS:=-DHAVE_XSHM \
	$(shell sdl-config --cflags) $(shell pkg-config --cflags x11 xext)
SDL_HAA_LDFLAGS:=-release $(RELEASE) -version-info $(VERSION) -rpath $(PREFIX)/lib

all: $(SDL_HAA_TARGET)

$(SDL_HAA_TARGET): SDL_haa.lo
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(SDL_HAA_LDFLAGS) $(LDLIBS) $(SDL_HAA_LDLIBS) -o $@ $^
	
SDL_haa.lo: SDL_haa.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(SDL_HAA_CFLAGS) -c $^
	
clean:
	$(LIBTOOL) --mode=clean rm -f *.o *.lo $(SDL_HAA_TARGET)
	
install: $(SDL_HAA_TARGET)
	install -d $(DESTDIR)$(PREFIX)/include/SDL $(DESTDIR)$(PREFIX)/lib
	install SDL_haa.h $(DESTDIR)$(PREFIX)/include/SDL/
	$(LIBTOOL) --mode=install install -c $(SDL_HAA_TARGET) $(DESTDIR)$(PREFIX)/lib/
ifeq ($(DESTDIR),)
	$(LIBTOOL) --mode=finish $(PREFIX)/lib
endif

uninstall:
	-$(LIBTOOL) --mode=uninstall rm $(DESTDIR)$(PREFIX)/lib/$(SDL_HAA_TARGET)
	-rm $(DESTDIR)$(PREFIX)/include/SDL/SDL_haa.h

