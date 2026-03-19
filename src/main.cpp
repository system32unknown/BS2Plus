#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <array>
#include <string>
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

#define SHOWGUI

unsigned int lastdrawtime = 0;
unsigned int lastflip = 0;
SDL_Surface* screen = nullptr;
unsigned int keydown[1024] = {};
unsigned int keylastexec[1024] = {};

const char* TITLE_GAME = "BS2CE";

void updatescreen() {
	msectimer();
	checknetwork();
	findTrigger("STATUS", 0)->exec();
	lastflip = SDL_GetTicks();
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

int updatethread(void*) {
	while (threadrun()) {
		SDL_Delay(1);
		updatescreen();
	}
	setthreadrun(true);
	exit(0);
	return 0;
}

int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_VIDEO);

	bool showgui = true;
	if (!isserver)
		for (int i = 1; i < argc; i++)
			if (strstr(argv[i], "bs2addon://") == argv[i])
				showgui = false;

	if (showgui) {
#ifdef COMPILER_WINDOWS
		constexpr std::array<int, 2> SRCSIZE = { 486, 504 };
#else
		constexpr std::array<int, 2> SRCSIZE = { 240, 320 };
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

	for (int i = 0; i < MAX_STRINGS; i++) strings[i] = nullptr;

	SDL_EnableKeyRepeat(1, 0);
	for (int i = 0; i < 1024; i++)
		keydown[i] = 0;

	sandsem = SDL_CreateSemaphore(1);
	screensem = SDL_CreateSemaphore(1);

	stamps = new SDL_Surface * [MAX_STAMPS * 2];
	for (int i = 0; i < MAX_STAMPS * 2; i++)
		stamps[i] = nullptr;

	debugparameter = reinterpret_cast<Var*>(setVar("DEBUGPARAMETER", 0));
	debugvar = reinterpret_cast<Var*>(setVar("DEBUGVAR", 0));
	debugframe = reinterpret_cast<Var*>(setVar("DEBUGFRAME", 0));

	msectimer();
	frametimer();

	int* nullparams = new int[10]();
	addparams(nullparams);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	char* defaultconfigfile = "config.bs2"; {
		std::ifstream inp(checkfilename("myconfig.bs2"), std::ifstream::in);
		if (inp) defaultconfigfile = "myconfig.bs2";
	}

	initmenu(screen);

	setVar("BSVERSION", 1);
	setVar("BSNIGHTLY", 1);
	setVar("MODDED", 1);
	SDL_WM_SetCaption(TITLE_GAME, "");
	SDL_WM_SetIcon(SDL_LoadBMP(checkfilename("icon.bmp")), nullptr);

	constexpr int z = 1;
	setVar("DEFAULTWIDTH", 420);
	setVar("DEFAULTHEIGHT", 420);
	setVar("DEFAULTMENUWIDTH", 33);
	initsand(420 / z, 420 / z);
	setVar("ZOOM", z);

	static Var* update = reinterpret_cast<Var*>(setVar("UPDATEVIEW", 20));
	static Var* speed = reinterpret_cast<Var*>(setVar("SPEED", 0));
	static Var* lastfps = reinterpret_cast<Var*>(setVar("FPS", 0));

#if defined(_WIN32)
	setVar("WINDOWS", 1);
#else
	setVar("WINDOWS", 0);
#endif
	static Var* keycode = reinterpret_cast<Var*>(setVar("SHOWKEYCODE", 0));
	static Var* keyrepeatdelay = reinterpret_cast<Var*>(setVar("KEYREPEATDELAY", SDL_DEFAULT_REPEAT_DELAY / 10));
	static Var* keyrepeatrate = reinterpret_cast<Var*>(setVar("KEYREPEATRATE", SDL_DEFAULT_REPEAT_INTERVAL / 10));
	static Var* vphysics = reinterpret_cast<Var*>(setVar("PHYSICS", 1));

	static Var* vyear = reinterpret_cast<Var*>(setVar("YEAR", 0));
	static Var* vmonth = reinterpret_cast<Var*>(setVar("MONTH", 0));
	static Var* vday = reinterpret_cast<Var*>(setVar("DAY", 0));
	static Var* vwday = reinterpret_cast<Var*>(setVar("WEEKDAY", 0));
	static Var* vhour = reinterpret_cast<Var*>(setVar("HOUR", 0));
	static Var* vminute = reinterpret_cast<Var*>(setVar("MINUTE", 0));
	static Var* vsecond = reinterpret_cast<Var*>(setVar("SECOND", 0));
	static Var* vunixtime = reinterpret_cast<Var*>(setVar("UNIXTIME", 0));
	static Var* rclickmod = reinterpret_cast<Var*>(setVar("RCLICK", 0));
	static Var* mclickmod = reinterpret_cast<Var*>(setVar("MCLICK", 0));

	Trigger* precalcTrigger = findTrigger("PREPHYSICS", 0);
	Trigger* postcalcTrigger = findTrigger("POSTPHYSICS", 0);
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

	{
		std::string welcome = "Welcome to ";
		welcome += TITLE_GAME;
		print(welcome.data(), 0);
	}
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
			memmove(argv[i] + 7, argv[i] + 11, argLen - 11 + 1);
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

	unsigned int second = SDL_GetTicks() / 1000;
	int fps = 0;
	float steps = 0.0f;

	lastdrawtime = SDL_GetTicks();
	while (lastdrawtime == SDL_GetTicks());
	const int minTickTime = static_cast<int>(SDL_GetTicks()) - static_cast<int>(lastdrawtime);

	lastflip = 0;

	static Var* lastactionmsec = reinterpret_cast<Var*>(setVar("LASTACTIONMSEC", 0));
	static Var* lastactionmframe = reinterpret_cast<Var*>(setVar("LASTACTIONFRAME", 0));
	static Var* varframe = reinterpret_cast<Var*>(setVar("FRAME", 0));
	static Var* varmsec = reinterpret_cast<Var*>(setVar("MSEC", 0));
	static Var* fullscreen = reinterpret_cast<Var*>(setVar("FULLSCREEN", 0));
	static Var* fullscreenx = reinterpret_cast<Var*>(setVar("FULLSCREENX", 0));
	static Var* fullscreeny = reinterpret_cast<Var*>(setVar("FULLSCREENY", 0));

	int lastfullscreen = 0;
	int lastfullscreenx = 0;
	int lastfullscreeny = 0;

	lastfps->value = 0;

	const int mp = 0;
	if (mp) SDL_CreateThread(updatethread, nullptr);

	Trigger* secondtimer = findTrigger("SECOND", 0);

	bool done = false;
	while (!done) {
#ifdef COMPILER_WINDOWS
		{
			time_t now = time(nullptr);
			tm* ltime = localtime(&now);
			vunixtime->value = static_cast<int>(now);
			vyear->value = ltime->tm_year + 1900;
			vmonth->value = ltime->tm_mon;
			vday->value = ltime->tm_mday;
			vwday->value = ltime->tm_wday;
			vhour->value = ltime->tm_hour;
			vminute->value = ltime->tm_min;
			vsecond->value = ltime->tm_sec;
		}
#endif
		const unsigned int now = SDL_GetTicks();
		const unsigned int delay = now - lastdrawtime;
		lastdrawtime = now;
		if (delay) steps += static_cast<float>(delay) * speed->value / 1000.0f;

		if ((steps < 0.0f) && (1000 / minTickTime > speed->value))
			SDL_Delay(1);

		if ((fullscreen->value != lastfullscreen) || (fullscreenx->value != lastfullscreenx) || (fullscreeny->value != lastfullscreeny)) {
#ifdef COMPILER_WINDOWS
			if (!fullscreenx->value) fullscreenx->value = GetSystemMetrics(SM_CXSCREEN);
			if (!fullscreeny->value) fullscreeny->value = GetSystemMetrics(SM_CYSCREEN);
#else
			if (!fullscreenx->value) fullscreenx->value = 640;
			if (!fullscreeny->value) fullscreeny->value = 480;
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

		if ((steps > 0.0f) && speed->value) {
			steps--;
			precalcTrigger->exec();
			if (vphysics->value) calc();
			postcalcTrigger->exec();
			pretimer->exec();
			frametimer();
			posttimer->exec();
			fps++;
			if (!mp && (static_cast<int>(SDL_GetTicks() - lastflip) > update->value)) {
				preupdatescreen->exec();
				updatescreen();
				postupdatescreen->exec();
			}
		} else if (speed->value == 0) {
			if (!mp && (static_cast<int>(SDL_GetTicks() - lastflip) > update->value)) {
				preupdatescreen->exec();
				updatescreen();
				postupdatescreen->exec();
			}
		}

		const float maxSteps = static_cast<float>(minTickTime) * speed->value / 1000.0f;
		if (steps > maxSteps) steps = maxSteps;

		if (second != SDL_GetTicks() / 1000) {
			lastfps->value = fps;
			fps = 0;
			second = SDL_GetTicks() / 1000;
			secondtimer->exec();
		}

		Uint8* keystate = SDL_GetKeyState(nullptr);
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				findTrigger("QUIT", 0)->exec();
				break;

			case SDL_KEYDOWN: {
				if (keycode->value == 1) {
					char keytmp[255];
					snprintf(keytmp, sizeof(keytmp), "Pressed Key: %i", event.key.keysym.sym);
					print(keytmp, 0);
				}
				const int kd = event.key.keysym.sym;
				if (!keydown[kd]) {
					execkey("KEY_", kd);
					keydown[kd] = static_cast<unsigned int>(keyrepeatdelay->value);
				} else {
					if (keydown[kd] <= 1) {
						execkey("KEYREPEAT_", kd);
						keydown[kd] = static_cast<unsigned int>(keyrepeatrate->value);
					} else {
						keydown[kd]--;
					}
				}
				break;
			}

			case SDL_KEYUP: {
				const int ku = event.key.keysym.sym;
				execkey("KEYUP_", ku);
				keydown[ku] = 0;
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
				if (keystate[rclickmod->value])
					clickmenu(screen, event.button.x, event.button.y, 4, true);
				else if (keystate[mclickmod->value])
					clickmenu(screen, event.button.x, event.button.y, 2, true);
				else
					clickmenu(screen, event.button.x, event.button.y, SDL_GetMouseState(nullptr, nullptr), true);
				break;

			case SDL_MOUSEBUTTONUP:
				if (event.button.button == 4)
					findTrigger("MOUSEWHEELUP", 0)->exec();
				if (event.button.button == 5)
					findTrigger("MOUSEWHEELDOWN", 0)->exec();
				break;

			case SDL_VIDEORESIZE: {
				int w = event.resize.w + 1;
				int h = event.resize.h + 1;
				w = std::max(200, std::min(w, 2000));
				h = std::max(200, std::min(h, 2000));
				SDL_FreeSurface(screen);
				screen = SDL_SetVideoMode(w - 1, h - 1, 16, SDL_RESIZABLE | SDL_HWSURFACE | SDL_DOUBLEBUF);
				autoresize(screen);
				hideSubMenu();
				redrawmenu(3);
				break;
			}

			case SDL_SYSWMEVENT:
				sysmessage(event.syswm.msg);
				break;
			}
		}

		int mousex = 0, mousey = 0;
		static Var* absx = reinterpret_cast<Var*>(setVar("ABSX", 0));
		static Var* absy = reinterpret_cast<Var*>(setVar("ABSY", 0));
		static int lastmousedown = 0;

		const int mousedown = static_cast<int>(SDL_GetMouseState(&mousex, &mousey));
		if (mousedown) {
			lastactionmsec->value = varmsec->value;
			lastactionmframe->value = varframe->value;
			if (lastmousedown) {
				if (keystate[rclickmod->value])
					clickmenu(screen, mousex, mousey, 4, false);
				else if (keystate[mclickmod->value])
					clickmenu(screen, mousex, mousey, 2, false);
				else
					clickmenu(screen, mousex, mousey, SDL_GetMouseState(&mousex, &mousey), false);
			}
		} else clickmenu(screen, mousex, mousey, SDL_GetMouseState(&mousex, &mousey), false);
		lastmousedown = mousedown;

		if ((absx->value != mousex) || (absy->value != mousey)) {
			lastactionmsec->value = varmsec->value;
			lastactionmframe->value = varframe->value;
		}
		absx->value = mousex;
		absy->value = mousey;

		static Var* ctrl = reinterpret_cast<Var*>(setVar("CTRL", 0));
		static Var* shift = reinterpret_cast<Var*>(setVar("SHIFT", 0));
		static Var* alt = reinterpret_cast<Var*>(setVar("ALT", 0));
		static Var* lctrl = reinterpret_cast<Var*>(setVar("LCTRL", 0));
		static Var* lshift = reinterpret_cast<Var*>(setVar("LSHIFT", 0));
		static Var* lalt = reinterpret_cast<Var*>(setVar("LALT", 0));
		static Var* rctrl = reinterpret_cast<Var*>(setVar("RCTRL", 0));
		static Var* rshift = reinterpret_cast<Var*>(setVar("RSHIFT", 0));
		static Var* ralt = reinterpret_cast<Var*>(setVar("RALT", 0));
		static Var* capslock = reinterpret_cast<Var*>(setVar("CAPSLOCK", 0));
		static Var* numlock = reinterpret_cast<Var*>(setVar("NUMLOCK", 0));

		for (int i = 0; i < 333; i++) {
			if (keystate[i]) {
				execkey("HOLDKEY_", i);
				if (i < 255) {
					lastactionmsec->value = varmsec->value;
					lastactionmframe->value = varframe->value;
				}
			}
		}

		const SDLMod kmods = SDL_GetModState();
		ctrl->value = (keystate[SDLK_LCTRL] || keystate[SDLK_RCTRL]) ? 1 : 0;
		shift->value = (keystate[SDLK_LSHIFT] || keystate[SDLK_RSHIFT]) ? 1 : 0;
		alt->value = (keystate[SDLK_LALT] || keystate[SDLK_RALT]) ? 1 : 0;
		capslock->value = (kmods & KMOD_CAPS) ? 1 : 0;
		numlock->value = (kmods & KMOD_NUM) ? 1 : 0;
		lalt->value = keystate[SDLK_LALT] ? 1 : 0;
		lshift->value = keystate[SDLK_LSHIFT] ? 1 : 0;
		lctrl->value = keystate[SDLK_LCTRL] ? 1 : 0;
		ralt->value = keystate[SDLK_RALT] ? 1 : 0;
		rshift->value = keystate[SDLK_RSHIFT] ? 1 : 0;
		rctrl->value = keystate[SDLK_RCTRL] ? 1 : 0;
	}

	return 0;
}