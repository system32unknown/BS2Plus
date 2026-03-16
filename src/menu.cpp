#include "menu.h"
#include "cursors.h"
#include "console.h"
#include "win.h"

char status_text[4097];
int MENU_TOP = 32;
int MENU_LEFT = 32;
int MENU_RIGHT = 32;
int MENU_BOTTOM = 52;
#define MENU_WIDTH 32
#define MENU_HEIGHT 32
Var* vmenutop;
Var* vmenubottom;
Var* vmenuright;
Var* vmenuleft;
Var* vstatus;
Var* vconsole;
Var* vview;
Var* vviewmaxpos;
Var* vviewmaxneg;
Var* vviewnegr;
Var* vviewnegg;
Var* vviewnegb;
Var* vviewposr;
Var* vviewposg;
Var* vviewposb;
Var* vviewr;
Var* vviewg;
Var* vviewb;
Var* vshowpreview;
Var* valpha1;
Var* valpha2;
Var* vcursor;
Var* vx;
Var* vy;
Var* borderr;
Var* borderg;
Var* borderb;
Var* backgroundr;
Var* backgroundg;
Var* backgroundb;
Var* menufontcolorr;
Var* menufontcolorg;
Var* menufontcolorb;
Var* fgtransparentcolorr;
Var* fgtransparentcolorg;
Var* fgtransparentcolorb;
Var* fgtransparentcolor;
Var* fglayeralpha;
Var* bgtransparentcolorr;
Var* bgtransparentcolorg;
Var* bgtransparentcolorb;
Var* bglayeralpha;
Var* zoomvar;

int lastx = -1000, lasty = -1000;
int scroll_left;
int scroll_top;
int redraw;
bool redrawmenus;
int subMenu;
int subMenuStay;
int subMenuAlign;

SDL_Surface* lastscreen = 0;
SDL_Surface* fglayer = 0;
SDL_Surface* bglayer = 0;
static Var* solid;
bool oversand;

#ifdef COMPILER_SDL_TTF
TTF_Font* status_font;
#endif
Uint16 border;
Uint16 background;
char* mouseover;

int console_visible;
std::list<Button*> menubuttons[5];
SDL_Surface* menusurfaces[5];
SDL_Rect menurects[5];
SDL_Rect menupos[5];
SDL_Cursor* menucursor, * sandcursor[MAX_CURSORS];
int scroll[5];
int realscrollx[5];
int realscrolly[5];

SDL_Color menufontfgcolor;
SDL_Color menufontbgcolor;

static SDL_Cursor* init_system_cursor(const char* image[]) {
	int i, row, col;
	Uint8 data[4 * 16];
	Uint8 mask[4 * 16];
	int hot_x, hot_y;

	i = -1;
	for (row = 0; row < 16; ++row) {
		for (col = 0; col < 16; ++col) {
			if (col % 8) {
				data[i] <<= 1;
				mask[i] <<= 1;
			} else {
				++i;
				data[i] = mask[i] = 0;
			}
			switch (image[4 + row][col]) {
			case 'X':
				data[i] |= 0x01;
				mask[i] |= 0x01;
				break;
			case '.':
				mask[i] |= 0x01;
				break;
			case ' ':
				break;
			}
		}
	}
	sscanf(image[4 + row], "%d,%d", &hot_x, &hot_y);
	return SDL_CreateCursor(data, mask, 16, 16, hot_x, hot_y);
}

void initmenu(SDL_Surface* screen) {
	setVar("SPEED", 60);
	zoomvar = (Var*)setVar("ZOOM", 1);
	vmenutop = (Var*)setVar("MENUTOP", 33);
	vmenubottom = (Var*)setVar("MENUBOTTOM", 33);
	vmenuright = (Var*)setVar("MENURIGHT", 32);
	vmenuleft = (Var*)setVar("MENULEFT", 33);
	vstatus = (Var*)setVar("STATUS", 1);
	vconsole = (Var*)setVar("CONSOLE", 0);
	vview = (Var*)setVar("VIEW", 0);
	vviewmaxpos = (Var*)setVar("VIEWMAXPOS", 1200);
	vviewmaxneg = (Var*)setVar("VIEWMAXNEG", 350);
	vviewnegr = (Var*)setVar("VIEWNEGR", 0);
	vviewnegg = (Var*)setVar("VIEWNEGG", 255);
	vviewnegb = (Var*)setVar("VIEWNEGB", 0);
	vviewposr = (Var*)setVar("VIEWPOSR", 0);
	vviewposg = (Var*)setVar("VIEWPOSG", 0);
	vviewposb = (Var*)setVar("VIEWPOSB", 255);
	vviewr = (Var*)setVar("VIEWR", 255);
	vviewg = (Var*)setVar("VIEWG", 0);
	vviewb = (Var*)setVar("VIEWB", 0);
	vshowpreview = (Var*)setVar("SHOWPREVIEW", 0);
	valpha1 = (Var*)setVar("ALPHA1", 255);
	valpha2 = (Var*)setVar("ALPHA2", 255);
	vcursor = (Var*)setVar("CURSOR", 1);
	vx = (Var*)setVar("X", 0);
	vy = (Var*)setVar("Y", 0);
	borderr = (Var*)setVar("BORDERR", 255);
	borderg = (Var*)setVar("BORDERG", 255);
	borderb = (Var*)setVar("BORDERB", 255);
	backgroundr = (Var*)setVar("BACKGROUNDR", 0);
	backgroundg = (Var*)setVar("BACKGROUNDG", 0);
	backgroundb = (Var*)setVar("BACKGROUNDB", 0);
	menufontcolorr = (Var*)setVar("MENUFONTCOLORR", 255);
	menufontcolorg = (Var*)setVar("MENUFONTCOLORG", 255);
	menufontcolorb = (Var*)setVar("MENUFONTCOLORB", 255);
	fgtransparentcolorr = (Var*)setVar("FGLAYERTRANSPARENTR", 0);
	fgtransparentcolorg = (Var*)setVar("FGLAYERTRANSPARENTG", 0);
	fgtransparentcolorb = (Var*)setVar("FGLAYERTRANSPARENTB", 0);
	fgtransparentcolor = (Var*)setVar("FGLAYERTRANSPARENT", 0);
	fglayeralpha = (Var*)setVar("FGLAYERALPHA", 255);

	menucursor = SDL_GetCursor();
	lastscreen = screen;
#ifdef COMPILER_SDL_TTF
	status_font = TTF_OpenFont(checkfilename("font.ttf"), 12);
#endif
	redraw = 3;
	subMenu = -1;
	subMenuStay = 0;

	sandcursor[0] = SDL_GetCursor();
	for (int i = 1; i < MAX_CURSORS; i++)
		sandcursor[i] = init_system_cursor(&cursors[21 * i]);

	solid = (Var*)setVar("SOLIDDRAW", 1);
	status_text[0] = 0;
};

void drawmenu(SDL_Surface* screen) {
	SDL_SemWait(screensem);
	SDL_Surface* sandsurface = getSandSurface();
	static SDL_Rect clearrects[4];
	static int lastconsole = 0;
	if (vconsole->value != lastconsole)
		redraw = 3;
	lastconsole = vconsole->value;
	static int vmenutopold, vmenubottomold, vmenuleftold, vmenurightold, vstatusols;
	border = SDL_MapRGB(screen->format, borderr->value, borderg->value, borderb->value);
	background = SDL_MapRGB(screen->format, backgroundr->value, backgroundg->value, backgroundb->value);
	menufontfgcolor.r = menufontcolorr->value;
	menufontfgcolor.g = menufontcolorg->value;
	menufontfgcolor.b = menufontcolorb->value;
	menufontbgcolor.r = backgroundr->value;
	menufontbgcolor.g = backgroundg->value;
	menufontbgcolor.b = backgroundb->value;
	if ((vstatusols != vstatus->value) || (vmenutopold != vmenutop->value) || (vmenuleftold != vmenuleft->value) || (vmenurightold != vmenuright->value) || (vmenubottomold != vmenubottom->value)) {
		vmenutopold = vmenutop->value;
		vmenuleftold = vmenuleft->value;
		vmenurightold = vmenuright->value;
		vmenubottomold = vmenubottom->value;
		vstatusols = vstatus->value;
		MENU_TOP = vmenutop->value - 1;
		MENU_BOTTOM = vmenubottom->value - 1;
		MENU_LEFT = vmenuleft->value - 1;
		MENU_RIGHT = vmenuright->value;
		if (vstatus->value)
			MENU_BOTTOM += 19;
		else
			MENU_BOTTOM += 1;
		SDL_FillRect(screen, 0, background);
		redraw = 3;
		SDL_SemPost(screensem);
		autoresize(screen);
		return;
	}

	static int zoom;
	if (zoom != zoomvar->value)
		if (redraw < 2)
			redraw = 2;
	zoom = zoomvar->value;

	int maxwidth = screen->w - MENU_RIGHT - MENU_LEFT;
	int maxheight = screen->h - MENU_TOP - MENU_BOTTOM;

	if (redraw >= 3) {
		menurects[0].x = MENU_LEFT;
		menurects[0].y = 0;
		menurects[0].w = maxwidth;
		menurects[0].h = MENU_TOP;
		menurects[1].x = 0;
		menurects[1].y = MENU_TOP;
		menurects[0].w = MENU_LEFT;
		menurects[0].h = maxheight;
		menurects[2].x = screen->w - MENU_RIGHT + 1;
		menurects[2].y = MENU_TOP;
		menurects[0].w = MENU_RIGHT;
		menurects[0].h = maxheight;
		menurects[3].x = MENU_LEFT;
		menurects[3].y = screen->h - MENU_BOTTOM + 1;
		menurects[0].w = maxwidth;
		menurects[0].h = MENU_BOTTOM - vstatus->value * 18;
		if (menusurfaces[0] != 0) {
			SDL_FreeSurface(menusurfaces[0]);
			menusurfaces[0] = 0;
		}
		if (MENU_TOP != -1)
			menusurfaces[0] = createmenu(&(menubuttons[0]), MENU_ALIGN_H, maxwidth, MENU_TOP);
		if (menusurfaces[1] != 0) {
			SDL_FreeSurface(menusurfaces[1]);
			menusurfaces[1] = 0;
		}
		if (MENU_LEFT != -1)
			menusurfaces[1] = createmenu(&(menubuttons[1]), MENU_ALIGN_V, MENU_LEFT, maxheight);
		if (menusurfaces[2] != 0) {
			SDL_FreeSurface(menusurfaces[2]);
			menusurfaces[2] = 0;
		}
		if (MENU_RIGHT != -1)
			menusurfaces[2] = createmenu(&(menubuttons[2]), MENU_ALIGN_V, MENU_RIGHT, maxheight);
		if (menusurfaces[3] != 0) {
			SDL_FreeSurface(menusurfaces[3]);
			menusurfaces[3] = 0;
		}
		if (MENU_BOTTOM != -1)
			menusurfaces[3] = createmenu(&(menubuttons[3]), MENU_ALIGN_H, maxwidth, MENU_BOTTOM - vstatus->value * 18);
	}

	checkscroll();

	SDL_Rect dstrect;
	dstrect.x = MENU_LEFT + 1;
	dstrect.y = MENU_TOP + 1;

	SDL_Rect srcrect;
	srcrect.x = 1 + scroll_left;
	srcrect.y = 0 + scroll_top;
	srcrect.w = screen->w - MENU_LEFT - MENU_RIGHT - 1;
	srcrect.h = screen->h - MENU_TOP - MENU_BOTTOM - 1;
	if (srcrect.w + zoom * 2 > sandsurface->w * zoom) {
		dstrect.x = MENU_LEFT + srcrect.w / 2 - sandsurface->w / 2 * zoom + zoom + 1;
		scroll_left = -srcrect.w / 2 + sandsurface->w / 2 * zoom + 1 - zoom + 1;
		srcrect.w = (sandsurface->w - 2) * zoom;
		srcrect.x = 1;
	}
	if (srcrect.h + zoom * 2 - 1 > sandsurface->h * zoom) {
		dstrect.y += srcrect.h / 2 - sandsurface->h / 2 * zoom + zoom - 1;
		scroll_top = -srcrect.h / 2 + sandsurface->h / 2 * zoom - zoom - 1;
		srcrect.h = sandsurface->h * zoom - zoom * 2 + 2;
		srcrect.y = 0;
	}

	if (redraw >= 2) {
		if ((vview->value == 10) || ((valpha1->value < 256) && (vview->value == 0))) {
			clearrects[0].x = 0;
			clearrects[0].y = 0;
			clearrects[0].w = dstrect.x;
			clearrects[0].h = screen->h;

			clearrects[1].x = dstrect.x;
			clearrects[1].h = 0;
			clearrects[1].w = 0;
			clearrects[1].h = 0;

			clearrects[2].x = dstrect.x;
			clearrects[2].y = dstrect.y + srcrect.h;
			clearrects[2].w = 0;
			clearrects[2].h = screen->h - clearrects[2].y;

			clearrects[3].x = dstrect.x + srcrect.w;
			clearrects[3].y = 0;
			clearrects[3].w = screen->w - clearrects[3].x;
			clearrects[3].h = screen->h;

			SDL_FillRect(screen, &(clearrects[0]), background);
			SDL_FillRect(screen, &(clearrects[1]), background);
			SDL_FillRect(screen, &(clearrects[2]), background);
			SDL_FillRect(screen, &(clearrects[3]), background);
		} else
			SDL_FillRect(screen, 0, background);
		SDL_DrawRect16(screen, MENU_LEFT, MENU_TOP, screen->w - MENU_RIGHT - MENU_LEFT, screen->h - MENU_BOTTOM - MENU_TOP, border);
	}

	if (redraw >= 0) {
		SDL_DrawLine16(screen, dstrect.x - 1, MENU_TOP, 0, dstrect.y + srcrect.h - MENU_TOP, border);
		SDL_DrawLine16(screen, dstrect.x + srcrect.w, dstrect.y, 0, screen->h - MENU_BOTTOM - dstrect.y, border);
		SDL_DrawLine16(screen, dstrect.x, dstrect.y - 1, screen->w - dstrect.x - MENU_RIGHT, 0, border);
		SDL_DrawLine16(screen, MENU_LEFT, dstrect.y + srcrect.h, dstrect.x + srcrect.w - MENU_LEFT, 0, border);
	}

	if ((redraw >= 0) || redrawmenus) {
		SDL_Rect r;
		if ((vmenutop->value != 0) && menusurfaces[0]) {
			r.x = (scroll[0] + 1) * 1000 * (menusurfaces[0]->w + 1 - maxwidth) / maxwidth / 1000;
			r.w = maxwidth;
			r.y = 0;
			r.h = MENU_TOP;
			realscrollx[0] = r.x;
			realscrolly[0] = r.y;
			if (r.x > menusurfaces[0]->w - maxwidth)
				r.x = menusurfaces[0]->w - maxwidth;
			SDL_BlitSurface(menusurfaces[0], &r, screen, &menurects[0]);
		}
		if ((vmenubottom->value != 0) && menusurfaces[3]) {
			r.x = (scroll[3] + 1) * 1000 * (menusurfaces[3]->w + 1 - maxwidth) / maxwidth / 1000;
			r.w = maxwidth;
			r.y = 0;
			r.h = MENU_BOTTOM - vstatus->value * 19;
			realscrollx[3] = r.x;
			realscrolly[3] = r.y;
			if (r.x > menusurfaces[3]->w - maxwidth)
				r.x = menusurfaces[3]->w - maxwidth;
			SDL_BlitSurface(menusurfaces[3], &r, screen, &menurects[3]);
		}
		if ((vmenuleft->value != 0) && menusurfaces[1]) {
			r.y = (scroll[1] + 1) * 1000 * (menusurfaces[1]->h + 1 - maxheight) / maxheight / 1000;
			r.h = maxheight;
			r.x = 0;
			r.w = MENU_LEFT;
			realscrollx[1] = r.x;
			realscrolly[1] = r.y;
			if (r.y > menusurfaces[1]->h - maxheight)
				r.y = menusurfaces[1]->h - maxheight;
			SDL_BlitSurface(menusurfaces[1], &r, screen, &menurects[1]);
		}
		if ((vmenuright->value != 0) && menusurfaces[2]) {
			r.y = (scroll[2] + 1) * 1000 * (menusurfaces[2]->h + 1 - maxheight) / maxheight / 1000;
			r.h = maxheight;
			r.x = 0;
			r.w = MENU_RIGHT;
			realscrollx[2] = r.x;
			realscrolly[2] = r.y;
			if (r.y > menusurfaces[2]->h - maxheight)
				r.y = menusurfaces[2]->h - maxheight;
			SDL_BlitSurface(menusurfaces[2], &r, screen, &menurects[2]);
		}
	}

	static Var* vpreview = (Var*)setVar("PREVIEW", 0);
	static bool lastpreview = false;
	if (oversand && (vshowpreview->value)) {
		vpreview->value = 1;
		recalccolors(false);
		recalccolors(true);
		findTrigger("DRAW", 0)->exec(lastx, lasty, 1, true);
		vpreview->value = 0;
		lastpreview = true;
	} else if (lastpreview) {
		recalccolors(false);
		recalccolors(true);
		lastpreview = false;
	}

	if (!(vview->value)) {
		if (zoom == 1) {
			if (bglayer) {
				SDL_Rect r;
				r.x = srcrect.x;
				r.y = 1 + srcrect.y;
				r.w = srcrect.w;
				r.h = srcrect.h;
				SDL_BlitSurface(bglayer, &r, screen, &dstrect);
			}
			if (valpha1->value < 255) {
				SDL_SetColorKey(sandsurface, 0, 0);
				SDL_SetAlpha(sandsurface, SDL_SRCALPHA | SDL_RLEACCEL, valpha1->value);
				SDL_BlitSurface(sandsurface, &srcrect, screen, &dstrect);
				SDL_SetColorKey(sandsurface, SDL_SRCCOLORKEY, SDL_MapRGB(sandsurface->format, getElement(0)->r, getElement(0)->g, getElement(0)->b));
				SDL_SetAlpha(sandsurface, SDL_RLEACCEL, 255);
				SDL_BlitSurface(sandsurface, &srcrect, screen, &dstrect);
			} else {
				if (bglayer)
					SDL_SetColorKey(sandsurface, SDL_SRCCOLORKEY, SDL_MapRGB(sandsurface->format, getElement(0)->r, getElement(0)->g, getElement(0)->b));
				else
					SDL_SetColorKey(sandsurface, 0, 0);
				SDL_SetAlpha(sandsurface, SDL_RLEACCEL, 255);
				SDL_BlitSurface(sandsurface, &srcrect, screen, &dstrect);
			}
			if (fglayer) {
				SDL_Rect r;
				r.x = srcrect.x;
				r.y = 1 + srcrect.y;
				r.w = srcrect.w;
				r.h = srcrect.h;
				if (fgtransparentcolor->value)
					SDL_SetColorKey(fglayer, SDL_SRCCOLORKEY, SDL_MapRGB(fglayer->format, fgtransparentcolorr->value, fgtransparentcolorg->value, fgtransparentcolorb->value));
				else
					SDL_SetColorKey(fglayer, 0, 0);
				SDL_SetAlpha(fglayer, SDL_SRCALPHA | SDL_RLEACCEL, fglayeralpha->value);
				SDL_BlitSurface(fglayer, &r, screen, &dstrect);
			}
		} else {
			Uint16* p = (Uint16*)screen->pixels, * yp, * ypz, * pz;
			pz = (Uint16*)(getSandSurface()->pixels);
			int ypitch = screen->pitch / 2;
			int ypitchz = getSandSurface()->pitch / 2;
			int to;
			int dy = scroll_top - MENU_TOP - 2 + zoom;
			int dx = scroll_left - MENU_LEFT + zoom - 1;
			for (int y = dstrect.y; y < dstrect.y + srcrect.h; y++) {
				yp = p + y * ypitch;
				ypz = pz + ((y + dy) / zoom) * ypitchz;
				to = dstrect.x + srcrect.w;
				for (int x = dstrect.x; x < to; x++)
					*(yp + x) = *(ypz + (x + dx) / zoom);
			}
		}
	} else {
		int view = vview->value;
		int maxpos = vviewmaxpos->value;
		int maxneg = -vviewmaxneg->value;
		Uint16* colormappositiv = new Uint16[maxpos];
		Uint16* colormapnegativ = new Uint16[-maxneg];
		Uint16 colormapneutral = 0;
		int maxelements = getelementsmax();
		Uint16* colormap = new Uint16[maxelements + 1];
		Element* e = getElement(0);
		if (vview->value > 0) {
			colormapneutral = SDL_MapRGB(screen->format, vviewr->value, vviewg->value, vviewb->value);
			for (int i = 0; i < maxpos; i++)
				colormappositiv[i] = SDL_MapRGB(screen->format, (i * vviewposr->value / maxpos), (i * vviewposg->value / maxpos), (i * vviewposb->value / maxpos));
			for (int i2 = 0; i2 < -maxneg; i2++)
				colormapnegativ[i2] = SDL_MapRGB(screen->format, -(i2 * vviewnegr->value / maxneg), -(i2 * vviewnegg->value / maxneg), -(i2 * vviewnegb->value / maxneg));
		}
		if (vview->value == -1)
			for (int i = 0; i < maxelements; i++)
				colormap[i] = SDL_MapRGB(screen->format, e[i].cr1, e[i].cg1, e[i].cb1);
		if (vview->value == -2)
			for (int i = 0; i < maxelements; i++)
				colormap[i] = SDL_MapRGB(screen->format, e[i].cr2, e[i].cg2, e[i].cb2);
		if (vview->value == -3)
			for (int i = 0; i < maxelements; i++)
				colormap[i] = SDL_MapRGB(screen->format, e[i].cr3, e[i].cg3, e[i].cb3);
		Uint16* p = (Uint16*)screen->pixels, * yp, * ypz, * pz;
		int ypitchz;
		if (vview->value < 10) {
			pz = (Uint16*)(getRealSandSurface()->pixels);
			ypitchz = getRealSandSurface()->pitch / 2;
		} else {
			pz = (Uint16*)(getSandSurface()->pixels);
			ypitchz = getSandSurface()->pitch / 2;
		}
		int ypitch = screen->pitch / 2;
		int to;
		int dy = scroll_top - MENU_TOP - 2 + zoom;
		int dx = scroll_left - MENU_LEFT + zoom - 1;
		int blurfactor1 = 1;
		int blurfactor2 = 1;
		int blurfactor = 2;
		int alpha1 = valpha1->value;
		int alpha2 = valpha2->value;
		if (vview->value == 12) {
			blurfactor1 = 3;
			blurfactor2 = 1;
			blurfactor = 2;
		}
		for (int y = dstrect.y; y < dstrect.y + srcrect.h; y++) {
			yp = p + y * ypitch;
			if (zoom == 1)
				if (vview->value > 9)
					ypz = pz + (((y + dy) / zoom) + 2) * ypitchz;
				else
					ypz = pz + (((y + dy) / zoom) + 3) * ypitchz;
			else if (vview->value > 9)
				ypz = pz + (((y + dy) / zoom)) * ypitchz - 1;
			else
				ypz = pz + (((y + dy) / zoom) + 1) * ypitchz - 1;
			to = dstrect.x + srcrect.w;
			int t = 0;
			if ((vview->value > 0) && (vview->value < 9))
				for (int x = dstrect.x; x < to; x++) {
					if (view == 1)
						t = e[*(ypz + (x + dx) / zoom)].weight;
					else if (view == 2)
						t = e[*(ypz + (x + dx) / zoom)].spray;
					else if (view == 3)
						t = e[*(ypz + (x + dx) / zoom)].slide;
					else if (view == 4)
						t = e[*(ypz + (x + dx) / zoom)].viscousity;
					else if (view == 5)
						t = e[*(ypz + (x + dx) / zoom)].dietotalrate;
					else if (view == 6)
						t = e[*(ypz + (x + dx) / zoom)].r;
					else if (view == 7)
						t = e[*(ypz + (x + dx) / zoom)].g;
					else if (view == 8)
						t = e[*(ypz + (x + dx) / zoom)].b;
					if (!t)
						*(yp + x) = colormapneutral;
					else if ((t > 0) && (t < maxpos))
						*(yp + x) = colormappositiv[t];
					else if ((t < 0) && (t > maxneg))
						*(yp + x) = colormapnegativ[-t];
					else if (t > 0)
						*(yp + x) = colormappositiv[maxpos - 1];
					else if (t < 0)
						*(yp + x) = colormapnegativ[-(maxneg + 1)];
				} else if (vview->value == 10) {
					int r, g, b, b1, a1;
					for (int x = dstrect.x; x < to; x++) {
						if (*(yp + x) != *(ypz + (x + dx) / zoom)) {
							b1 = *(yp + x);
							a1 = *(ypz + (x + dx) / zoom);
							if (r = (a1 & 63488) - (b1 & 63488)) {
								if (r > 2048 * alpha1)
									r = 2048 * alpha1;
								else if (r < -2048 * alpha2)
									r = -2048 * alpha2;
							}
							if (g = (a1 & 1984) - (b1 & 1984)) {
								if (g > 64 * alpha1)
									g = 64 * alpha1;
								else if (g < -64 * alpha2)
									g = -64 * alpha2;
							}
							if (b = (a1 & 31) - (b1 & 31)) {
								if (b > alpha1)
									b = alpha1;
								else if (b < -alpha2)
									b = -alpha2;
							}
							*(yp + x) = b1 + r + g + b;
						}
					}
				} else if (vview->value == 11)
					for (int x = dstrect.x; x < to; x++)
						*(yp + x) = ((*(yp + x) + *(ypz + (x + dx) / zoom)) / 2);
				else if (vview->value == 12)
				for (int x = dstrect.x; x < to; x++) {
					int a = *(yp + x);
					int b = *(ypz + (x + dx) / zoom);
					*(yp + x) = b + ((((a & 63488) * blurfactor1 - (b & 63488) * blurfactor2) / blurfactor) & 63488) + ((((a & 2016) * blurfactor1 - (b & 2016) * blurfactor2) / blurfactor) & 2016) + ((((a & 31) * blurfactor1 - (b & 31) * blurfactor2) / blurfactor) & 31);
				} else
					for (int x = dstrect.x; x < to; x++)
						*(yp + x) = colormap[*(ypz + (x + dx) / zoom)];
		}
		delete (colormappositiv);
		delete (colormapnegativ);
		delete (colormap);
	}

	if ((subMenu != -1) && (subMenu != -2) && (menusurfaces[4] || redrawmenus)) {
		if (menurects[4].x + menusurfaces[4]->w > screen->w)
			menurects[4].x = screen->w - menusurfaces[4]->w;
		if (menurects[4].y + menusurfaces[4]->h + vstatus->value * 18 > screen->h)
			menurects[4].y = screen->h - menusurfaces[4]->h - vstatus->value * 18;
		if (menurects[4].x < 0)
			menurects[4].x = 0;
		if (menurects[4].y < 0)
			menurects[4].y = 0;
		SDL_Rect r;
		if ((subMenu == MENU_BAR_TOP) || (subMenu == MENU_BAR_BOTTOM)) {
			r.y = (scroll[4] + 1) * 1000 * (menusurfaces[4]->h + 1 - maxheight) / maxheight / 1000;
			if (r.y < 0)
				r.y = 0;
			r.h = maxheight;
			r.x = 0;
			r.w = 32000;
			realscrollx[4] = r.x;
			realscrolly[4] = r.y;
		} else {
			r.x = (scroll[4] + 1) * 1000 * (menusurfaces[4]->w + 1 - maxwidth) / maxwidth / 1000;
			if (r.x < 0)
				r.x = 0;
			r.w = maxwidth;
			r.y = 0;
			r.h = 32000;
			realscrollx[4] = r.x;
			realscrolly[4] = r.y;
		}
		SDL_BlitSurface(menusurfaces[4], &r, screen, &menurects[4]);
		int h = menusurfaces[4]->h;
		if (h > maxheight)
			h = maxheight;
		int w = menusurfaces[4]->w;
		if (w > maxwidth)
			w = maxwidth;
		if (subMenu != MENU_BAR_TOP)
			SDL_DrawLine16(screen, menurects[4].x, menurects[4].y, w - 1, 0, border);
		if (subMenu != MENU_BAR_LEFT)
			SDL_DrawLine16(screen, menurects[4].x - 1, menurects[4].y, 0, h - 1, border);
		if (subMenu != MENU_BAR_RIGHT)
			SDL_DrawLine16(screen, menurects[4].x + menusurfaces[4]->w, menurects[4].y, 0, h - 1, border);
		if (subMenu != MENU_BAR_BOTTOM)
			SDL_DrawLine16(screen, menurects[4].x, menurects[4].y + h, w - 1, 0, border);
	}

#ifdef COMPILER_SDL_TTF
	if (vconsole->value) {
		consolenews = false;
		dstrect.x = 0;
		dstrect.y = 0;
		SDL_Surface* console = getConsoleSurface();
		SDL_BlitSurface(console, 0, screen, &dstrect);
		SDL_DrawLine16(screen, screen->w - 15, console->h, 7, -7, border);
		SDL_DrawLine16(screen, screen->w - 1, console->h, -7, -7, border);
		SDL_DrawLine16(screen, 0, console->h, screen->w, 0, border);
	} else if (((redraw >= 2) || consolenews) && (MENU_TOP && MENU_RIGHT)) {
		if (consolenews)
			for (int s = 1; s < 7; s++) {
				SDL_DrawLine16(screen, screen->w - 1 - s * 2, 0, s, s, border);
				SDL_DrawLine16(screen, screen->w - 1, 0, -s, s, border);
			} else {
				int s = 5;
				SDL_DrawLine16(screen, screen->w - 1 - s * 2, 0, s, s, border);
				SDL_DrawLine16(screen, screen->w - 1, 0, -s, s, border);
			}
	}

	static char status_textold[4097];
	if (vstatusols && (strcmp(status_text, status_textold) || (redraw >= 2))) {
		SDL_DrawLine16(screen, 0, screen->h - 18, screen->w, 0, border);
		dstrect.x = 0;
		dstrect.w = screen->w;
		dstrect.y = screen->h - 17;
		dstrect.h = 17;
		SDL_FillRect(screen, &dstrect, background);
		strcpy(status_textold, status_text);
#ifdef COMPILER_SDL_TTF
		SDL_DrawText16(screen, status_font, status_text, 2, screen->h - 2, border);
#endif
	}
#endif

	redrawmenus = false;
	redraw = 0;
	SDL_SemPost(screensem);
}

void showconsole(int visible) {
	if (visible == -1)
		if (console_visible != false)
			console_visible = false;
		else console_visible = true;
	else console_visible = visible;
	redrawmenu(2);
}

void checkscroll() {
	int zoom = zoomvar->value;
	static Var* sx = (Var*)setVar("SCROLLX", 0);
	static Var* sy = (Var*)setVar("SCROLLY", 0);
	SDL_Surface* sandSurface = getSandSurface();
	if (scroll_top > sandSurface->h * zoom - lastscreen->h + MENU_TOP + MENU_BOTTOM - zoom + 3)
		scroll_top = (sandSurface->h * zoom - lastscreen->h + MENU_TOP + MENU_BOTTOM - zoom + 3);
	if (scroll_left > sandSurface->w * zoom - lastscreen->w + MENU_LEFT + MENU_RIGHT - zoom + 1 - zoom)
		scroll_left = sandSurface->w * zoom - lastscreen->w + MENU_LEFT + MENU_RIGHT - zoom + 1 - zoom;
	if (scroll_left < 0)
		scroll_left = 0;
	if (scroll_top < 1 - zoom)
		scroll_top = 1 - zoom;
	sx->value = scroll_left;
	sy->value = scroll_top;
}

void clickmenu(SDL_Surface* screen, int x, int y, int b, int click) {
	static int lx, ly;
	static bool nomousedown;
	static int nomousedownbutton = 0;

	int drawx, drawy;
	drawx = (x - MENU_LEFT + scroll_left - 1) / zoomvar->value + 1;
	if (zoomvar->value == 1)
		drawy = (y - MENU_TOP + scroll_top - 1) / zoomvar->value + 2;
	else
		drawy = (y - MENU_TOP + scroll_top - 2) / zoomvar->value + 2;
	if (nomousedown == true) {
		if (b != 0) return;
		nomousedown = false;
	}
	if (nomousedownbutton && !b) {
		nomousedownbutton = 0;
		findTrigger("MOUSEUP", 0)->exec(drawx, drawy, b, click);
	}
	if ((lx == x) && (ly == y) && (!b) && (!click))
		return;
	lx = x;
	ly = y;
	std::list<Button*>::iterator it;
	int cursor = 0;
	if (x <= MENU_LEFT)
		cursor = 1;
	else if (x >= lastscreen->w - MENU_RIGHT)
		cursor = 1;
	else if (y <= MENU_TOP)
		cursor = 1;
	else if (y >= lastscreen->h - MENU_BOTTOM)
		cursor = 1;
	else if ((subMenu != -1) && (subMenu != -2))
		if ((x > menurects[4].x) && (x < menurects[4].x + menusurfaces[4]->w) &&
			(y - 1 > menurects[4].y) && (y - 1 < menurects[4].y + menusurfaces[4]->h))
			cursor = 1;
	if (cursor) {
		oversand = false;
		SDL_SetCursor(menucursor);
		if (!nomousedownbutton)
			mouseover = "Nothing";
		vx->value = 0;
		vy->value = 0;
	} else {
		oversand = true;
		SDL_SetCursor(sandcursor[vcursor->value % MAX_CURSORS]);
		static Var* zoomvar = (Var*)setVar("ZOOM", 1);
		int zoom = zoomvar->value;
		if (!nomousedownbutton)
			mouseover = getElement(getPixel((x - MENU_LEFT + scroll_left - 1) / zoom + 1, (y - MENU_TOP + scroll_top - 1) / zoom + 2))->name;
		vx->value = drawx;
		vy->value = drawy;
	}
	int dx, dy;
	int i;
	if (subMenu != -1)
		i = 4;
	else
		i = 3;
	if (vconsole->value) {
		SDL_Surface* console = getConsoleSurface();
		if (y < console->h) {
			if (b && click && MENU_TOP && MENU_RIGHT && (x > screen->w - 14) && (y > console->h - 7))
				if (b == 1) vconsole->value = !vconsole->value;
				else if (b == 2) ossystem("telnet", "localhost 7777", false);
				else if (b == 4) ossystem("notepad", "console.txt", false);
			return;
		}
	} else if (b && click && MENU_TOP && MENU_RIGHT && (x > screen->w - 14) && (y < 7)) {
		if (b == 1) vconsole->value = !vconsole->value;
		else if (b == 2) ossystem("telnet", "localhost 7777", false);
		else if (b == 4) ossystem("notepad", "console.txt", false);
		return;
	}
	int inmenu = 0;
	for (; i >= 0; i--) {
		dx = menurects[i].x;
		dy = menurects[i].y;
		it = menubuttons[i].begin();
		if ((menusurfaces[i]) && (x > dx) && (y > dy) && (x < menurects[i].x + menurects[i].w) && (y < menurects[i].y + menurects[i].h)) {
			inmenu = 1;
			if ((i == MENU_BAR_LEFT) || (i == MENU_BAR_RIGHT) || ((i == 4) && ((subMenu == MENU_BAR_TOP) || (subMenu == MENU_BAR_BOTTOM)))) {
				scroll[i] = y - menurects[i].y;
				redrawmenus = true;
			} else {
				scroll[i] = x - menurects[i].x;
				redrawmenus = true;
			}
			dx -= realscrollx[i];
			dy -= realscrolly[i];
			while (it != menubuttons[i].end()) {
				if ((((*it)->icon)->type[0] == 'S') && (!strcmp(((*it)->icon)->type, "SEPERATOR") || !strcmp(((*it)->icon)->type, "SEPARATOR"))) {
					it++;
					continue;
				}
				SDL_Surface* icon = ((*it)->icon)->getpic();
				if ((((*it)->left + dx < x) && ((*it)->left + icon->w + dx > x) && ((*it)->top + dy < y) && ((*it)->top + icon->h + dy > y))) {
					if (!nomousedownbutton || ((*it)->id == nomousedownbutton))
						mouseover = (*it)->tiptext;
					if ((click || b) && (*it)->click != NULL && (!nomousedownbutton || ((*it)->id == nomousedownbutton))) {
						if (!(*it)->mode)
							nomousedown = true;
						else
							nomousedownbutton = (*it)->id;
						addparams((*it)->params);
						((Trigger*)((*it)->click))->exec(x - dx - (*it)->left - 1, y - dy - (*it)->top + -1, b, click);
						removeparams(false);
						if (subMenu == -2) {
							static int oldi = 0;
							if (menubuttons[4].size() == 0) {
								subMenu = -1;
								return;
							}
							if (i == 4)
								subMenu = oldi;
							else
								subMenu = i;
							int oldw = 0, oldh = 0;
							if (menusurfaces[4] != 0) {
								oldw = menusurfaces[4]->w;
								oldh = menusurfaces[4]->h;
								SDL_FreeSurface(menusurfaces[4]);
							}
							if (subMenuAlign)
								if ((subMenu == MENU_BAR_LEFT) || (subMenu == MENU_BAR_RIGHT))
									menusurfaces[4] = createmenu(&(menubuttons[4]), subMenuAlign, screen->w - MENU_RIGHT - MENU_LEFT, 1000000, 0, true);
								else
									menusurfaces[4] = createmenu(&(menubuttons[4]), subMenuAlign, 1000000, screen->h - MENU_TOP - MENU_BOTTOM, 0, true);
							else {
								if ((subMenu == MENU_BAR_LEFT) || (subMenu == MENU_BAR_RIGHT))
									menusurfaces[4] = createmenu(&(menubuttons[4]), MENU_ALIGN_H, screen->w - MENU_RIGHT - MENU_LEFT, 1000000, 0, true);
								else
									menusurfaces[4] = createmenu(&(menubuttons[4]), MENU_ALIGN_V, 1000000, screen->h - MENU_TOP - MENU_BOTTOM, 0, true);
							}
							if (i == MENU_BAR_TOP) {
								menurects[4].x = (*it)->left + dx - menusurfaces[4]->w / 2 + icon->w / 2;
								menurects[4].y = (*it)->top + dy + MENU_TOP;
							} else if (i == MENU_BAR_LEFT) {
								menurects[4].x = dx + MENU_LEFT;
								menurects[4].y = (*it)->top + dy;
							} else if (i == MENU_BAR_RIGHT) {
								menurects[4].x = dx - menusurfaces[4]->w;
								menurects[4].y = (*it)->top + dy;
							} else if (i == MENU_BAR_BOTTOM) {
								menurects[4].x = (*it)->left + dx - menusurfaces[4]->w / 2 + icon->w / 2;
								menurects[4].y = (*it)->top + dy - menusurfaces[4]->h;
							} else {
								if (oldi == MENU_BAR_TOP) {
									menurects[4].y = MENU_TOP;
									menurects[4].x -= (menusurfaces[4]->w - oldw) / 2;
								}
								if (oldi == MENU_BAR_LEFT) {
									menurects[4].y -= (menusurfaces[4]->h - oldh) / 2;
									menurects[4].x = MENU_LEFT;
								}
								if (oldi == MENU_BAR_RIGHT) {
									menurects[4].y -= (menusurfaces[4]->h - oldh) / 2;
									menurects[4].x = screen->w - MENU_RIGHT - menusurfaces[4]->w + 1;
								}
								if (oldi == MENU_BAR_BOTTOM) {
									menurects[4].y = screen->h - MENU_BOTTOM - menusurfaces[4]->h + 1;
									menurects[4].x -= (menusurfaces[4]->w - oldw) / 2;
								}
							}
							if (i != 4)
								oldi = i;
						} else if (subMenu == -4)
							subMenu = 4;
						else if ((subMenu != -1) && (subMenuStay == 0)) {
							subMenu = -1;
							redrawmenu(3);
						}
					}
					SDL_FreeSurface(icon);
					return;
				}
				SDL_FreeSurface(icon);
				it++;
			}
		}
	}
	if ((inmenu == 0) && (x > MENU_LEFT) && (x < screen->w - MENU_RIGHT) && (y > MENU_TOP) && (y < screen->h - MENU_BOTTOM) && (nomousedownbutton <= 1)) {
		findTrigger("MOUSEMOVE", 0)->exec(drawx, drawy, b, false);
		if (b) {
			if (click)
				findTrigger("MOUSEDOWN", 0)->exec(drawx, drawy, b, true);
			findTrigger("DRAG", 0)->exec(drawx, drawy, b, click);
			if (!click && solid->value) {
				int dx = drawx - lastx;
				int dy = drawy - lasty;
				int dist = (int)sqrt((float)(dx * dx + dy * dy)) + 1;
				for (int i = 0; i < dist; i++) {
					findTrigger("DRAW", 0)->exec(lastx + dx * i / dist, lasty + dy * i / dist, b, click);
					click = false;
				}
			} else findTrigger("DRAW", 0)->exec(drawx, drawy, b, click);
			nomousedownbutton = 1;
		}
	}
	lastx = drawx;
	lasty = drawy;
	if ((b) && (subMenu != -1)) {
		if (subMenu == -4)
			subMenu = 4;
		else if (subMenuStay == 0) {
			subMenu = -1;
		}
		redrawmenu(3);
	}
}

void redrawmenu(int i) {
	if (redraw < i)
		redraw = i;
}

void addButtonToMenuBar(int i, Button* button) {
	if ((i >= 0) && (i < 5))
		menubuttons[i].push_back(button);
	redrawmenu(3);
}

void clearMenuBar(int i) {
	if ((i >= 0) && (i < 5)) {
		for (std::list<Button*>::iterator it = menubuttons[i].begin(); it != menubuttons[i].end(); it++) {
			delPic((*it)->icon);
			delete (*it);
		}
		menubuttons[i].clear();
	}
	redrawmenu(3);
}

void showSubMenu(int stay, int align) {
	subMenuStay = stay;
	subMenu = -2;
	subMenuAlign = align;
}

void showSubMenu(int stay, int align, int x, int y) {
	if (align == 0)
		align = MENU_ALIGN_V;
	subMenuStay = stay;
	if (menubuttons[4].size() == 0) {
		subMenu = -1;
		return;
	}
	if (lastscreen != NULL)
		menusurfaces[4] = createmenu(&(menubuttons[4]), align, 1000000, lastscreen->h, 0, true);
	menurects[4].x = x;
	menurects[4].y = y;
	subMenu = -4;
	redrawmenu(3);
}

void hideSubMenu() {
	subMenu = -1;
	redrawmenu(3);
}

int getmenuwidth(int i) {
	if (i == MENU_BAR_TOP)
		return MENU_TOP;
	else if (i == MENU_BAR_LEFT)
		return MENU_LEFT;
	else if (i == MENU_BAR_RIGHT)
		return MENU_RIGHT;
	else if (i == MENU_BAR_BOTTOM)
		return MENU_BOTTOM;
	return 0;
}

Button::Button() {
	click = NULL;
	mode = 0;
}

Button::Button(Button* b) {
	params = b->params;
	click = b->click;
	id = b->id;
	mode = b->mode;
	icon = getPic(b->icon);
	tiptext = b->tiptext;
}

SDL_Surface* createmenu(std::list<Button*>* buttons, int align, int width, int height, int scroll, bool crop) {
	const int MAXLINES = 32;
	int line = 0;
	scroll = 0;
	int seperators[MAXLINES];
	int minus[MAXLINES];
	int w[MAXLINES];
	int h[MAXLINES];
	for (int i = 0; i < MAXLINES; i++) {
		seperators[i] = 0;
		minus[i] = 0;
		w[i] = 0;
		h[i] = 0;
	}
	std::list<Button*>::iterator it = buttons->begin();
	while (it != buttons->end()) {
		if ((*it) && (*it)->icon) {
			SDL_Surface* icon = ((*it)->icon)->getpic();
			if ((strcmp(((*it)->icon)->type, "NEWLINE") == 0) && (line < MAXLINES - 1)) {
				if (align == MENU_ALIGN_V)
					while ((line > 0) && (h[line] == 0))
						line--;
				if (align == MENU_ALIGN_H)
					while ((line > 0) && (w[line] == 0))
						line--;
				int i = 0;
				getVar(((*it)->icon)->text, &i);
				if (i != 0) {
					if (align == MENU_ALIGN_H)
						h[line] = i;
					if (align == MENU_ALIGN_V)
						w[line] = i;
				}
				line++;
			} else if ((strcmp(((*it)->icon)->type, "SEPERATOR") == 0) || (strcmp(((*it)->icon)->type, "SEPARATOR") == 0)) {
				int i = 0;
				getVar(((*it)->icon)->text, &i);
				if (i > 0) {
					if (align == MENU_ALIGN_H)
						w[line] += i;
					if (align == MENU_ALIGN_V)
						h[line] += i;
				} else if (i == 0)
					seperators[line]++;
				else if (i < 0) {
					minus[line] -= i;
				}
			} else {
				if (align == MENU_ALIGN_H) {
					if (h[line] < icon->h)
						h[line] = icon->h;
					if (minus[line] <= 0)
						w[line] += icon->w;
					else {
						if (minus[line] >= icon->w) {
							minus[line] -= icon->w;
						} else {
							w[line] += icon->w - minus[line];
							minus[line] = 0;
						}
					}
				}
				if (align == MENU_ALIGN_V) {
					if (w[line] < icon->w)
						w[line] = icon->w;
					if (minus[line] <= 0)
						h[line] += icon->h;
					else {
						if (minus[line] >= icon->h) {
							minus[line] -= icon->h;
						} else {
							h[line] += icon->h - minus[line];
							minus[line] = 0;
						}
					}
				}
			}
			SDL_FreeSurface(icon);
		}
		it++;
	}
	if (align == MENU_ALIGN_V)
		while ((line > 0) && (h[line] == 0))
			line--;
	if (align == MENU_ALIGN_H)
		while ((line > 0) && (w[line] == 0))
			line--;
	if ((h[line] == 0) || (w[line] == 0))
		return 0;
	SDL_Rect dstrect;
	dstrect.x = 0;
	dstrect.y = 0;
	it = buttons->begin();
	int maxwidth = 0;
	int maxheight = 0;
	if (align == MENU_ALIGN_H)
		for (int i = 0; i < MAXLINES; i++) {
			if (w[i] > maxwidth)
				maxwidth = w[i];
			maxheight += h[i];
		} else
			for (int i = 0; i < MAXLINES; i++) {
				maxwidth += w[i];
				if (h[i] > maxheight)
					maxheight = h[i];
			}
		if (width == 0)
			width = maxwidth;
		if (width == 0)
			height = maxheight;
		SDL_Surface* s;
		if ((align == MENU_ALIGN_V) && (width < w[line]))
			maxwidth = width;
		if ((align == MENU_ALIGN_H) && (height < h[line]))
			maxheight = height;
		if (crop == true) {
			s = SDL_CreateRGBSurface(SDL_SWSURFACE, maxwidth, maxheight, 16, 0, 0, 0, 0);
			SDL_FillRect(s, 0, background);
		} else {
			if (width < maxwidth)
				width = maxwidth;
			if (height < maxheight)
				height = maxheight;
			s = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 16, 0, 0, 0, 0);
			SDL_FillRect(s, 0, background);
		}
		line = 0;
		int leftoffset = 0;
		while (it != buttons->end()) {
			if ((strcmp(((*it)->icon)->type, "NEWLINE") == 0) && (line < MAXLINES - 1)) {
				if (align == MENU_ALIGN_H) {
					dstrect.x = 0;
					dstrect.y += h[line];
				}
				if (align == MENU_ALIGN_V) {
					leftoffset += w[line];
					dstrect.y = 0;
				}
				it++;
				line++;
			} else if ((strcmp(((*it)->icon)->type, "SEPERATOR") == 0) || (strcmp(((*it)->icon)->type, "SEPARATOR") == 0)) {
				int i = 0;
				getVar(((*it)->icon)->text, &i);
				if (i == 0) {
					if (crop == true) {
						if (align == MENU_ALIGN_H)
							i = (maxwidth - w[line]) / seperators[line];
						if (align == MENU_ALIGN_V)
							i = (maxheight - h[line]) / seperators[line];
					} else {
						if (align == MENU_ALIGN_H)
							i = (width - w[line]) / seperators[line];
						if (align == MENU_ALIGN_V)
							i = (height - h[line]) / seperators[line];
					}
					if (i < 0)
						i = 0;
				}
				if (align == MENU_ALIGN_H)
					dstrect.x += i;
				if (align == MENU_ALIGN_V)
					dstrect.y += i;
				it++;
			} else {
				SDL_Surface* tmpsurface;
				tmpsurface = ((*it)->icon)->getpic();
				if (align == MENU_ALIGN_V)
					dstrect.x = leftoffset + (w[line] - tmpsurface->w) / 2;
				SDL_BlitSurface(tmpsurface, 0, s, &dstrect);
				if ((*it)->border)
					SDL_DrawRect16(s, dstrect.x, dstrect.y, tmpsurface->w - 1, tmpsurface->h - 1, SDL_MapRGB(s->format, (*it)->r, (*it)->g, (*it)->b));
				(*it)->top = dstrect.y;
				(*it)->left = dstrect.x;
				if (align == MENU_ALIGN_H)
					dstrect.x += tmpsurface->w;
				if (align == MENU_ALIGN_V)
					dstrect.y += tmpsurface->h;
				it++;
				SDL_FreeSurface(tmpsurface);
			}
		}
		return s;
}

void scrollto(int x, int y) {
	scroll_left = x;
	scroll_top = y;
	checkscroll();
}

char* getmouseover() {
	return mouseover;
}

void autoresize(SDL_Surface* screen) {
	static Var* h = (Var*)setVar("WINDOWHEIGHT", 0);
	static Var* w = (Var*)setVar("WINDOWWIDTH", 0);
	h->value = screen->h - MENU_TOP - MENU_BOTTOM - 1;
	w->value = screen->w - MENU_LEFT - MENU_RIGHT - 1;
	findTrigger("WINDOWRESIZED", 0)->exec();
}