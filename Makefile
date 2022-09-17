obj = glpixels.o
bin = glpixels

CFLAGS = -pedantic -Wall -g
LDFLAGS = -lX11 -lGL -lglut -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
