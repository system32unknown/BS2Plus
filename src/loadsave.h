#ifndef LOADSAVE_H
#define LOADSAVE_H

#include "compiler.h"
#include "SDL/SDL.h"

void quicksave(int slot);
void quickload(int slot);
int save(SDL_Surface* screen, char* filename);
int load(char* filename);

#endif
