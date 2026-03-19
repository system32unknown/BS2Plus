#include "pics.h"
#include "elements.h"
#include <cstring>
#include "console.h"
#include <stack>
#include "win.h"
#include "menu.h"

std::stack<Pic*> picStack;

#ifdef COMPILER_SDL_TTF
TTF_Font* pic_font;
#endif

void loadMenuFont(char* c, int s) {
#ifdef COMPILER_SDL_TTF
	pic_font = TTF_OpenFont(checkfilename(c), s);
#endif
}

SDL_Surface* getBMP(char* file) {
	return SDL_LoadBMP(checkfilename(file));
}

SDL_Surface* Pic::getpic(int a) {
	if (a > 127) return SDL_CreateRGBSurface(SDL_SWSURFACE, 16, 32, 32, 0, 0, 0, 0);

	if (!strcmp(type, "File") || !strcmp(type, "FILE")) {
		if (old == nullptr) {
			char tmp[1024];
			if (strstr(text, "/") || strstr(text, "\\"))
				strncpy(tmp, text, sizeof(tmp) - 1);
			else snprintf(tmp, sizeof(tmp), "img/%s", text);
			tmp[sizeof(tmp) - 1] = 0;

			old = getBMP(tmp);
			if (old == nullptr) {
#if LOGLEVEL >= 3
				char tmp2[1024];
				snprintf(tmp2, sizeof(tmp2), "Couldn't load bitmap \"%s\". Using empty bitmap.", tmp);
				error(tmp2, 0);
#endif
				old = SDL_CreateRGBSurface(SDL_SWSURFACE, 16, 32, 32, 0, 0, 0, 0);
			}
		}
		SDL_Surface* s = SDL_CreateRGBSurface(SDL_SWSURFACE, old->w, old->h, 32, 0, 0, 0, 0);
		SDL_BlitSurface(old, nullptr, s, nullptr);
		return s;
	}

	if (!strcmp(type, "ELEMENT")) {
		int i = 0;
		getVar(text, &i);
		if (getElement(i)) return getElement(i)->icon->getpic(a + 1);
	}

#ifdef COMPILER_SDL_TTF
	if (!strcmp(type, "ELEMENTNAME")) {
		int i = 0;
		getVar(text, &i);
		if (getElement(i)) return TTF_RenderText_Shaded(pic_font, getElement(i)->name, menufontfgcolor, menufontbgcolor);
	}
#endif

	if (!strcmp(type, "GROUP")) {
		int i = 0;
		getVar(text, &i);
		if (getGroup(i)) {
			if (getGroup(i)->icon) return getGroup(i)->icon->getpic(a + 1);
			else return SDL_CreateRGBSurface(SDL_SWSURFACE, 16, 0, 0, 0, 255, 255, 0);
		}
	}

#ifdef COMPILER_SDL_TTF
	if (!strcmp(type, "GROUPNAME")) {
		int i = 0;
		getVar(text, &i);
		if (getGroup(i)) return TTF_RenderText_Shaded(pic_font, getGroup(i)->name, menufontfgcolor, menufontbgcolor);
	}
#endif

	if (!strcmp(type, "HEX")) {
		SDL_Surface* s = SDL_CreateRGBSurface(SDL_SWSURFACE, 32, 32, 16, 0, 0, 0, 0);
		const Uint16 c = SDL_MapRGB(s->format, ((Varint*)r)->val(), ((Varint*)g)->val(), ((Varint*)b)->val());
		Uint16* bufp = static_cast<Uint16*>(s->pixels);
		for (int ii = 0; ii < 256; ii++) {
			unsigned char t = static_cast<unsigned char>(text[ii]) - 48;
			if (t > 10) t -= 7;
			*(bufp + ii * 4 + 0) = (t > 7) ? (t -= 8, c) : 0;
			*(bufp + ii * 4 + 1) = (t > 3) ? (t -= 4, c) : 0;
			*(bufp + ii * 4 + 2) = (t > 1) ? (t -= 2, c) : 0;
			*(bufp + ii * 4 + 3) = (t > 0) ? c : 0;
		}
		return s;
	}

#ifdef COMPILER_SDL_TTF
	if ((!strcmp(type, "TEXT") || !strcmp(type, "Text")) && (strlen(text) > 0)) {
		if (old == nullptr)
			old = TTF_RenderText_Shaded(pic_font, text, menufontfgcolor, menufontbgcolor);
		SDL_Surface* s = SDL_CreateRGBSurface(SDL_SWSURFACE, old->w, old->h, 32, 0, 0, 0, 0);
		SDL_BlitSurface(old, nullptr, s, nullptr);
		return s;
	}

	if (!strcmp(type, "NUMBER")) {
		char tmp[255];
		snprintf(tmp, sizeof(tmp), "%i", reinterpret_cast<Varint*>(v)->val());
		return TTF_RenderText_Shaded(pic_font, tmp, menufontfgcolor, menufontbgcolor);
	}

	if (!strcmp(type, "STRING")) {
		if (strings[staticint])
			return TTF_RenderText_Shaded(pic_font, strings[staticint], menufontfgcolor, menufontbgcolor);
	}
#endif

	if (!strcmp(type, "STAMP")) {
		const int stamp = staticint + MAX_STAMPS;
		if (!stamps[stamp]) return SDL_CreateRGBSurface(SDL_SWSURFACE, 16, 32, 32, 0, 255, 255, 0);
		SDL_Surface* s = SDL_CreateRGBSurface(SDL_SWSURFACE, stamps[stamp]->w, stamps[stamp]->h, 32, 0, 0, 0, 0);
		SDL_BlitSurface(stamps[stamp], nullptr, s, nullptr);
		return s;
	}

	return SDL_CreateRGBSurface(SDL_SWSURFACE, 16, 32, 32, 0, 255, 255, 0);
}

void Pic::calc() {
	if (!strcmp(type, "ELEMENT") || !strcmp(type, "GROUP") || !strcmp(type, "GROUPNAME") || !strcmp(type, "ELEMENTNAME") || !strcmp(type, "STATICNUMBER")) {
		char t[16];
		int i = 0;
		getVar(text, &i);
		snprintf(t, sizeof(t), "%i", i);
		strncpy(text, t, sizeof(t));
	}

	if (!strcmp(type, "STAMP") || !strcmp(type, "STRING")) {
		char* x = new char[strlen(text) + 1];
		strcpy(x, text);
		Varint* varint = new Varint(x);
		staticint = varint->val();
		delete varint;
		v = nullptr;
	}

	if (!strcmp(type, "NUMBER")) {
		char* x = new char[strlen(text) + 1];
		strcpy(x, text);
		v = new Varint(x);
	}
}

Pic* getPic(char* type, char* text) {
	Pic* r;
	if (!picStack.empty()) {
		r = picStack.top();
		picStack.pop();
	} else r = new Pic();
	strcpy(r->type, type);
	strcpy(r->text, text);
	r->old = nullptr;
	r->v = nullptr;
	r->r = nullptr;
	r->g = nullptr;
	r->b = nullptr;
	r->staticint = 0;
	return r;
}

Pic* getPic(Pic* p) {
	Pic* r;
	if (!picStack.empty()) {
		r = picStack.top();
		picStack.pop();
	} else r = new Pic();
	strcpy(r->type, p->type);
	strcpy(r->text, p->text);
	r->r = p->r;
	r->g = p->g;
	r->b = p->b;
	r->old = p->old;
	r->v = p->v;
	r->staticint = p->staticint;
	return r;
}

void delPic(Pic* p) {
	picStack.push(p);
}

char* Pic::toString() {
	char* tmp;
	if (!strcmp(type, "HEX")) {
		const size_t len = 1024 + strlen(type) + strlen(text) + strlen(((Varint*)r)->text) + strlen(((Varint*)g)->text) + strlen(((Varint*)b)->text);
		tmp = new char[len];
		snprintf(tmp, len, "%s \"%s\" %s %s %s", type, text, ((Varint*)r)->text, ((Varint*)g)->text, ((Varint*)b)->text);
	} else {
		const size_t len = 1024 + strlen(type) + strlen(text);
		tmp = new char[len];
		snprintf(tmp, len, "%s \"%s\"", type, text);
	}
	return tmp;
}