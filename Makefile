include cfg.mk

CFLAGS = -O3 $(CFLAGS_cfg) $(CFLAGS_user)
LDFLAGS = $(LDFLAGS_cfg) $(LDFLAGS_user)

.PHONY: all
all: glpixels xpixels sdlpixels sdl2pixels

glpixels: glpixels.o
	$(CC) -o $@ $< $(LDFLAGS) -lglut -lGLU -lGL -lX11 -lXext -lXmu -lm

xpixels: xpixels.o
	$(CC) -o $@ $< $(LDFLAGS) -lX11 -lXext -lm

sdlpixels: sdlpixels.o
	$(CC) -o $@ $< $(LDFLAGS) `sdl-config --libs` -lm

sdl2pixels: sdl2pixels.o
	$(CC) -o $@ $< $(LDFLAGS) `sdl2-config --libs` -lm


sdlpixels.o: sdlpixels.c
	$(CC) -o $@ -c $< `sdl-config --cflags` $(CFLAGS)

sdl2pixels.o: sdl2pixels.c
	$(CC) -o $@ -c $< `sdl2-config --cflags` $(CFLAGS)

.PHONY: clean
clean:
	rm -f glpixels xpixels sdlpixels sdl2pixels *.o
