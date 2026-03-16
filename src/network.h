#ifndef NETWORK_H
#define NETWORK_H

#include "SDL/SDL.h"
#include "config.h"

extern bool isserver;
void initnetwork();
void checknetwork();
void sendowner(char* text, int owner, int size = -1);
int connect(char* address, int port);
void disconnect(int owner);

#endif