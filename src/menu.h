#ifndef MENU_H
#define MENU_H

#include "compiler.h"
#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif
#include "SDL/SDL.h"
#include <list>
#include "sdlbasics.h"
#include "console.h"
#include "sand.h"
#include "trigger.h"
#include "output.h"
#include "vars.h"

#define MENU_BAR_TOP 0
#define MENU_BAR_LEFT 1
#define MENU_BAR_RIGHT 2
#define MENU_BAR_BOTTOM 3
#define MENU_BAR_SUB 4

#define MENU_ALIGN_H 1
#define MENU_ALIGN_V 2

struct Button {
	Button();
	Button(Button* b);
	char* tiptext;
	Pic* icon;
	void* click;
	int* params;
	int top, left;
	int id;
	int mode;
	int r, g, b;
	bool border;
};

void initmenu(SDL_Surface* screen);
void drawmenu(SDL_Surface* screen);
void clickmenu(SDL_Surface* screen, int x, int y, int b, int click);
void showconsole(int visible);
void redrawmenu(int i);
void addButtonToMenuBar(int i, Button* button);
void clearMenuBar(int i);
int getmenuwidth(int i);
void checkscroll();
SDL_Surface* createmenu(std::list<Button*>* buttons, int align = 0, int width = 0, int height = 0, bool crop = false);
void showSubMenu(int stay, int align);
void showSubMenu(int stay, int align, int x, int y);
void hideSubMenu();
void scrollto(int x, int y);
char* getmouseover();
void autoresize(SDL_Surface* screen);

extern char status_text[4097];
extern SDL_Surface* fglayer;
extern SDL_Surface* bglayer;
extern SDL_Color menufontfgcolor;
extern SDL_Color menufontbgcolor;
#endif