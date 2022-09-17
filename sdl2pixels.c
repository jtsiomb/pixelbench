#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>

int init(void);
void display(void);

int win_width, win_height;
int max_xscroll, max_yscroll;

#define IMG_W	1400
#define IMG_H	1200
unsigned int img[IMG_W * IMG_H];

unsigned int start_tm;
unsigned int num_frames;

int quit;
SDL_Window *win;
SDL_Renderer *sdlrend;
SDL_Texture *sdltex;


int main(int argc, char **argv)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

	if(SDL_CreateWindowAndRenderer(800, 600, 0, &win, &sdlrend) == -1) {
		fprintf(stderr, "failed to set video mode\n");
		return 1;
	}
	SDL_SetWindowTitle(win, "SDL pixel drawing benchmark");

	printf("SDL2 video driver: %s\n", SDL_GetCurrentVideoDriver());

	if(init() == -1) {
		return 1;
	}

	start_tm = SDL_GetTicks();
	while(!quit) {
		SDL_Event ev;
		while(SDL_PollEvent(&ev)) {
			switch(ev.type) {
			case SDL_QUIT:
				goto done;

			case SDL_KEYDOWN:
				if(ev.key.keysym.sym == 27) goto done;
				break;

			default:
				break;
			}
			if(quit) goto done;
		}

		display();
	}

done:
	SDL_Quit();
	return 0;
}


int init(void)
{
	int i, j, xor, r, g, b;
	unsigned int *ptr;

	ptr = img;
	for(i=0; i<IMG_H; i++) {
		for(j=0; j<IMG_W; j++) {
			xor = i ^ j;
			r = (xor >> 1) & 0xff;
			g = xor & 0xff;
			b = (xor << 1) & 0xff;
			*ptr++ = b | (g << 8) | (r << 16);
		}
	}

	win_width = 800;
	win_height = 600;

	max_xscroll = IMG_W - win_width;
	max_yscroll = IMG_H - win_height;

	if(!(sdltex = SDL_CreateTexture(sdlrend, SDL_PIXELFORMAT_XRGB8888,
					SDL_TEXTUREACCESS_STREAMING, win_width, win_height))) {
		fprintf(stderr, "failed to create SDL texture\n");
		return -1;
	}

	return 0;
}

void display(void)
{
	int i, j;
	unsigned int tm = SDL_GetTicks() - start_tm;
	unsigned int interv;
	float t = tm / 256.0f;
	int xoffs = (int)((sin(t) * 0.5f + 0.5f) * max_xscroll);
	int yoffs = (int)((cos(t) * 0.5f + 0.5f) * max_yscroll);
	unsigned int *start = img + yoffs * IMG_W + xoffs;

	SDL_UpdateTexture(sdltex, 0, start, IMG_W * 4);
	SDL_RenderCopy(sdlrend, sdltex, 0, 0);
	SDL_RenderPresent(sdlrend);

	num_frames++;
	tm = SDL_GetTicks();
	interv = tm - start_tm;
	if(interv >= 4000) {
		unsigned int fps = 100000 * num_frames / interv;
		printf("SDL2: %.2f fps\n", fps / 100.0f);
		num_frames = 0;
		start_tm = tm;
		quit = 1;
	}
}
