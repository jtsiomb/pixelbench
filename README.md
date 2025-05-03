pixelbench
==========
Pixelbench is an experiment, to see how different methods of displaying a
framebuffer perform on various systems.

Author: John Tsiombikas <nuclear@mutantstargoat.com>

This program is placed in the public domain. Feel free to use, modify, and/or
redistribute it at will.

Build instructions
------------------
Just type `./configure` and then `make`. It will attempt to build all the
different variations of the benchmark.

You can build a specific version by typing `make <binary name>`, where the
possible binary names are: `glpixels`, `xpixels`, `sdlpixels`, and `sdl2pixels`.
