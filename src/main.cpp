#include <stdio.h>
#include <stdlib.h>
#include "SDL/SDL.h"
#include "SDL/SDL_syswm.h"
#include "SDL/SDL_net.h"
#include "SDL/SDL_thread.h"
#include "sdlbasics.h"
#include "menu.h"
#include "config.h"
#include "console.h"
#include "sand.h"
#include "trigger.h"
#include "loadsave.h"
#include "network.h"
#include "output.h"
#include "win.h"
#include "keys.h"
#include <time.h>

#define SHOWGUI

unsigned int lastdrawtime, lastflip;
SDL_Surface* screen;
unsigned int keydown[1024];
unsigned int keylastexec[1024];

char* TITLE_GAME = "BS2CE";

void updatescreen() {
	msectimer();
	checknetwork();
	findTrigger("STATUS", 0)->exec();
	lastflip = (SDL_GetTicks());
#ifdef SHOWGUI
	SDL_UnlockSurface(screen);
#endif
	recalccolors(true);
	drawmenu(screen);
#ifdef SHOWGUI
	SDL_Flip(screen);
	SDL_LockSurface(screen);
#endif
}

int updatethread(void* i) {
	i = 0;
	while (threadrun()) {
		SDL_Delay(1);
		updatescreen();
	}
	setthreadrun(true);
	exit(0);
	return 0;
}

int main(int argc, char* argv[]) {
	int mp = 0;

	SDL_Init(SDL_INIT_VIDEO);

	bool showgui = true;
	if (!isserver)
		if (argc > 1)
			for (int i = 1; i < argc; i++)
				if ((strstr(argv[i], "bs2addon://") == argv[i]))
					showgui = false;
	if (showgui) {
		std::array<int, 2> SRCSIZE;
#ifdef COMPILER_WINDOWS
		SRCSIZE = { 486, 504 };
#else
		SRCSIZE = { 240, 320 };
#endif
		screen = SDL_SetVideoMode(SRCSIZE[0], SRCSIZE[1], 16, SDL_RESIZABLE | SDL_HWSURFACE | SDL_DOUBLEBUF);
		mousebuttonbug(true);
	} else screen = SDL_CreateRGBSurface(SDL_SWSURFACE, 640, 480, 16, 0, 0, 0, 0);

	osinit(*argv);

#ifdef COMPILER_SDL_TTF
	TTF_Init();
	loadMenuFont(checkfilename("font.ttf"), 12);
#endif
	initelements();

	findGroup("None", true, -1);
	initConsole();
	initnetwork();
	for (int i = 0; i < MAX_STRINGS; i++)
		strings[i] = 0;

	SDL_EnableKeyRepeat(1, 0);
	for (int i = 0; i < 1024; i++)
		keydown[i] = false;

	sandsem = SDL_CreateSemaphore(1);
	screensem = SDL_CreateSemaphore(1);

	stamps = new SDL_Surface * [MAX_STAMPS * 2];
	for (int i2 = 0; i2 < MAX_STAMPS * 2; i2++)
		stamps[i2] = 0;

	debugparameter = (Var*)setVar("DEBUGPARAMETER", 0);
	debugvar = (Var*)setVar("DEBUGVAR", 0);
	debugframe = (Var*)setVar("DEBUGFRAME", 0);
	msectimer();
	frametimer();
	int* nullparams = new int[10];
	for (int i3 = 0; i3 < 10; i3++)
		nullparams[i3] = 0;
	addparams(nullparams);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	std::ifstream inp;
	inp.open(checkfilename("myconfig.bs2"), std::ifstream::in);
	inp.close();
	char* defaultconfigfile = "config.bs2";
	if (!inp.fail()) {
		defaultconfigfile = "myconfig.bs2";
	}

	initmenu(screen);

	setVar("BSVERSION", 1);
	setVar("BSNIGHTLY", 1);
	setVar("MODDED", 1);
	SDL_WM_SetCaption(TITLE_GAME, "");
	SDL_WM_SetIcon(SDL_LoadBMP(checkfilename("icon.bmp")), NULL);

	int z = 1;
	setVar("DEFAULTWIDTH", 420);
	setVar("DEFAULTHEIGHT", 420);
	setVar("DEFAULTMENUWIDTH", 33);
	initsand(420 / z, 420 / z);

	setVar("ZOOM", z);
	static Var* update = (Var*)setVar("UPDATEVIEW", 20);
	static Var* speed = (Var*)setVar("SPEED", 0);
	static Var* lastfps = (Var*)setVar("FPS", 0);
	(Var*)setVar("WINDOWS", 1);
	static Var* keycode = (Var*)setVar("SHOWKEYCODE", 0);
	static Var* keyrepeatdelay = (Var*)setVar("KEYREPEATDELAY", SDL_DEFAULT_REPEAT_DELAY / 10);
	static Var* keyrepeatrate = (Var*)setVar("KEYREPEATRATE", SDL_DEFAULT_REPEAT_INTERVAL / 10);
	static Var* vphysics = (Var*)setVar("PHYSICS", 1);

	static Var* vyear = (Var*)setVar("YEAR", 0);
	static Var* vmonth = (Var*)setVar("MONTH", 0);
	static Var* vday = (Var*)setVar("DAY", 0);
	static Var* vwday = (Var*)setVar("WEEKDAY", 0);
	static Var* vhour = (Var*)setVar("HOUR", 0);
	static Var* vminute = (Var*)setVar("MINUTE", 0);
	static Var* vsecond = (Var*)setVar("SECOND", 0);
	static Var* vunixtime = (Var*)setVar("UNIXTIME", 0);
	static Var* rclickmod = (Var*)setVar("RCLICK", 0);
	static Var* mclickmod = (Var*)setVar("MCLICK", 0);

	Trigger* precalc = findTrigger("PREPHYSICS", 0);
	Trigger* postcalc = findTrigger("POSTPHYSICS", 0);
	Trigger* pretimer = findTrigger("PRETIMER", 0);
	Trigger* posttimer = findTrigger("POSTTIMER", 0);
	Trigger* preupdatescreen = findTrigger("PREUPDATESCREEN", 0);
	Trigger* postupdatescreen = findTrigger("POSTUPDATESCREEN", 0);

	setthreadrun(false);
	showconsole(false);
	autoresize(screen);

	parsechar("ON KEY_ESC EXIT\n", 0);
	parsechar("ON KEY_SPACE EXIT\n", 0);
	parsechar("ON QUIT EXIT\n", 0);
	parsechar("KEYCODE 27 ESC\n", 0);
	parsechar("KEYCODE 32 SPACE\n", 0);

	std::string welcome = "Welcome to ";
	welcome += TITLE_GAME;
	print(welcome.data(), 0);
	consolenews = false;

	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

	bool parsedefault = true;
	bool configloaded = false;

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '/') continue;

		const size_t argLen = strlen(argv[i]);

		if (strncmp(argv[i], "bs2mod://", 9) == 0 && !configloaded) {
			parsefile(defaultconfigfile, 0);
			configloaded = true;
		}

		if (strncmp(argv[i], "bs2addon://", 11) == 0) {
			memmove(argv[i] + 7, argv[i] + 11, argLen - 11 + 1); // +1 for null terminator
			memcpy(argv[i], "http://", 7);

			if (!isserver) {
				int sock = connect("localhost", 7777);
				sendowner(argv[i], sock, strlen(argv[i]));
				disconnect(sock);
				exit(0);
			}

			if (!configloaded) {
				parsefile(defaultconfigfile, 0);
				configloaded = true;
			}
		}

		checkfile(argv[i], false);
		parsefile(argv[i], 0);
		parsedefault = false;
	}

	if (parsedefault) parsefile(defaultconfigfile, 0);
	bool done = false;

	unsigned int second = SDL_GetTicks() / 1000;
	int fps = 0;
	float steps = 0;
	lastdrawtime = (int)SDL_GetTicks();
	while (lastdrawtime == (unsigned int)SDL_GetTicks());
	int minTickTime = (int)SDL_GetTicks() - lastdrawtime;

	lastflip = 0;

	static Var* lastactionmsec = (Var*)setVar("LASTACTIONMSEC", 0);
	static Var* lastactionmframe = (Var*)setVar("LASTACTIONFRAME", 0);
	static Var* varframe = (Var*)setVar("FRAME", 0);
	static Var* varmsec = (Var*)setVar("MSEC", 0);

	static Var* fullscreen = (Var*)setVar("FULLSCREEN", 0);
	static Var* fullscreenx = (Var*)setVar("FULLSCREENX", 0);
	static Var* fullscreeny = (Var*)setVar("FULLSCREENY", 0);
	int lastfullscreen = 0;
	int lastfullscreenx = 0;
	int lastfullscreeny = 0;

	lastfps->value = 0;

	if (mp) SDL_CreateThread(updatethread, 0);

	Trigger* secondtimer = findTrigger("SECOND", 0);

	while (!done) {
#ifdef COMPILER_WINDOWS
		time_t now;
		time(&now);
		tm* ltime;
		ltime = localtime(&now);
		vunixtime->value = time(0);
		vyear->value = ltime->tm_year + 1900;
		vmonth->value = ltime->tm_mon;
		vday->value = ltime->tm_mday;
		vwday->value = ltime->tm_wday;
		vhour->value = ltime->tm_hour;
		vminute->value = ltime->tm_min;
		vsecond->value = ltime->tm_sec;
#endif
		unsigned int delay = (int)SDL_GetTicks() - lastdrawtime;
		lastdrawtime = SDL_GetTicks();
		if (delay) steps += ((float)delay) * speed->value / 1000;

		if ((steps < 0) && (1000 / minTickTime > speed->value))
			SDL_Delay(1);
		if ((fullscreen->value != lastfullscreen) || (fullscreenx->value != lastfullscreenx) || (fullscreeny->value != lastfullscreeny)) {
#ifdef COMPILER_WINDOWS
			if (!fullscreenx->value)
				fullscreenx->value = GetSystemMetrics(SM_CXSCREEN);
			if (!fullscreeny->value)
				fullscreeny->value = GetSystemMetrics(SM_CYSCREEN);
#else
			if (!fullscreenx->value)
				fullscreenx->value = 640;
			if (!fullscreeny->value)
				fullscreeny->value = 480;
#endif
			lastfullscreen = fullscreen->value;
			lastfullscreenx = fullscreenx->value;
			lastfullscreeny = fullscreeny->value;
			SDL_FreeSurface(screen);
			if (lastfullscreen) screen = SDL_SetVideoMode(lastfullscreenx, lastfullscreeny, 16, SDL_FULLSCREEN | SDL_RESIZABLE | SDL_HWSURFACE | SDL_DOUBLEBUF);
			else screen = SDL_SetVideoMode(483, 505, 16, SDL_RESIZABLE | SDL_HWSURFACE | SDL_DOUBLEBUF);
			autoresize(screen);
			hideSubMenu();
			redrawmenu(3);
		}
		if ((steps > 0) && speed->value) {
			steps--;
			precalc->exec();
			if (vphysics->value) calc();
			postcalc->exec();
			pretimer->exec();
			frametimer();
			posttimer->exec();
			fps++;
			if (!mp && ((int)(SDL_GetTicks() - lastflip) > update->value)) {
				preupdatescreen->exec();
				updatescreen();
				postupdatescreen->exec();
			}
		} else if (speed->value == 0)
			if (!mp && ((int)(SDL_GetTicks() - lastflip) > update->value)) {
				preupdatescreen->exec();
				updatescreen();
				postupdatescreen->exec();
			}
		if (steps > ((float)minTickTime) * speed->value / 1000)
			steps = ((float)minTickTime) * speed->value / 1000;

		if (second != SDL_GetTicks() / 1000) {
			lastfps->value = fps;
			fps = 0;
			second = SDL_GetTicks() / 1000;
			secondtimer->exec();
		}

		Uint8* keystate = SDL_GetKeyState(NULL);
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			int k = event.key.keysym.sym;
			switch (event.type) {
			case SDL_QUIT:
				findTrigger("QUIT", 0)->exec();
				break;
			case SDL_KEYDOWN:
				if (keycode->value == 1) {
					char keytmp[255];
					sprintf(keytmp, "Pressed Key: %i", event.key.keysym.sym);
					print(keytmp, 0);
				}
				k = event.key.keysym.sym;
				if (keydown[k] == false) {
					execkey("KEY_", k);
					keydown[k] = keyrepeatdelay->value;
				} else {
					if (keydown[k] <= 1) {
						execkey("KEYREPEAT_", k);
						keydown[k] = keyrepeatrate->value;
					} else {
						keydown[k]--;
					}
				}
				break;
			case SDL_KEYUP:
				k = event.key.keysym.sym;
				execkey("KEYUP_", k);
				keydown[k] = false;
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (keystate[rclickmod->value])
					clickmenu(screen, event.button.x, event.button.y, 4, true);
				else if (keystate[mclickmod->value])
					clickmenu(screen, event.button.x, event.button.y, 2, true);
				else
					clickmenu(screen, event.button.x, event.button.y, SDL_GetMouseState(NULL, NULL), true);
				break;
			case SDL_MOUSEBUTTONUP:
				if (event.button.button == 4)
					findTrigger("MOUSEWHEELUP", 0)->exec();
				if (event.button.button == 5)
					findTrigger("MOUSEWHEELDOWN", 0)->exec();
				break;
			case SDL_VIDEORESIZE:
			{
				int width = event.resize.w + 1;
				int height = event.resize.h + 1;
				if (width < 200) width = 200;
				if (width > 2000) width = 2000;
				if (height < 200) height = 200;
				if (height > 2000) height = 2000;
				SDL_FreeSurface(screen);
				screen = SDL_SetVideoMode(width - 1, height - 1, 16, SDL_RESIZABLE | SDL_HWSURFACE | SDL_DOUBLEBUF);
				autoresize(screen);
				hideSubMenu();
				redrawmenu(3);
			}
			break;
			case SDL_SYSWMEVENT:
				sysmessage(event.syswm.msg);
				break;
			}
		}
		int mousex, mousey, mousedown;
		static Var* absx = (Var*)setVar("ABSX", 0);
		static Var* absy = (Var*)setVar("ABSY", 0);
		static int lastmousedown = 0;
		if (mousedown = SDL_GetMouseState(&mousex, &mousey)) {
			lastactionmsec->value = varmsec->value;
			lastactionmframe->value = varframe->value;
			SDL_GetMouseState(&mousex, &mousey);
			if (lastmousedown) {
				if (keystate[rclickmod->value])
					clickmenu(screen, mousex, mousey, 4, !lastmousedown);
				else if (keystate[mclickmod->value])
					clickmenu(screen, mousex, mousey, 2, !lastmousedown);
				else
					clickmenu(screen, mousex, mousey, SDL_GetMouseState(&mousex, &mousey), !lastmousedown);
			}
		} else clickmenu(screen, mousex, mousey, SDL_GetMouseState(&mousex, &mousey), false);
		lastmousedown = mousedown;
		if ((absx->value != mousex) || (absy->value != mousey)) {
			lastactionmsec->value = varmsec->value;
			lastactionmframe->value = varframe->value;
		}
		absx->value = mousex;
		absy->value = mousey;
		static Var* ctrl = (Var*)setVar("CTRL", 0);
		static Var* shift = (Var*)setVar("SHIFT", 0);
		static Var* alt = (Var*)setVar("ALT", 0);
		static Var* lctrl = (Var*)setVar("LCTRL", 0);
		static Var* lshift = (Var*)setVar("LSHIFT", 0);
		static Var* lalt = (Var*)setVar("LALT", 0);
		static Var* rctrl = (Var*)setVar("RCTRL", 0);
		static Var* rshift = (Var*)setVar("RSHIFT", 0);
		static Var* ralt = (Var*)setVar("RALT", 0);
		static Var* capslock = (Var*)setVar("CAPSLOCK", 0);
		static Var* numlock = (Var*)setVar("NUMLOCK", 0);
		for (int i = 0; i < 333; i++)
			if (keystate[i]) {
				execkey("HOLDKEY_", i);
				if (i < 255) {
					lastactionmsec->value = varmsec->value;
					lastactionmframe->value = varframe->value;
				}
			}
		SDLMod kmods = SDL_GetModState();
		if (keystate[SDLK_LCTRL] || keystate[SDLK_RCTRL])
			ctrl->value = 1;
		else
			ctrl->value = 0;
		if (keystate[SDLK_LSHIFT] || keystate[SDLK_RSHIFT])
			shift->value = 1;
		else
			shift->value = 0;
		if (keystate[SDLK_LALT] || keystate[SDLK_RALT])
			alt->value = 1;
		else
			alt->value = 0;
		if (kmods & KMOD_CAPS)
			capslock->value = 1;
		else
			capslock->value = 0;
		if (kmods & KMOD_NUM)
			numlock->value = 1;
		else
			numlock->value = 0;
		if (keystate[SDLK_LALT])
			lalt->value = 1;
		else
			lalt->value = 0;
		if (keystate[SDLK_LSHIFT])
			lshift->value = 1;
		else
			lshift->value = 0;
		if (keystate[SDLK_LCTRL])
			lctrl->value = 1;
		else
			lctrl->value = 0;
		if (keystate[SDLK_RALT])
			ralt->value = 1;
		else
			ralt->value = 0;
		if (keystate[SDLK_RSHIFT])
			rshift->value = 1;
		else
			rshift->value = 0;
		if (keystate[SDLK_RCTRL])
			rctrl->value = 1;
		else
			rctrl->value = 0;
	}

	return 0;
}