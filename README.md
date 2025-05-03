pixelbench
==========
Pixelbench is an experiment, to see how different methods of displaying a
framebuffer perform on various systems.

Author: John Tsiombikas <nuclear@mutantstargoat.com>

This program is placed in the public domain. Feel free to use, modify, and/or
redistribute it at will.

Methods tested:
  - OpenGL (`glpixels`)
    * dense cloud of colored points
    * glDrawPixels
    * Textured quadrilateral
    * Textured triangle
  - X11 (`xpixels`)
    * core `PutImage` request
    * Shared memory extension (`XShmPutImage`)
  - SDL 1.2 (`sdlpixels`)
  - SDL 2.0 (`sdl2pixels`)
    * Renderer API

Build instructions
------------------
Just type `./configure` and then `make`. It will attempt to build all the
different variations of the benchmark.

You can build a specific version by typing `make <binary name>`, where the
possible binary names are: `glpixels`, `xpixels`, `sdlpixels`, and `sdl2pixels`.

FAQ
---
  1. So what's the best method?

     It varies. Texture mapped triangles are the best on modern sytems, and you
     get scaling for free. `glDrawPixels` is the fastest method on all tested
     SGI computers, and very decent on modern systems too. SDL1 is identical to
     X-SHM, while SDL2 is identical to OpenGL with textured triangles.
     `XShmPutImage` is always clearly faster than `XPutImage`.
