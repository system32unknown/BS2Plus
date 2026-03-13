#include "console.h"
#include "config.h"
#include <fstream>
#include "win.h"

#ifdef COMPILER_SDL_TTF
TTF_Font* console_font;
#endif
SDL_Surface* console_surface;
char* console_text;
char* currentFilename;
std::ofstream file;
bool consolenews;

void initConsole() {
#ifdef COMPILER_SDL_TTF
	console_font = TTF_OpenFont(checkfilename("font.ttf"), 12);
#endif
	console_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 2000, 200, 16, 0, 0, 0, 0);
	file.open(checkfilename("console.txt"), std::ios::out);
}

void addConsoleTextLine(char* txt) {
	if (txt == NULL) return;
#ifdef COMPILER_SDL_TTF
	SDL_CopyRect16(console_surface, 0, 14, console_surface->w, console_surface->h - 14, 0, 0);
	SDL_DrawFilledRect16(console_surface, 0, console_surface->h - 14, 10000, 10000, SDL_MapRGB(console_surface->format, 0, 0, 0));
	SDL_DrawText16(console_surface, console_font, txt, 2, console_surface->h - 2, SDL_MapRGB(console_surface->format, 255, 255, 255));
#endif
	file.write(txt, strlen(txt));
	file.write("\n", 1);
	file.flush();
	consolenews = true;
}

SDL_Surface* getConsoleSurface() {
	return console_surface;
}
