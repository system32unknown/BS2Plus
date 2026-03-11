#ifndef KEYS_H
#define KEYS_H

#include <list>

struct Key
{
	int code;
	char *name;
};

void addkey(char *name, int keycode);
void execkey(char *type, int keycode);

#endif
