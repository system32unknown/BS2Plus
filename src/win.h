#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#define MAX_LENGTH 1024
#define MAX_SWITCHES 512

#include "compiler.h"
#include "SDL/SDL.h"

#ifdef COMPILER_WINDOWS
#include <windows.h>
#endif

void osinit(char* filename);
void sysmessage(SDL_SysWMmsg* msg);
void ossystem(char* cmd, char* parameters, bool wait = true, bool hidden = false);
void mousebuttonbug(bool mouseup);
int copyStringToClipboard(char* source);
char* getStringFromClipboard();
char* opendialog(char* filter, char* defaultname);
char* savedialog(char* filter, char* defaultname);
bool yesnobox(char* text, char* title);
void messagebox(char* text, char* title);
char* inputbox();
char* checkfilename(char* filename);

#endif
