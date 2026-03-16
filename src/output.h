#ifndef OUTPUT_H
#define OUTPUT_H

#include "compiler.h"
#include "console.h"

#define LOGLEVEL 3

// LOGLEVEL 0: Nothing
// LOGLEVEL 1: Critical Errors
// LOGLEVEL 2:
// LOGLEVEL 3: Config Errors
// LOGLEVEL 4:
// LOGLEVEL 5:
// LOGLEVEL 6: Events
// LOGLEVEL 7: Triggers
// LOGLEVEL 8:
// LOGLEVEL 9:

int error(char* text, int owner);
int print(char* text, int owner, int size = -1);

#endif