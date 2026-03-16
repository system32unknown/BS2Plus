#ifndef COMPILER_H
#define COMPILER_H

#if defined(_WIN32)
#define COMPILER_WINDOWS
#endif

#define COMPILER_GCC
#define COMPILER_SYSTEM
#define USE_PNG
#define COMPILER_REMOVE
#define COMPILER_SDL_TTF
#define COMPILER_SDL_NET

#ifdef COMPILER_WINDOWS
#define RANDOMNUMBER rand()
#else
#define RANDOMNUMBER (rand() % 32768)
#endif

#define SEEDRAND srand

inline int myrand() {
	static int randseed = 0;
	randseed = randseed * 43 + 777;
	return randseed % 32768;
}

extern unsigned long int randseed;

inline int crand() {
	randseed = randseed * 1103515245 + 12345;
	return ((int)((randseed / 65536) % 32768));
}

inline void seedrand(int seed) {
	randseed = (unsigned long)seed;
}

#endif
