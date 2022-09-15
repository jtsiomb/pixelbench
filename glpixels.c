#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#define GL_GLEXT_PROTOTYPES	1
#include <GL/glut.h>

enum {
	MODE_POINTS,
	MODE_DRAWPIX,
	MODE_TEXQUAD,
	MODE_TEXTRI,

	NUM_MODES
};

static const char *modestr[] = { "GL_POINTS", "glDrawPixels", "textured quad",
	"textured triangle"};

int init(void);
void display(void);
void idle(void);
void reshape(int x, int y);
void keyb(unsigned char key, int x, int y);
void change_mode(int m);

int win_width, win_height;
int max_xscroll, max_yscroll;

#define IMG_W	1024
#define IMG_H	1024
unsigned int img[IMG_W * IMG_H];
float *varr, *carr;
unsigned int vbo_pos, vbo_col;
int varr_sz, carr_sz;
unsigned int tex, quad, tri;

int mode = MODE_POINTS;

int have_vbo = 1;	/* TODO */

unsigned int start_tm;
unsigned int num_frames;


int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("GL pixel drawing methods");

	change_mode(mode);

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyb);
	glutIdleFunc(idle);

	if(init() == -1) {
		return 1;
	}

	start_tm = glutGet(GLUT_ELAPSED_TIME);
	glutMainLoop();
	return 0;
}


int init(void)
{
	int i, j, xor, r, g, b;
	unsigned int *ptr;
	float *vptr;

	ptr = img;
	for(i=0; i<IMG_H; i++) {
		for(j=0; j<IMG_W; j++) {
			xor = i ^ j;
			r = (xor >> 1) & 0xff;
			g = xor & 0xff;
			b = (xor << 1) & 0xff;
			*ptr++ = r | (g << 8) | (b << 16);
		}
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, IMG_W);

	win_width = glutGet(GLUT_WINDOW_WIDTH);
	win_height = glutGet(GLUT_WINDOW_HEIGHT);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, win_width, win_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	varr_sz = win_width * win_height * sizeof *varr * 2;
	if(!(varr = malloc(varr_sz))) {
		fprintf(stderr, "failed to allocate vertex array\n");
		return -1;
	}
	carr_sz = win_width * win_height * sizeof *carr * 3;
	if(!(carr = malloc(carr_sz))) {
		fprintf(stderr, "failed to allocate color array\n");
		return -1;
	}

	vptr = varr;
	for(i=0; i<win_height; i++) {
		for(j=0; j<win_width; j++) {
			vptr[0] = j;
			vptr[1] = i;
			vptr += 2;
		}
	}

	if(have_vbo) {
		glGenBuffers(1, &vbo_pos);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
		glBufferData(GL_ARRAY_BUFFER, varr_sz, varr, GL_STATIC_DRAW);

		glGenBuffers(1, &vbo_col);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_col);
		glBufferData(GL_ARRAY_BUFFER, carr_sz, 0, GL_STREAM_DRAW);
	}

	quad = glGenLists(1);
	glNewList(quad, GL_COMPILE);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(0, 0);
	glTexCoord2f(1, 0); glVertex2f(1, 0);
	glTexCoord2f(1, 1); glVertex2f(1, 1);
	glTexCoord2f(0, 1); glVertex2f(0, 1);
	glEnd();
	glEndList();

	tri = glGenLists(1);
	glNewList(tri, GL_COMPILE);
	glBegin(GL_TRIANGLES);
	glTexCoord2f(0, 0); glVertex2f(0, 0);
	glTexCoord2f(2, 0); glVertex2f(2, 0);
	glTexCoord2f(0, 2); glVertex2f(0, 2);
	glEnd();
	glEndList();

	return 0;
}

void display(void)
{
	int i, j;
	unsigned int tm = glutGet(GLUT_ELAPSED_TIME);
	unsigned int interv;
	float t = tm / 256.0f;
	int xoffs = (int)((sin(t) * 0.5f + 0.5f) * max_xscroll);
	int yoffs = (int)((cos(t) * 0.5f + 0.5f) * max_yscroll);
	unsigned int *start = img + yoffs * IMG_W + xoffs;
	float *vptr = varr;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	switch(mode) {
	case MODE_POINTS:
		/* draw with points */
		vptr = carr;
		for(i=0; i<win_height; i++) {
			for(j=0; j<win_width; j++) {
				int r = start[j] & 0xff;
				int g = (start[j] >> 8) & 0xff;
				int b = (start[j] >> 16) & 0xff;
				vptr[0] = r / 255.0f;
				vptr[1] = g / 255.0f;
				vptr[2] = b / 255.0f;
				vptr += 3;
			}
			start += IMG_W;
		}

		if(have_vbo) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
			glVertexPointer(2, GL_FLOAT, 0, 0);
			glBindBuffer(GL_ARRAY_BUFFER, vbo_col);
			glBufferSubData(GL_ARRAY_BUFFER, 0, carr_sz, carr);
			glColorPointer(3, GL_FLOAT, 0, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		} else {
			glVertexPointer(2, GL_FLOAT, 0, varr);
			glColorPointer(3, GL_FLOAT, 0, carr);
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		glDrawArrays(GL_POINTS, 0, win_width * win_height);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		break;

	case MODE_DRAWPIX:
		/* draw with glDrawPixels */
		glDrawPixels(win_width, win_height, GL_RGBA, GL_UNSIGNED_BYTE, start);
		break;

	case MODE_TEXQUAD:
	case MODE_TEXTRI:
		/* draw with textured quad or triangle */
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, win_width, win_height, GL_RGBA,
				GL_UNSIGNED_BYTE, start);
		glEnable(GL_TEXTURE_2D);
		glScalef(win_width, win_height, 1);
		glCallList(mode == MODE_TEXQUAD ? quad : tri);
		glDisable(GL_TEXTURE_2D);
		break;
	}

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);

	tm = glutGet(GLUT_ELAPSED_TIME);
	interv = tm - start_tm;
	if(++num_frames == 1000 || interv > 5000) {
		unsigned int fps = 100000 * num_frames / interv;
		printf("%s: %.2f fps\n", modestr[mode], fps / 100.0f);
		num_frames = 0;
		start_tm = tm;

		if(mode < NUM_MODES - 1) {
			change_mode(mode + 1);
		} else {
			exit(0);
		}
	}
}

void idle(void)
{
	glutPostRedisplay();
}

void reshape(int x, int y)
{
	glViewport(0, 0, x, y);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, x, 0, y, -1, 1);

	win_width = x;
	win_height = y;
	max_xscroll = IMG_W - win_width;
	max_yscroll = IMG_H - win_height;
	if(max_xscroll < 0) max_xscroll = 0;
	if(max_yscroll < 0) max_yscroll = 0;
}

void keyb(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);

	case ' ':
		change_mode((mode + 1) % NUM_MODES);
		break;

	default:
		break;
	}
}

void change_mode(int m)
{
	char title[128];
	mode = m;
	sprintf(title, "GL pixel drawing test: %s\n", modestr[mode]);
	glutSetWindowTitle(title);
}
