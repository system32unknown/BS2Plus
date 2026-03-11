#include "pics.h"
#include "elements.h"
#include <string.h>
#include "console.h"
#include <stack>
#include "win.h"
#include "menu.h"
std::stack<Pic *> picStack;

#ifdef COMPILER_SDL_TTF
TTF_Font *pic_font;
#endif

void loadMenuFont(char *c, int s)
{
#ifdef COMPILER_SDL_TTF
	pic_font = TTF_OpenFont(checkfilename(c), s);
#endif
}

SDL_Surface *getBMP(char *file)
{
	return SDL_LoadBMP(checkfilename(file));
}

SDL_Surface *Pic::getpic(int a)
{
	if (a > 127) return SDL_CreateRGBSurface(SDL_SWSURFACE, 16, 32, 32, 0, 0, 0, 0);
	if (!strcmp(type, "File") || !strcmp(type, "FILE"))
	{
		if (old == 0)
		{
			char tmp[1024];
			if (strstr(text, "/") || strstr(text, "\\"))
				sprintf(tmp, "%s", text);
			else
				sprintf(tmp, "img/%s", text);
			old = getBMP(tmp);
			if (old == NULL)
			{
#if LOGLEVEL >= 3
				char tmp2[1024];
				sprintf(tmp2, "Couldn't load bitmap \"%s\". Using empty bitmap.", tmp);
				error(tmp2, 0);
#endif
				old = SDL_CreateRGBSurface(SDL_SWSURFACE, 16, 32, 32, 0, 0, 0, 0);
			}
		}
		SDL_Surface *s = SDL_CreateRGBSurface(SDL_SWSURFACE, old->w, old->h, 32, 0, 0, 0, 0);
		SDL_BlitSurface(old, NULL, s, NULL);
		return s;
	}
	if (!strcmp(type, "ELEMENT"))
	{
		int i = 0;
		getVar(text, &i);
		if (getElement(i))
			return (getElement(i)->icon)->getpic(a + 1);
	}
#ifdef COMPILER_SDL_TTF
	if (!strcmp(type, "ELEMENTNAME"))
	{
		int i = 0;
		getVar(text, &i);
		if (getElement(i))
			return TTF_RenderText_Shaded(pic_font, getElement(i)->name, menufontfgcolor, menufontbgcolor);
	}
#endif
	if (!strcmp(type, "GROUP"))
	{
		int i = 0;
		getVar(text, &i);
		if (getGroup(i))
		{
			if (getGroup(i)->icon)
				return (getGroup(i)->icon)->getpic(a + 1);
			else
				return SDL_CreateRGBSurface(SDL_SWSURFACE, 16, 0, 0, 0, 255, 255, 0);
		}
	}
#ifdef COMPILER_SDL_TTF
	if (!strcmp(type, "GROUPNAME"))
	{
		int i = 0;
		getVar(text, &i);
		if (getGroup(i))
			return TTF_RenderText_Shaded(pic_font, getGroup(i)->name, menufontfgcolor, menufontbgcolor);
	}
#endif
	if (!strcmp(type, "HEX"))
	{
		SDL_Surface *s = SDL_CreateRGBSurface(SDL_SWSURFACE, 32, 32, 16, 0, 0, 0, 0);
		Uint16 c = SDL_MapRGB(s->format, ((Varint *)r)->val(), ((Varint *)g)->val(), ((Varint *)b)->val());
		Uint16 *bufp = (Uint16 *)s->pixels;
		for (int ii = 0; ii < 256; ii++)
		{
			unsigned char t = text[ii] - 48;
			if (t > 10)
				t -= 7;
			if (t > 7)
			{
				*(bufp + ii * 4) = c;
				t -= 8;
			}
			else
				*(bufp + ii * 4) = 0;
			if (t > 3)
			{
				*(bufp + ii * 4 + 1) = c;
				t -= 4;
			}
			else
				*(bufp + ii * 4 + 1) = 0;
			if (t > 1)
			{
				*(bufp + ii * 4 + 2) = c;
				t -= 2;
			}
			else
				*(bufp + ii * 4 + 2) = 0;
			if (t > 0)
			{
				*(bufp + ii * 4 + 3) = c;
			}
			else
				*(bufp + ii * 4 + 3) = 0;
		}
		return s;
	}
#ifdef COMPILER_SDL_TTF
	if ((!strcmp(type, "TEXT") || !strcmp(type, "Text")) && (strlen(text) > 0))
	{
		if (old == 0)
		{
			old = TTF_RenderText_Shaded(pic_font, text, menufontfgcolor, menufontbgcolor);
		}
		SDL_Surface *s = SDL_CreateRGBSurface(SDL_SWSURFACE, old->w, old->h, 32, 0, 0, 0, 0);
		SDL_BlitSurface(old, NULL, s, NULL);
		return s;
	}
	if (!strcmp(type, "NUMBER"))
	{
		char tmp[255];
		sprintf(tmp, "%i", ((Varint *)v)->val());
		SDL_Surface *destsurf = TTF_RenderText_Shaded(pic_font, tmp, menufontfgcolor, menufontbgcolor);
		return destsurf;
	}
	if (!strcmp(type, "STRING"))
	{
		if (strings[staticint])
		{
			SDL_Surface *destsurf = TTF_RenderText_Shaded(pic_font, strings[staticint], menufontfgcolor, menufontbgcolor);
			return destsurf;
		}
	}
#endif
	if (!strcmp(type, "STAMP"))
	{
		int stamp = staticint + MAX_STAMPS;
		if (!stamps[stamp])
			return SDL_CreateRGBSurface(SDL_SWSURFACE, 16, 32, 32, 0, 255, 255, 0);
		SDL_Surface *s = SDL_CreateRGBSurface(SDL_SWSURFACE, stamps[stamp]->w, stamps[stamp]->h, 32, 0, 0, 0, 0);
		SDL_BlitSurface(stamps[stamp], NULL, s, NULL);
		return s;
	}
	return SDL_CreateRGBSurface(SDL_SWSURFACE, 16, 32, 32, 0, 255, 255, 0);
}

void Pic::calc()
{
	if (!strcmp(type, "ELEMENT") || !strcmp(type, "GROUP") || !strcmp(type, "GROUPNAME") || !strcmp(type, "ELEMENTNAME") || !strcmp(type, "STATICNUMBER"))
	{
		char *t = new char[16];
		t[0] = 0;
		t[1] = 0;
		int i;
		getVar(text, &i);
		sprintf(t, "%i", i);
		strcpy(text, t);
	}
	if (!strcmp(type, "STAMP") || !strcmp(type, "STRING"))
	{
		char *x = new char[strlen(text) + 1];
		strcpy(x, text);
		v = new Varint(x);
		staticint = ((Varint *)v)->val();
		delete (((Varint *)v));
	}
	if (!strcmp(type, "NUMBER"))
	{
		char *x = new char[strlen(text) + 1];
		strcpy(x, text);
		v = new Varint(x);
	}
}

Pic *getPic(char *type, char *text)
{
	Pic *r;
	if (!picStack.empty())
	{
		r = picStack.top();
		picStack.pop();
	}
	else
	{
		r = new Pic();
		r->v = 0;
	}
	strcpy(r->type, type);
	strcpy(r->text, text);
	r->old = 0;
	return r;
}

Pic *getPic(Pic *p)
{
	Pic *r;
	if (!picStack.empty())
	{
		r = picStack.top();
		picStack.pop();
	}
	else
		r = new Pic();
	strcpy(r->type, p->type);
	strcpy(r->text, p->text);
	r->r = p->r;
	r->g = p->g;
	r->b = p->b;
	r->old = p->old;
	return r;
}

void delPic(Pic *p)
{
	picStack.push(p);
}

char *Pic::toString()
{
	char *tmp;
	if (!strcmp(type, "HEX"))
	{
		tmp = new char[1024 + strlen(type) + strlen(text) + strlen(((Varint *)r)->text) + strlen(((Varint *)g)->text) + strlen(((Varint *)b)->text)];
		sprintf(tmp, "%s \"%s\" %s %s %s", type, text, ((Varint *)r)->text, ((Varint *)g)->text, ((Varint *)b)->text);
	}
	else
	{
		tmp = new char[1024 + strlen(type) + strlen(text)];
		sprintf(tmp, "%s \"%s\"", type, text);
	}
	return tmp;
}
