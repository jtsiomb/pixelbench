.PHONY: all clean
.PHONY: build-gl build-x11 build-sdl build-sdl2
.PHONY: clean-gl clean-x11 clean-sdl clean-sdl2

all: build-gl build-x11 build-sdl build-sdl2
clean: clean-gl clean-x11 clean-sdl clean-sdl2

build-gl:
	$(MAKE) -f Makefile.gl

build-x11:
	$(MAKE) -f Makefile.x11

build-sdl:
	$(MAKE) -f Makefile.sdl

build-sdl2:
	$(MAKE) -f Makefile.sdl2


clean-gl:
	$(MAKE) -f Makefile.gl clean

clean-x11:
	$(MAKE) -f Makefile.x11 clean

clean-sdl:
	$(MAKE) -f Makefile.sdl clean

clean-sdl2:
	$(MAKE) -f Makefile.sdl2 clean
