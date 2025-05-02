#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>

enum { QUIT = 1, REDRAW = 2 };

enum {
	MODE_PUTIMAGE,
	MODE_XSHM,

	NUM_MODES
};

static const char *modestr[] = { "XPutImage", "XShmPutImage" };

int init(void);
void display(void);

static Window create_win(int width, int height, int bpp);
static void handle_event(XEvent *ev);
void change_mode(int m);
static void sig(int s);
int mask_to_shift(unsigned int mask);


int mode = MODE_PUTIMAGE;

Display *dpy;
Window win, root;
GC gc;
Visual *vis;
Atom xa_wm_proto, xa_wm_delwin;
int no_wm;
unsigned int time_msec;

XImage *ximg;
XShmSegmentInfo shm;
int wait_putimg;
int xshm_ev_completion;

unsigned int *framebuf;
int fb_pitch;
int fb_rshift, fb_gshift, fb_bshift;
unsigned int fb_rmask, fb_gmask, fb_bmask;

int win_width, win_height, mapped;
unsigned int pending;
int max_xscroll, max_yscroll;

#define IMG_W	1400
#define IMG_H	1200
unsigned int img[IMG_W * IMG_H];

unsigned int start_tm;
unsigned int num_frames;


int main(int argc, char **argv)
{
	int num_frames = 0;
	XEvent ev;
	struct timeval tv, tv0;
	char *env;

	if((env = getenv("XPIXELS_NO_WM"))) {
		if(isdigit(env[0])) {
			no_wm = atoi(env);
		} else {
			no_wm = 1;
		}
	}

	shm.shmid = -1;
	shm.shmaddr = (void*)-1;

	signal(SIGINT, sig);

	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to connect to the X server\n");
		return 1;
	}
	root = DefaultRootWindow(dpy);
	xa_wm_proto = XInternAtom(dpy, "WM_PROTOCOLS", 0);
	xa_wm_delwin = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);

	if(!XShmQueryExtension(dpy)) {
		fprintf(stderr, "X shared memory extension is not available\n");
		XCloseDisplay(dpy);
		return 1;
	}
	xshm_ev_completion = XShmGetEventBase(dpy) + ShmCompletion;

	if(!(win = create_win(800, 600, 24))) {
		return 1;
	}
	gc = XCreateGC(dpy, win, 0, 0);

	if(!(ximg = XShmCreateImage(dpy, vis, 24, ZPixmap, 0, &shm, 800, 600))) {
		fprintf(stderr, "failed to create shared memory image\n");
		goto end;
	}
	if((shm.shmid = shmget(IPC_PRIVATE, ximg->bytes_per_line * ximg->height, IPC_CREAT | 0777)) == -1) {
		fprintf(stderr, "failed to create shared memory block\n");
		goto end;
	}
	if((shm.shmaddr = ximg->data = shmat(shm.shmid, 0, 0)) == (void*)-1) {
		fprintf(stderr, "failed to attach shared memory block\n");
		goto end;
	}
	shm.readOnly = True;
	if(!XShmAttach(dpy, &shm)) {
		fprintf(stderr, "XShmAttach failed");
		goto end;
	}

	framebuf = (unsigned int*)ximg->data;
	fb_pitch = ximg->bytes_per_line;
	fb_rmask = ximg->red_mask;
	fb_gmask = ximg->green_mask;
	fb_bmask = ximg->blue_mask;
	fb_rshift = mask_to_shift(fb_rmask);
	fb_gshift = mask_to_shift(fb_gmask);
	fb_bshift = mask_to_shift(fb_bmask);

	change_mode(mode);

	if(init() == -1) {
		return 1;
	}

	gettimeofday(&tv0, 0);

	while(!(pending & QUIT)) {
		if(mapped) {/* && !wait_putimg) { */
			while(XPending(dpy)) {
				XNextEvent(dpy, &ev);
				handle_event(&ev);
				if(pending & QUIT) goto end;
			}

			if(!wait_putimg) {
				gettimeofday(&tv, 0);
				time_msec = (tv.tv_sec - tv0.tv_sec) * 1000 + (tv.tv_usec - tv0.tv_usec) / 1000;
				num_frames++;

				display();

				if(mode == MODE_XSHM) {
					XShmPutImage(dpy, win, gc, ximg, 0, 0, 0, 0, ximg->width, ximg->height, False);
				} else {
					XPutImage(dpy, win, gc, ximg, 0, 0, 0, 0, ximg->width, ximg->height);
				}
				XSync(dpy, False);
				/*wait_putimg = 1;*/
			}
		} else {
			XNextEvent(dpy, &ev);
			handle_event(&ev);
			if(pending & QUIT) goto end;
		}
	}

end:
	if(ximg) {
		XShmDetach(dpy, &shm);
		XDestroyImage(ximg);
		if(shm.shmaddr != (void*)-1) {
			shmdt(shm.shmaddr);
		}
		if(shm.shmid != -1) {
			shmctl(shm.shmid, IPC_RMID, 0);
		}
	}
	if(win) {
		XFreeGC(dpy, gc);
		XDestroyWindow(dpy, win);
	}
	XCloseDisplay(dpy);
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
	return 0;
}

void display(void)
{
	int i, j;
	unsigned int interv;
	float t = time_msec / 256.0f;
	int xoffs = (int)((sin(t) * 0.5f + 0.5f) * max_xscroll);
	int yoffs = (int)((cos(t) * 0.5f + 0.5f) * max_yscroll);
	unsigned int *start = img + yoffs * IMG_W + xoffs;
	unsigned int *dest;

	dest = framebuf;

	for(i=0; i<win_height; i++) {
		for(j=0; j<win_width; j++) {
			*dest++ = start[j];
		}
		start += IMG_W;
	}

	num_frames++;
	interv = time_msec - start_tm;
	if(interv >= 4000) {
		unsigned int fps = 100000 * num_frames / interv;
		printf("%s: %.2f fps\n", modestr[mode], fps / 100.0f);
		num_frames = 0;
		start_tm = time_msec;

		change_mode((mode + 1) % NUM_MODES);
	}
}


static Window create_win(int width, int height, int bpp)
{
	int scr, num_vis;
	Window win;
	XVisualInfo *vinf, vtmpl;
	unsigned int vinf_mask;
	XSetWindowAttributes xattr;
	XTextProperty txname;
	Colormap cmap;
	const char *name = "retrobench X11";

	scr = DefaultScreen(dpy);

	vtmpl.screen = scr;
	vtmpl.depth = bpp;
	vtmpl.class = bpp <= 8 ? PseudoColor : TrueColor;
	vinf_mask = VisualScreenMask | VisualDepthMask | VisualClassMask;
	if(!(vinf = XGetVisualInfo(dpy, vinf_mask, &vtmpl, &num_vis))) {
		fprintf(stderr, "failed to find appropriate visual for %d bpp\n", bpp);
		return 0;
	}
	vis = vinf->visual;

	if(!(cmap = XCreateColormap(dpy, root, vis, bpp <= 8 ? AllocAll : AllocNone))) {
		fprintf(stderr, "failed to allocate colormap\n");
		return 0;
	}

	xattr.background_pixel = BlackPixel(dpy, scr);
	xattr.colormap = cmap;
	xattr.override_redirect = no_wm ? True : False;
	win = XCreateWindow(dpy, root, 0, 0, width, height, 0, vinf->depth,
			InputOutput, vis, CWColormap | CWBackPixel | CWOverrideRedirect, &xattr);
	if(!win) return 0;

	XSelectInput(dpy, win, StructureNotifyMask | ExposureMask | KeyPressMask |
			KeyReleaseMask);

	XStringListToTextProperty((char**)&name, 1, &txname);
	XSetWMName(dpy, win, &txname);
	XSetWMIconName(dpy, win, &txname);
	XFree(txname.value);

	XSetWMProtocols(dpy, win, &xa_wm_delwin, 1);

	XMapWindow(dpy, win);
	return win;
}


static void handle_event(XEvent *ev)
{
	KeySym sym;

	switch(ev->type) {
	case MapNotify:
		mapped = 1;
		break;

	case UnmapNotify:
		mapped = 0;
		break;

	case Expose:
		pending |= REDRAW;
		break;

	case ConfigureNotify:
		if(ev->xconfigure.width != win_width || ev->xconfigure.height != win_height) {
			win_width = ev->xconfigure.width;
			win_height = ev->xconfigure.height;
			/* TODO */
		}
		break;

	case KeyPress:
		if((sym = XKeycodeToKeysym(dpy, ev->xkey.keycode, 0))) {
			if(sym == XK_Escape) {
				pending |= QUIT;
				break;
			}
			if(sym == XK_space) {
				change_mode((mode + 1) % NUM_MODES);
				break;
			}
		}
		break;

	case ClientMessage:
		if(ev->xclient.message_type == xa_wm_proto) {
			if(ev->xclient.data.l[0] == xa_wm_delwin) {
				pending |= QUIT;
			}
		}
		break;

	default:
		if(ev->type == xshm_ev_completion) {
			wait_putimg = 0;
		}
		break;
	}
}


void change_mode(int m)
{
	XTextProperty txname;
	char title[128], *ptr = title;
	mode = m;
	sprintf(title, "X11 pixel drawing test: %s\n", modestr[mode]);

	XStringListToTextProperty(&ptr, 1, &txname);
	XSetWMName(dpy, win, &txname);
	XSetWMIconName(dpy, win, &txname);
	XFree(txname.value);
}

static void sig(int s)
{
	pending |= QUIT;
}


int mask_to_shift(unsigned int mask)
{
	int s = 0;
	if(mask) {
		while(!(mask & 1)) {
			mask >>= 1;
			s++;
		}
	}
	return s;
}
