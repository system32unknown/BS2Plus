#ifndef PICS_H
#define PICS_H

#include "compiler.h"
#include "SDL/SDL.h"

struct Pic {
	char type[255];
	char text[255];
	SDL_Surface* getpic(int a = 0);
	void calc();
	void* v;
	void* r, * g, * b;
	char* toString();
	SDL_Surface* old;
	int staticint;
};

Pic* getPic(char* type, char* text);
Pic* getPic(Pic* p);
void delPic(Pic* p);

void loadMenuFont(char* c, int s);

#endif
