#ifndef CONFIG_H
#define CONFIG_H

#include "compiler.h"
#include <iostream>
#include <fstream>
#include "elements.h"
#include "console.h"
#include "menu.h"
#include "trigger.h"
#include "vars.h"
#include "output.h"
#include "pics.h"

#define TOKEN_ATOM_QUOTE 1
#define TOKEN_ATOM_BRACKET1 2
#define TOKEN_ATOM_BRACKET2 3

class Token
{
private:
	long pos, length;

public:
	char *text;
	int atom;
	Token(char *text);
	char *getToken(bool math = false);
	char *getRest();
	void reset();
	char *getuntillast();
};

bool checkfile(char *text, bool doexit = true);
int parsefile(char *filename, int owner);
int parsechar(char *text, int owner, char *filename = 0);
int parseline(char *text, int linenum, int owner, char *filename = 0);
Varint **getParameters(Token *tokens, char *exception = 0);
Action *parseaction(Token *tokens, int owner);

#endif
