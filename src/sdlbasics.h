#ifndef SANDBASICS_H
#define SANDBASICS_H

#include "compiler.h"

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif
#include "SDL/SDL.h"
#ifdef COMPILER_SDL_TTF
#include "SDL/SDL_ttf.h"
#endif
#include <math.h>
#include "console.h"

#define BRUSH_FILLEDCIRCLE 0
#define BRUSH_CIRCLE 1
#define BRUSH_FILLEDRECT 2
#define BRUSH_RECT 3
#define BRUSH_LINE 4
#define BRUSH_FILL 5
#define REPLACE_FILLEDCIRCLE 6
#define REPLACE_LINE 7
#define COPY_RECT 8
#define ROTATE_RECT 9
#define BRUSH_POINT 10
#define BRUSH_RADNOMFILLEDCIRCLE 11
#define COPY_STAMP 12
#define PASTE_STAMP 13
#define BRUSH_SWAPPOINTS 14
#define BRUSH_FILLEDELLIPSE 15
#define BRUSH_ELLIPSE 16
#define REPLACE_FILLEDELLIPSE 17
#define BRUSH_RANDOMFILLEDELLIPSE 18
#define BRUSH_POINTS 19
#define BRUSH_OBJECT 20

#define MAX_STAMPS 100000

void SDL_DrawPoint16(SDL_Surface* screen, int x, int y, Uint16 color);
void SDL_DrawSavePoint16(SDL_Surface* screen, int x, int y, Uint16 color);
void SDL_DrawLine16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color);
void SDL_ReplaceLine16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color, Sint32 replace);
void SDL_DrawRect16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color);
void SDL_DrawFilledRect16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color);
void SDL_DrawCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, Uint16 color);
void SDL_DrawFilledCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, Uint16 color);
void SDL_DrawRandomFilledCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, int rate, Uint16 color);
void SDL_ReplaceFilledCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, Uint16 color, Sint32 replace);
void SDL_CopyRect16(SDL_Surface* screen, int x, int y, int dx, int dy, int tox, int toy);
void SDL_DrawText16(SDL_Surface* screen, void* f, char* t, int x, int y, Uint16 color, int* width = 0, int* height = 0, int align = 0);
void SDL_Fill16(SDL_Surface* screen, int x, int y, Uint16 color);
void SDL_RotateRect16(SDL_Surface* screen, int x, int y, int dx, int dy, int direction);
void SDL_CopyStamp16(SDL_Surface* screen, int x, int y, int dx, int dy, int stamp);
void SDL_PasteStamp16(SDL_Surface* screen, int x, int y, int dx, int dy, int stamp, Uint16 transparent);
void SDL_SwapPoints16(SDL_Surface* screen, int x, int y, int x2, int y2, bool sand);

extern SDL_Surface** stamps;
#endif
