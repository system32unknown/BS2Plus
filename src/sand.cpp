#include "sand.h"
#include "menu.h"
#include "win.h"
#include <cstdint>
#define DOESEATELEMENTSMAX 256

SDL_Surface* sandSurface = 0;
SDL_Surface* outSurface = 0;
SDL_Surface* pressureSurface = 0;

int doutSurface;
int dpressureSurface;

int gravity;

int brush;
int brushsize, reverse;
Var* rev;

int height;
int width;
Uint16* bufp;
int ypitch;

int rl;

int element1, element2, element3;

int recalc;
bool mustredraw = true;

Uint16* color;

int* weight;
int* aweight;
int* weightreverse;
int* directionypitch;
int* spray;
int* slide;
int* side;
int* viscousity;
int* direction;
bool* used;
bool* oldused;
bool* skip;
bool* eat;
Uint16* eatmax;
Uint16** eatspeed;
Uint16** eatelement;
Uint16** eattoself;
bool** eattrigger;
uintptr_t** eattoother;
Uint16** eatnot;
bool** doeseatelement;
Uint16** life;
Uint16** dieto;
Uint16* dierate;
bool* nobias;
Var* varborder = 0;
Var* varinteractions = 0;
Var* vardeath = 0;
Var* vargravity = 0;
Var* varwind = 0;
Var* seed = 0;

int sandelements;
int oldinteractions, olddeath, oldgravity;
void* outSurfacep;
int outSurfaceh;
SDL_sem* sandsem;
SDL_sem* screensem;

char* fontname = "font.ttf";
int lastfontsize = -1;
std::list<Wind*> winds;

void initsand(int w, int h) {
	varborder = (Var*)setVar("BORDER", 0);
	varinteractions = (Var*)setVar("INTERACTIONS", 1);
	vardeath = (Var*)setVar("DEATH", 1);
	vargravity = (Var*)setVar("GRAVITY", 1000);
	varwind = (Var*)setVar("WIND", 0);
	seed = (Var*)setVar("SEED", 0);
	reverse = -1;
	gravity = 1;

	resize(w, h);
	rev = (Var*)setVar("REVERSE", 0);
	sandelements = 0;
	precalc(3);
}

void resize(int w, int h) {
	SDL_SemWait(screensem);
	static Var* wVar = (Var*)setVar("WIDTH", w);
	static Var* hVar = (Var*)setVar("HEIGHT", h);
	if (outSurface) {
		outSurface->h = outSurfaceh;
		outSurface->pixels = outSurfacep;
		SDL_FreeSurface(outSurface);
	}
	wVar->value = w;
	hVar->value = h;
	SDL_Surface* oldsandSurface = sandSurface;
	sandSurface = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCCOLORKEY | SDL_SRCALPHA, w + 2, h + 2, 16, 0, 0, 0, 0);
	outSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, w + 2, h + 2, 16, 0, 0, 0, 0);

	Uint16* p = (Uint16*)outSurface->pixels, * end = p + (sandSurface->pitch / 2 * (h - 1) + w - 1);
	for (; p < end; p++)
		*p = 0;

	height = sandSurface->h - 2;
	width = sandSurface->w;
	int oldypitch = ypitch;
	ypitch = sandSurface->pitch / 2;
	outSurfaceh = outSurface->h;
	outSurface->h = h;
	outSurfacep = outSurface->pixels;
	outSurface->pixels = ((Uint16*)outSurface->pixels + (ypitch));
	doutSurface = (Uint16*)outSurface->pixels - (Uint16*)sandSurface->pixels - ypitch;

	bufp = (Uint16*)sandSurface->pixels + ypitch;
	redrawmenu(3);
	precalc(3);

	if (oldsandSurface) {
		int minw = oldsandSurface->w < sandSurface->w ? oldsandSurface->w - 1 : sandSurface->w;
		int minh = oldsandSurface->h < sandSurface->h ? oldsandSurface->h - 1 : sandSurface->h;
		for (int x = 0; x < minw; x++)
			for (int y = 0; y < minh; y++)
				*((Uint16*)sandSurface->pixels + ypitch * y + x) = *((Uint16*)oldsandSurface->pixels + oldypitch * y + x);
		recalccolors();
		SDL_FreeSurface(oldsandSurface);
	}
	SDL_SemPost(screensem);
}

SDL_Surface* getSandSurface() {
	return outSurface;
}

SDL_Surface* getRealSandSurface() {
	return sandSurface;
}

void sandclick(int x, int y, int b, int c) {
	b = 0;
	c = 0;
	sanddraw(element1, brush, x, y, 0, 0, 0, 0);
}

void sanddraw(Uint16 e, int b, int x, int y, int dx, int dy, int a1, int a2) {
	static Var* vpreview = (Var*)setVar("PREVIEW", 0);
	precalc(recalc);
	static Element* element;
	element = getElement(e);
	static Uint16 color;
	color = SDL_MapRGB(outSurface->format, element->r, element->g, element->b);

	int h;
	void* pixels;
	switch (b) {
	case BRUSH_FILLEDCIRCLE:
		if (!(vpreview->value))
			SDL_DrawFilledCircle16(sandSurface, x, y, dx, dx, e);
		if (!mustredraw)
			SDL_DrawFilledCircle16(outSurface, x, y - 1, dx, dx, color);
		break;
	case BRUSH_CIRCLE:
		if (!(vpreview->value))
			SDL_DrawCircle16(sandSurface, x, y, dx, dx, e);
		if (!mustredraw)
			SDL_DrawCircle16(outSurface, x, y - 1, dx, dx, color);
		break;
	case BRUSH_FILLEDRECT:
		if (!(vpreview->value))
			SDL_DrawFilledRect16(sandSurface, x, y, dx, dy, e);
		if (!mustredraw)
			SDL_DrawFilledRect16(outSurface, x, y - 1, dx, dy, color);
		break;
	case BRUSH_RECT:
		if (!(vpreview->value))
			SDL_DrawRect16(sandSurface, x, y, dx, dy, e);
		if (!mustredraw)
			SDL_DrawRect16(outSurface, x, y - 1, dx, dy, color);
		break;
	case BRUSH_LINE:
		if (!(vpreview->value))
			SDL_DrawLine16(sandSurface, x, y, dx, dy, e);
		if (!mustredraw)
			SDL_DrawLine16(outSurface, x, y - 1, dx, dy, color);
		break;
	case BRUSH_FILL:
		if (!(vpreview->value))
			SDL_Fill16(sandSurface, x, y, e);
		if (!(vpreview->value))
			recalccolors();
		break;
	case REPLACE_FILLEDCIRCLE:
		if (!(vpreview->value))
			SDL_ReplaceFilledCircle16(sandSurface, x, y, dx, dx, e, a1);
		if (!(vpreview->value))
			recalccolors();
		break;
	case REPLACE_LINE:
		if (!(vpreview->value))
			SDL_ReplaceLine16(sandSurface, x, y, dx, dy, e, a1);
		if (!(vpreview->value))
			recalccolors();
		break;
	case COPY_RECT:
		if (x + dx >= sandSurface->w)
			dx = sandSurface->w - x - 1;
		if (x < 1)
			x = 1;
		if (y + dy > sandSurface->h - 1)
			dy = sandSurface->h - y - 1;
		if (y < 1)
			y = 1;
		if (!(vpreview->value))
			SDL_CopyRect16(sandSurface, x, y, dx, dy, a1, a2);
		if (!(vpreview->value))
			recalccolors();
		h = outSurface->h;
		pixels = outSurface->pixels;
		outSurface->h = outSurfaceh;
		outSurface->pixels = outSurfacep;
		if (vpreview->value)
			SDL_CopyRect16(outSurface, x, y - 1, dx, dy, a1, a2);
		outSurface->h = h;
		outSurface->pixels = pixels;
		break;
	case ROTATE_RECT:
		if (x + dx >= sandSurface->w)
			dx = sandSurface->w - x - 1;
		if (x < 1) {
			dx += x - 1;
			x = 1;
		}
		if (y + dy >= sandSurface->h)
			dy = sandSurface->h - y - 1;
		if (y < 1) {
			dy += y - 1;
			y = 1;
		}
		if (!(vpreview->value))
			SDL_RotateRect16(sandSurface, x, y, dx, dy, a1);
		h = outSurface->h;
		pixels = outSurface->pixels;
		outSurface->h = outSurfaceh;
		outSurface->pixels = outSurfacep;
		SDL_RotateRect16(outSurface, x, y, dx, dy, a1);
		outSurface->h = h;
		outSurface->pixels = pixels;
		break;
	case BRUSH_POINT:
		if (!(vpreview->value))
			SDL_DrawSavePoint16(sandSurface, x, y, e);
		if (!mustredraw)
			SDL_DrawSavePoint16(outSurface, x, y - 1, color);
		break;
	case BRUSH_RADNOMFILLEDCIRCLE:
		if (!(vpreview->value))
			SDL_DrawRandomFilledCircle16(sandSurface, x, y, dx, dx, dy, e);
		if (!(vpreview->value))
			recalccolors();
		if (vpreview->value)
			SDL_DrawRandomFilledCircle16(outSurface, x, y, dx, dx, dy * 15, color);
		break;
	case COPY_STAMP:
		if (!(vpreview->value))
			SDL_CopyStamp16(sandSurface, x, y, dx, dy, a1);
		if (!(vpreview->value))
			recalccolors(false);
		if (!(vpreview->value))
			recalccolors(true);
		if (!(vpreview->value))
			SDL_CopyStamp16(outSurface, x, y - 1, dx, dy, a1 + MAX_STAMPS);
		break;
	case PASTE_STAMP:
		if (!(vpreview->value))
			SDL_PasteStamp16(sandSurface, x, y, dx, dy, a1, e);
		if (!(vpreview->value))
			recalccolors();
		if ((vpreview->value))
			SDL_PasteStamp16(outSurface, x, y - 1, dx, dy, a1 + MAX_STAMPS, color);
		break;
	case BRUSH_SWAPPOINTS:
		if (!(vpreview->value))
			SDL_SwapPoints16(sandSurface, x, y, dx, dy, true);
		if (!mustredraw)
			SDL_SwapPoints16(outSurface, x, y - 1, dx, dy - 1, false);
		if ((y > outSurface->h) || (y < 0) || (dy > outSurface->h) || (dy < 0))
			recalccolors();
		break;
	case BRUSH_FILLEDELLIPSE:
		if (!(vpreview->value))
			SDL_DrawFilledCircle16(sandSurface, x, y, dx, dy, e);
		if (!mustredraw)
			SDL_DrawFilledCircle16(outSurface, x, y - 1, dx, dy, color);
		break;
	case BRUSH_ELLIPSE:
		if (!(vpreview->value))
			SDL_DrawCircle16(sandSurface, x, y, dx, dy, e);
		if (!mustredraw)
			SDL_DrawCircle16(outSurface, x, y - 1, dx, dy, color);
		break;
	case REPLACE_FILLEDELLIPSE:
		if (!(vpreview->value))
			SDL_ReplaceFilledCircle16(sandSurface, x, y, dx, dy, e, a1);
		if (!(vpreview->value))
			recalccolors();
		break;
	case BRUSH_RANDOMFILLEDELLIPSE:
		if (!(vpreview->value))
			SDL_DrawRandomFilledCircle16(sandSurface, x, y, dx, dy, a1, e);
		if (!(vpreview->value))
			recalccolors();
		if (vpreview->value)
			SDL_DrawRandomFilledCircle16(outSurface, x, y, dx, dy, a1 * 15, color);
		break;
	}
	if (!(vpreview->value))
		used[e] = true;
}

void sandwrite(Uint16 element, int x, int y, int size, char* text, int align) {
#ifdef COMPILER_SDL_TTF
	static Var* ww = (Var*)setVar("WRITEWIDTH", 0);
	static Var* wh = (Var*)setVar("WRITEHEIGHT", 0);
	if (size <= 0)
		return;
	static TTF_Font* font = 0;
	if (lastfontsize != size) {
		if (font != 0)
			TTF_CloseFont(font);
		font = TTF_OpenFont(checkfilename(fontname), size);
		if (!font)
			return;
		lastfontsize = size;
	}
	SDL_DrawText16(sandSurface, font, text, x, y, element, &(ww->value), &(wh->value), align);
	used[element] = true;
	recalccolors();
#endif
}

void recalccolors(bool b) {
	if (!sandelements)
		return;
	if (b == false) {
		mustredraw = true;
		return;
	} else if (mustredraw) {
		SDL_SemWait(sandsem);
		mustredraw = false;
		Uint16* end = bufp + ypitch * (height - 1) + (width - 1);
		Uint16* t = bufp - 1;
		while (end != ++t)
			*(t + doutSurface) = *(color + *t);
		SDL_SemPost(sandsem);
	}
}

void recalcused() {
	for (int i = ((sandelements / 8) + 2) * 8; i != 0; i--)
		used[i] = false;
	used[0] = true;
	used[1] = true;
	Uint16* end = bufp + ypitch * (height - 1) + (width - 1);
	Uint16* t = bufp - 1;
	while (end != ++t)
		used[*t] = true;
}

void setprecalc(int p) {
	if (recalc < p) recalc = p;
}

void precalc(int p) {
	int t;
	std::list<Interaction*>::iterator it, end;
	std::list<Die*>::iterator it2, end2;
	Element* elements = getElement(0);
	int elementsmaxnew = getelementsmax();

	if (sandelements < elementsmaxnew + 5) {
		SDL_SemWait(sandsem);
		if (sandelements) {
			delete (color);
			delete (weight);
			delete (aweight);
			delete (weightreverse);
			delete (directionypitch);
			delete (spray);
			delete (slide);
			delete (side);
			delete (viscousity);
			delete (direction);
			delete (used);
			delete (oldused);
			delete (skip);
			delete (dierate);
			delete (nobias);
			for (int i = 0; i < sandelements; i++) {
				delete (eatspeed[i]);
				delete (eatelement[i]);
				delete (eattoself[i]);
				delete (eattrigger[i]);
				delete (eattoother[i]);
				delete (eatnot[i]);
				delete (doeseatelement[i]);
				delete (life[i]);
				delete (dieto[i]);
			}
			delete (eat);
			delete (eatspeed);
			delete (eatelement);
			delete (eattoself);
			delete (eattrigger);
			delete (eattoother);
			delete (eatnot);
			delete (doeseatelement);
			delete (life);
			delete (dieto);
		}
		sandelements = elementsmaxnew + 10;
		color = new Uint16[sandelements];
		weight = new int[sandelements];
		aweight = new int[sandelements];
		weightreverse = new int[sandelements];
		directionypitch = new int[sandelements];
		spray = new int[sandelements];
		slide = new int[sandelements];
		side = new int[sandelements];
		viscousity = new int[sandelements];
		direction = new int[sandelements];
		used = new bool[((sandelements / 8) + 20) * 8];
		oldused = new bool[((sandelements / 8) + 20) * 8];
		skip = new bool[sandelements];
		eat = new bool[sandelements];
		eatspeed = new Uint16 * [sandelements];
		eatelement = new Uint16 * [sandelements];
		eattoself = new Uint16 * [sandelements];
		eattrigger = new bool* [sandelements];
		eattoother = new uintptr_t * [sandelements];
		eatnot = new Uint16 * [sandelements];
		doeseatelement = new bool* [sandelements];
		life = new Uint16 * [sandelements];
		dieto = new Uint16 * [sandelements];
		dierate = new Uint16[sandelements];
		eatmax = new Uint16[sandelements];
		nobias = new bool[sandelements];
		for (int i = 0; i < sandelements; i++) {
			if (i < elementsmaxnew)
				eatmax[i] = elements[i].interactioncount + 10;
			else
				eatmax[i] = 10;
			eatspeed[i] = new Uint16[eatmax[i]];
			eatelement[i] = new Uint16[eatmax[i]];
			eattoself[i] = new Uint16[eatmax[i]];
			eattrigger[i] = new bool[eatmax[i]];
			eattoother[i] = new uintptr_t[eatmax[i]];
			eatnot[i] = new Uint16[eatmax[i]];
			doeseatelement[i] = new bool[DOESEATELEMENTSMAX + 16];
			life[i] = new Uint16[MAX_DIES];
			dieto[i] = new Uint16[MAX_DIES];
		}
		recalcused();
		p = 3;
		SDL_SemPost(sandsem);
	}

	int gravityfactor = 1000000;
	if (oldgravity)
		gravityfactor = 10000 / oldgravity;
	if (p >= 2) {
		for (int i = elementsmaxnew - 1; i >= 0; i--) {
			color[i] = SDL_MapRGB(outSurface->format, elements[i].r, elements[i].g, elements[i].b);
			weight[i] = ((long)elements[i].weight * 328) / gravityfactor;
			if (elements[i].weight == 0)
				weight[i] = 999999;
			if (weight[i] > 999999)
				weight[i] = 999998;
			if (!weight[i])
				if (i == 0)
					weight[i] = 1;
				else
					weight[i] = 2;
			weightreverse[i] = weight[i] * reverse;
			aweight[i] = abs(weight[i]);
			slide[i] = elements[i].slide;
			side[i] = slide[i] * 2 + 1;
			spray[i] = elements[i].spray;
			viscousity[i] = elements[i].viscousity;
			direction[i] = weightreverse[i] / abs(weightreverse[i]) * reverse;
			directionypitch[i] = ypitch * abs(weight[i]) / weight[i] * reverse;
			end2 = elements[i].dies->end();
			dierate[i] = elements[i].dietotalrate;
			nobias[i] = elements[i].nobias;
			t = 0;
			if (olddeath) {
				for (it2 = elements[i].dies->begin(); it2 != end2; it2++) {
					life[i][t] = 32768 * (*it2)->rate / dierate[i];
					dieto[i][t] = (*it2)->dieto;
					t++;
				}
				dieto[i][t] = 0;
				life[i][t] = 0;
			} else {
				dieto[i][0] = 0;
				life[i][0] = 0;
			}
		}
		weight[1] = 9999999;
	}
	if (p >= 2)
		for (int i = elementsmaxnew - 1; i >= 0; i--) {
			if (eatmax[i] < elements[i].interactioncount + 1) {
				eatmax[i] = elements[i].interactioncount + 5;
				delete (eatspeed[i]);
				delete (eatelement[i]);
				delete (eattoself[i]);
				delete (eattrigger[i]);
				delete (eattoother[i]);
				delete (eatnot[i]);
				eatspeed[i] = new Uint16[eatmax[i]];
				eatelement[i] = new Uint16[eatmax[i]];
				eattoself[i] = new Uint16[eatmax[i]];
				eattrigger[i] = new bool[eatmax[i]];
				eattoother[i] = new uintptr_t[eatmax[i]];
				eatnot[i] = new Uint16[eatmax[i]];
			}
			end = elements[i].interactions->end();
			for (int iiii = 0; iiii < DOESEATELEMENTSMAX; iiii++)
				doeseatelement[i][iiii] = false;
			t = 0;
			bool all = false;
			if (oldinteractions) {
				for (it = elements[i].interactions->begin(); it != end; it++) {
					if (used[(*it)->element]) {
						if ((eatelement[i][t] = (*it)->element) == 1)
							all = true;
						eatspeed[i][t] = (*it)->rate;
						eattoself[i][t] = (*it)->toself;
						eattoother[i][t] = (*it)->toother;
						if ((*it)->trigger == 0) {
							eattrigger[i][t] = true;
						} else {
							eattrigger[i][t] = false;
							eattoother[i][t] = (uintptr_t)(*it)->trigger;
						}
						eatnot[i][t] = (*it)->except;
						doeseatelement[i][eatelement[i][t] % DOESEATELEMENTSMAX] = true;
						t++;
					}
				}
				if (all)
					for (int iiii = 0; iiii < DOESEATELEMENTSMAX; iiii++)
						doeseatelement[i][iiii] = true;
				eatspeed[i][t] = 0;
				eat[i] = (t != 0);
			} else {
				eatspeed[i][0] = 0;
			}
			skip[i] = (weight[i] == 999999) && !t && !dierate[i];
		}
	if (p >= 1) {
		for (int y = height - 1; y >= 0; y--) {
			*(bufp + ypitch * y + width) = 1;
			*(bufp + ypitch * y) = 1;
		}
	}
	skip[0] = true;
	skip[1] = false;
	if (p >= 2) {
		recalccolors(false);
		recalccolors(true);
	}
	recalc = 0;
}

void calc() {
	SEEDRAND(seed->value++);
	Uint16* o;
	static int usedcounter = 0;
	static int oldborder;
	static int oldreverse;

	static Uint16* dont = 0;
	static int oldwidth = 0;
	int s = width * 2 + 10;
	if (oldwidth != width) {
		delete (dont);
		dont = new Uint16[s];
	}
	for (int i3 = 0; i3 < s; i3++)
		dont[i3] = 10000;
	oldwidth = width;
	int dontp;
	static Var* vnextInteraction = (Var*)setVar("NEXTINTERACTION", 0);
	vnextInteraction->value = 0;
	if (usedcounter++ > 1000) {
		recalcused();
		usedcounter = 0;
	}
	if (rev->value == 0)
		reverse = 1;
	if (rev->value == 1)
		reverse = -1;
	if (oldreverse != reverse)
		recalc = 3;
	oldreverse = reverse;

	if ((oldinteractions != varinteractions->value) || (olddeath != vardeath->value) || (oldgravity != vargravity->value)) {
		if (recalc < 3)
			recalc = 3;
		oldinteractions = varinteractions->value;
		olddeath = vardeath->value;
		oldgravity = vargravity->value;
	}

	if (recalc < 2)
		for (int i = 0; i < sandelements; i++)
			if (oldused[i] != used[i])
				recalc = 2;
	for (int i2 = 0; i2 < sandelements; i2++)
		oldused[i2] = used[i2];
	if (recalc)
		precalc(recalc);
	static int y, start;
	static int r, yr, w;
	static Uint16* end;
	static Uint16* ybuf;
	static Uint16 random, tmp = 0, tmp2;
	tmp = 0;
	int t = 0;
	static Uint16* op;
	static int i, eatcounter, diecounter;
	static bool goleft = true, goright = true;
	goleft = goright = true;

	if (rl) {
		rl = false;
		start = 1;
	} else {
		rl = true;
		start = width - 2;
	}

	static int windright, windleft;
	windright = 32768 + varwind->value * 32768 / 1000;
	windleft = 32768 - varwind->value * 32768 / 1000;
	if (windleft <= 0)
		windleft = 1;
	if (windright <= 0)
		windright = 1;

	if (reverse > 0)
		y = height - 1;
	else
		y = 0;
	while (((reverse > 0) && (y >= 0)) || ((reverse < 0) && (y < height))) {
		static bool nospray;
		if ((y == height - 1) || (y == 0))
			nospray = true;
		else
			nospray = false;
		if ((random = RANDOMNUMBER) % 2) {
			r = 1;
			yr = ypitch;
		} else {
			r = -1;
			yr = -ypitch;
		}
		ybuf = bufp + ypitch * y;

		if (rl)
			end = ybuf;
		else
			end = (ybuf + width) - 1;
		*end = 1;

		o = (Uint16*)(ybuf + start);
		dontp = dont + width + 5 - o;
		do {
			if ((*(eat + (t = *o))) && (*(*(doeseatelement + t) + (*(o - r) % DOESEATELEMENTSMAX)) || *(*(doeseatelement + t) + (*(o + r) % DOESEATELEMENTSMAX)) || *(*(doeseatelement + t) + (*(o + yr) % DOESEATELEMENTSMAX)) || *(*(doeseatelement + t) + (*(o - yr) % DOESEATELEMENTSMAX))) && (*(o + dontp) != y)) {
				eatcounter = 0;
				do {
					if (*(*(eatelement + t) + eatcounter) != 1) {
						if (tmp2 = ((tmp = *(*(eatelement + t) + eatcounter)) == *(o - 1)) + (tmp == *(o + 1)) + ((tmp == *(o + yr)) && !nospray) + (tmp == *(o - yr))) {
#define INTERACTION(x, dx, dy)                                                                                         \
	if ((tmp == *(x)) && !(tmp2--))                                                                                    \
	{                                                                                                                  \
		if (*(*(eattrigger + t) + eatcounter))                                                                         \
		{                                                                                                              \
			*(used + (*(x) = *(*(eattoother + t) + eatcounter))) = true;                                               \
			*(used + (*o = *(*(eattoself + t) + eatcounter))) = true;                                                  \
			*(o + doutSurface) = *(color + *o);                                                                        \
			*((x) + doutSurface) = *(color + *(x));                                                                    \
			t = 1;                                                                                                     \
			goto next;                                                                                                 \
		}                                                                                                              \
		else                                                                                                           \
		{                                                                                                              \
			interactionTrigger((void *)eattoother[t][eatcounter], t, tmp, o - ybuf, y + 1, o - ybuf + dx, y + dy + 1); \
			*end = 1;                                                                                                  \
			if (vnextInteraction->value)                                                                               \
				vnextInteraction->value = 0;                                                                           \
			else                                                                                                       \
			{                                                                                                          \
				if (*(nobias + t))                                                                                     \
				{                                                                                                      \
					if (rl)                                                                                            \
					{                                                                                                  \
						if (*(o - 1) != 1)                                                                             \
							o--;                                                                                       \
					}                                                                                                  \
					else                                                                                               \
					{                                                                                                  \
						if (*(o + 1) != 1)                                                                             \
							o++;                                                                                       \
					}                                                                                                  \
				}                                                                                                      \
				goto next;                                                                                             \
			}                                                                                                          \
		}                                                                                                              \
	}
							if (*(nobias + t)) {
								if ((RANDOMNUMBER < eatspeed[t][eatcounter])) {
									*(o + dontp) = y - reverse;
									tmp2 = RANDOMNUMBER % tmp2;
									INTERACTION(o + yr, 0, yr / abs(yr))
										INTERACTION(o - yr, 0, -yr / abs(yr))
										INTERACTION(o - r, -r / abs(r), 0)
										INTERACTION(o + r, r / abs(r), 0)
								}
							} else {
								tmp2 = 0;
								if (RANDOMNUMBER < eatspeed[t][eatcounter]) {
									INTERACTION(o - r, -r / abs(r), 0)
										tmp2 = 0;
								}
								if (RANDOMNUMBER < eatspeed[t][eatcounter]) {
									INTERACTION(o + r, r / abs(r), 0)
										tmp2 = 0;
								}
								if (RANDOMNUMBER < eatspeed[t][eatcounter]) {
									INTERACTION(o + yr, 0, yr / abs(yr))
										tmp2 = 0;
								}
								if (RANDOMNUMBER < eatspeed[t][eatcounter]) {
									INTERACTION(o - yr, 0, -yr / abs(yr))
								}
							}
						}
					} else {
#define INTERACTION_ALL(x, dx, dy)                                                                                      \
	if ((t != *(x)) && (eatnot[t][eatcounter] != *(x)) && (1 != *(x)) && !(tmp2--))                                     \
	{                                                                                                                   \
		if (eattrigger[t][eatcounter])                                                                                  \
		{                                                                                                               \
			if (eattoother[t][eatcounter] != 1)                                                                         \
				used[*(x) = eattoother[t][eatcounter]] = true;                                                          \
			if (eattoself[t][eatcounter] != 1)                                                                          \
				used[ *o = eattoself[t][eatcounter]] = true;                                                            \
			else                                                                                                        \
				*o = *(x);                                                                                              \
			*(o + doutSurface) = *(color + *o);                                                                         \
			*((x) + doutSurface) = *(color + *(x));                                                                     \
			t = 1;                                                                                                      \
			goto next;                                                                                                  \
		}                                                                                                               \
		else                                                                                                            \
		{                                                                                                               \
			interactionTrigger((void *)eattoother[t][eatcounter], t, *(x), o - ybuf, y + 1, o - ybuf + dx, y + dy + 1); \
			*end = 1;                                                                                                   \
			if (vnextInteraction->value)                                                                                \
				vnextInteraction->value = 0;                                                                            \
			else                                                                                                        \
			{                                                                                                           \
				if (*(nobias + t))                                                                                      \
				{                                                                                                       \
					if (rl) {																							\
						if (*(o - 1) != 1)                                                                              \
							o--;                                                                                        \
					} else {																							\
						if (*(o + 1) != 1)                                                                              \
							o++;                                                                                        \
					}                                                                                                   \
				}                                                                                                       \
				goto next;                                                                                              \
			}                                                                                                           \
		}                                                                                                               \
	}
						if (*(nobias + t)) {
							if ((RANDOMNUMBER < eatspeed[t][eatcounter])) {
								tmp2 = ((t != *(o - r)) && (eatnot[t][eatcounter] != *(o - r)) && (1 != *(o - r))) +
									((t != *(o + r)) && (eatnot[t][eatcounter] != *(o + r)) && (1 != *(o + r))) +
									((t != *(o - yr)) && (eatnot[t][eatcounter] != *(o - yr)) && (1 != *(o - yr))) +
									((t != *(o + yr)) && (eatnot[t][eatcounter] != *(o + yr)) && (1 != *(o + yr)) && !nospray);
								if (tmp2) {
									*(o + dontp) = y - reverse;
									tmp2 = RANDOMNUMBER % tmp2;
									INTERACTION_ALL(o - r, -r / abs(r), 0)
										INTERACTION_ALL(o + r, r / abs(r), 0)
										INTERACTION_ALL(o + yr, 0, yr / abs(yr))
										INTERACTION_ALL(o - yr, 0, -yr / abs(yr))
								}
							}
						} else {
							tmp2 = 0;
							if (RANDOMNUMBER < eatspeed[t][eatcounter]) {
								INTERACTION_ALL(o - r, -r / abs(r), 0)
									tmp2 = 0;
							}
							if (RANDOMNUMBER < eatspeed[t][eatcounter]) {
								INTERACTION_ALL(o + r, r / abs(r), 0)
									tmp2 = 0;
							}
							if (RANDOMNUMBER < eatspeed[t][eatcounter]) {
								INTERACTION_ALL(o + yr, 0, yr / abs(yr))
									tmp2 = 0;
							}
							if (RANDOMNUMBER < eatspeed[t][eatcounter]) {
								INTERACTION_ALL(o - yr, 0, -yr / abs(yr))
							}
						}
					}
				} while (eatspeed[t][++eatcounter]);
			}
			if (*(dierate + t) && ((random = (random * 3723 + 6) % 32768) <= *(dierate + t))) {
				diecounter = 0;
				do {
					if (RANDOMNUMBER <= *(*(life + t) + diecounter)) {
						*(o + doutSurface) = *(color + (*o = *(*(dieto + t) + diecounter)));
						goto next;
					}
				} while (*(*(life + t) + ++diecounter));
			} else if ((w = *(aweight + (t))) != 999999) {
				if ((*(op = o + *(directionypitch + t)) != t) &&
					(!(*op) &&
						(nospray || ((random % *(spray + t)) && (*(op + *(directionypitch + t)) == t)) ||
							((random = (random * 7) % 32768) < w) && (*(o + dontp) != y)) ||
						((*(weight + t) > *(weight + *op)) && (*(o + dontp) != y) && ((random = (random * 7) % 32768) < ((*(weight + t) - *(weight + *op)) / (*(viscousity + t) + *(viscousity + *op))))))) {
					if (*(nobias + t)) *(o + dontp) = y - reverse;
					*(o + doutSurface) = *(color + (*(o) = *(op)));
					*(op + doutSurface) = *(color + (*(op) = t));
				} else if (((*(o + 1) != t) || (*(o - 1) != t)) && ((random++) % *(side + t))) {
					i = 1;
					do {
						if (w <= *(aweight + *(o + i))) {
							goright = false;
						} else if (w > *(aweight + *(op + i))) {
							if (w > *(aweight + *(op - i))) {
								if (random % 2) {
									goright = false;
								} else {
									goleft = false;
									break;
								}
							} else {
								goleft = false;
								break;
							}
						}
						if (w <= *(aweight + *(o - i))) {
							goleft = false;
						} else if (w > *(aweight + *(op - i))) {
							if (w > *(aweight + *(op + i))) {
								if (random % 2) {
									goright = false;
								} else {
									goleft = false;
									break;
								}
							} else {
								goright = false;
								break;
							}
						}
					} while (goleft && goright && i++);
					if (goleft || goright) {
						if (!goright && (random < windright)) {
							if (i > 2)
								i = 2;
							*(o + doutSurface) = *(color + (*(o) = *(o - i)));
							*(o + doutSurface - i) = *(color + (*(o - i) = t));
						} else if (!goleft && (random < windleft)) {
							if (i > 2)
								i = 2;
							*(o + doutSurface) = *(color + (*(o) = *(o + i)));
							*(o + doutSurface + i) = *(color + (*(o + i) = t));
						}
					} else {
						goleft = goright = true;
					}
				}
			}
		next:
			if (rl) {
#ifdef COMPILER_GCC
				do
					while (!(*(long*)(o - 2)))
						o -= 2;
				while (*(skip + *(--o)) && *(skip + *(--o)) && *(skip + *(--o)) && *(skip + *(--o)));
#else
				while (!--o);
				while (*(skip + *(o))) o--;
#endif
			} else {
#ifdef COMPILER_GCC
				do
					while (!(*(long*)(o + 1)))
						o += 2;
				while (*(skip + *(++o)) && *(skip + *(++o)) && *(skip + *(++o)) && *(skip + *(++o)));
#else
				while (!++o);
				while (*(skip + *(o))) o++;
#endif
			}
		} while (o != end);

		if (reverse > 0) --y;
		else ++y;
	}

	o = (Uint16*)(bufp - width);
	op = (Uint16*)(bufp);
	int delta = (height * ypitch + ypitch);

	t = varborder->value;
	if (t == 1) {
		while (o < op) {
			*(o) = 1;
			*(o++ + delta) = 1;
		}
	} else if (t == 0) {
		while (o < op) {
			*(o + doutSurface) = *(o) = 0;
			*(o + delta + doutSurface) = *(o + delta) = 0;
			o++;
		}
	} else if (t == 2) {
		if (oldborder != t)
			while (o < op) {
				*(o) = 0;
				*(o++ + delta) = 0;
			}
		while (o < op) {
			if (!*(o + ypitch)) {
				*(o + ypitch + doutSurface) = *(color + (*(o + ypitch) = *(o + delta)));
				*(o + delta + doutSurface) = *(o + delta) = 0;
			}
			if (!*(o + delta - ypitch)) {
				*(o + delta - ypitch + doutSurface) = *(color + (*(o + delta - ypitch) = *(o)));
				*(o + doutSurface) = *(o) = 0;
			}
			o++;
		}
		o = (Uint16*)(bufp - width);
	}
	oldborder = t;
}

Uint16 getPixel(int x, int y) {
	if ((x < 1) || (x > width) || (y <= 0) || (y > height)) return 1;
	return *(bufp + ypitch * (y - 1) + (x));
}