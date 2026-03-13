#ifndef CONSOLE_H
#define CONSOLE_H

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif
#include "sdlbasics.h"

extern bool consolenews;

void initConsole();
void addConsoleTextLine(char* txt);
SDL_Surface* getConsoleSurface();

#endif