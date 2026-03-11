#ifndef SAND_H
#define SAND_H
#include "compiler.h"
#include "SDL.h"
#include "sdlbasics.h"
#include "console.h"
#include "elements.h"
#include "output.h"
#include <iostream>

#define MAX_SAND_HEIGHT 2000
#define MAX_SAND_WEIGHT 2000

SDL_Surface *getSandSurface();
SDL_Surface *getRealSandSurface();
void initsand(int w, int h);
void resize(int w, int h);

void sanddraw(Uint16 e, int b, int x, int y, int dx, int dy, int a1, int a2);
void sandwrite(Uint16 element, int x, int y, int size, char *text, int align);
void recalccolors(bool b = false);
void precalc(int p);
void setprecalc(int p);
void recalcused();
void calc();
void wind();
void pressure();
Uint16 getPixel(int x, int y);

extern SDL_sem *sandsem;
extern SDL_sem *screensem;
extern char *fontname;
extern int lastfontsize;
extern bool *used;

struct Wind
{
	float angle;
	int x;
	int y;
};

extern std::list<Wind *> winds;

#endif
