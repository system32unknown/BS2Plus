#include "output.h"
#include "network.h"

int error(char *text, int owner)
{
	if (owner)
	{
		sendowner(text, owner);
	}
	else
	{
		addConsoleTextLine(text);
		return 0;
	}
	return 0;
}

int print(char *text, int owner, int size)
{
	if (owner)
	{
		sendowner(text, owner, size);
	}
	else
	{
		addConsoleTextLine(text);
		return 0;
	}
	return 0;
}
