#ifndef PICS_H
#define PICS_H

#include "compiler.h"
#include "SDL/SDL.h"

struct Pic {
	char type[255];
	char text[255];
	void* v;
	void* r;
	void* g;
	void* b;
	SDL_Surface* old;
	int staticint;

	SDL_Surface* getpic(int a = 0);
	void calc();
	char* toString();
};

Pic* getPic(char* type, char* text);
Pic* getPic(Pic* p);
void delPic(Pic* p);
void loadMenuFont(char* c, int s);

#endif