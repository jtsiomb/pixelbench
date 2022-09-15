#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <GL/glut.h>

enum {
	MODE_VERTEX,
	MODE_DRAWPIX,
	MODE_TEXQUAD,
	MODE_TEXTRI,

	NUM_MODES
};

int init(void);
void display(void);
void idle(void);
void reshape(int x, int y);
void keyb(unsigned char key, int x, int y);

int win_width, win_height;
int max_xscroll, max_yscroll;

#define IMG_W	1024
#define IMG_H	1024
unsigned int img[IMG_W * IMG_H];

int mode = MODE_DRAWPIX;

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("GL pixel drawing methods");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyb);
	glutIdleFunc(idle);

	if(init() == -1) {
		return 1;
	}
	glutMainLoop();
	return 0;
}


int init(void)
{
	int i, j, xor, r, g, b;
	unsigned int *ptr;
	unsigned int tex;

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
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, win_width, win_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	return 0;
}

void display(void)
{
	unsigned int tm = glutGet(GLUT_ELAPSED_TIME);
	float t = tm / 256.0f;
	int xoffs = (int)((sin(t) * 0.5f + 0.5f) * max_xscroll);
	int yoffs = (int)((cos(t) * 0.5f + 0.5f) * max_yscroll);
	unsigned int *start;

	switch(mode) {
	case MODE_VERTEX:
		/* draw with points */
		break;

	case MODE_DRAWPIX:
		/* draw with glDrawPixels */
		start = img + yoffs * IMG_W + xoffs;
		glDrawPixels(win_width, win_height, GL_RGBA, GL_UNSIGNED_BYTE, start);
		break;

	case MODE_TEXQUAD:
		/* draw with textured quad */
		break;

	case MODE_TEXTRI:
		/* draw with textured triangle */
		break;
	}

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
}

void idle(void)
{
	glutPostRedisplay();
}

void reshape(int x, int y)
{
	glViewport(0, 0, x, y);

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
		mode = (mode + 1) % NUM_MODES;
		break;

	default:
		break;
	}
}
