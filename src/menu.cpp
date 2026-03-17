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
	static int vmenutopold = 0;
	static int vmenubottomold = 0;
	static int vmenuleftold = 0;
	static int vmenurightold = 0;
	static int vstatusold = 0;
	static int zoom = 0;

	if (vconsole->value != lastconsole) redraw = 3;
	lastconsole = vconsole->value;

	border = SDL_MapRGB(screen->format, borderr->value, borderg->value, borderb->value);
	background = SDL_MapRGB(screen->format, backgroundr->value, backgroundg->value, backgroundb->value);
	menufontfgcolor = {static_cast<Uint8>(menufontcolorr->value), static_cast<Uint8>(menufontcolorg->value), static_cast<Uint8>(menufontcolorb->value), 0};
	menufontbgcolor = {static_cast<Uint8>(backgroundr->value), static_cast<Uint8>(backgroundg->value), static_cast<Uint8>(backgroundb->value), 0};

	if ((vstatusold != vstatus->value) || (vmenutopold != vmenutop->value) || (vmenuleftold != vmenuleft->value) || (vmenurightold != vmenuright->value) || (vmenubottomold != vmenubottom->value)) {
		vmenutopold = vmenutop->value;
		vmenuleftold = vmenuleft->value;
		vmenurightold = vmenuright->value;
		vmenubottomold = vmenubottom->value;
		vstatusold = vstatus->value;

		MENU_TOP = vmenutop->value - 1;
		MENU_BOTTOM = vmenubottom->value - 1;
		MENU_LEFT = vmenuleft->value - 1;
		MENU_RIGHT = vmenuright->value;
		MENU_BOTTOM += vstatus->value ? 19 : 1;

		SDL_FillRect(screen, nullptr, background);
		redraw = 3;
		SDL_SemPost(screensem);
		autoresize(screen);
		return;
	}

	if (zoom != zoomvar->value && redraw < 2) redraw = 2;
	zoom = zoomvar->value;

	const int maxwidth = screen->w - MENU_RIGHT - MENU_LEFT;
	const int maxheight = screen->h - MENU_TOP - MENU_BOTTOM;

	if (redraw >= 3) {
		menurects[0] = {static_cast<Sint16>(MENU_LEFT), 0, static_cast<Uint16>(maxwidth), static_cast<Uint16>(MENU_TOP)};
		menurects[1] = {0, static_cast<Sint16>(MENU_TOP), static_cast<Uint16>(MENU_LEFT), static_cast<Uint16>(maxheight)};
		menurects[2] = {static_cast<Sint16>(screen->w - MENU_RIGHT + 1), static_cast<Sint16>(MENU_TOP), static_cast<Uint16>(MENU_RIGHT), static_cast<Uint16>(maxheight)};
		menurects[3] = {static_cast<Sint16>(MENU_LEFT), static_cast<Sint16>(screen->h - MENU_BOTTOM + 1), static_cast<Uint16>(maxwidth), static_cast<Uint16>(MENU_BOTTOM - vstatus->value * 18)};

		auto rebuildBar = [&](int idx, int align, int w, int h) {
			if (menusurfaces[idx]) {
				SDL_FreeSurface(menusurfaces[idx]);
				menusurfaces[idx] = nullptr;
			}
			menusurfaces[idx] = createmenu(&menubuttons[idx], align, w, h);
		};

		if (MENU_TOP != -1) rebuildBar(0, MENU_ALIGN_H, maxwidth, MENU_TOP);
		if (MENU_LEFT != -1) rebuildBar(1, MENU_ALIGN_V, MENU_LEFT, maxheight);
		if (MENU_RIGHT != -1) rebuildBar(2, MENU_ALIGN_V, MENU_RIGHT, maxheight);
		if (MENU_BOTTOM != -1) rebuildBar(3, MENU_ALIGN_H, maxwidth, MENU_BOTTOM - vstatus->value * 18);
	}

	checkscroll();

	SDL_Rect dstrect = {};
	dstrect.x = static_cast<Sint16>(MENU_LEFT + 1);
	dstrect.y = static_cast<Sint16>(MENU_TOP + 1);

	SDL_Rect srcrect = {};
	srcrect.x = static_cast<Sint16>(1 + scroll_left);
	srcrect.y = static_cast<Sint16>(scroll_top);
	srcrect.w = static_cast<Uint16>(screen->w - MENU_LEFT - MENU_RIGHT - 1);
	srcrect.h = static_cast<Uint16>(screen->h - MENU_TOP - MENU_BOTTOM - 1);

	if (srcrect.w + zoom * 2 > sandsurface->w * zoom) {
		dstrect.x = static_cast<Sint16>(MENU_LEFT + srcrect.w / 2 - sandsurface->w / 2 * zoom + zoom + 1);
		scroll_left = -srcrect.w / 2 + sandsurface->w / 2 * zoom + 1 - zoom + 1;
		srcrect.w = static_cast<Uint16>((sandsurface->w - 2) * zoom);
		srcrect.x = 1;
	}
	if (srcrect.h + zoom * 2 - 1 > sandsurface->h * zoom) {
		dstrect.y += static_cast<Sint16>(srcrect.h / 2 - sandsurface->h / 2 * zoom + zoom - 1);
		scroll_top = -srcrect.h / 2 + sandsurface->h / 2 * zoom - zoom - 1;
		srcrect.h = static_cast<Uint16>(sandsurface->h * zoom - zoom * 2 + 2);
		srcrect.y = 0;
	}

	// ── Background / border clear (redraw >= 2) ───────────────────────────────
	if (redraw >= 2) {
		if ((vview->value == 10) ||
			((valpha1->value < 256) && (vview->value == 0))) {
			clearrects[0] = {0, 0, static_cast<Uint16>(dstrect.x), static_cast<Uint16>(screen->h)};
			clearrects[1] = {static_cast<Sint16>(dstrect.x), 0, 0, 0};
			clearrects[2] = {static_cast<Sint16>(dstrect.x), static_cast<Sint16>(dstrect.y + srcrect.h), 0, static_cast<Uint16>(screen->h - (dstrect.y + srcrect.h))};
			clearrects[3] = {static_cast<Sint16>(dstrect.x + srcrect.w), 0, static_cast<Uint16>(screen->w - (dstrect.x + srcrect.w)), static_cast<Uint16>(screen->h)};
			for (auto& r : clearrects) SDL_FillRect(screen, &r, background);
		} else SDL_FillRect(screen, nullptr, background);
		SDL_DrawRect16(screen, MENU_LEFT, MENU_TOP, screen->w - MENU_RIGHT - MENU_LEFT, screen->h - MENU_BOTTOM - MENU_TOP, border);
	}

	if (redraw >= 0) {
		SDL_DrawLine16(screen, dstrect.x - 1, MENU_TOP, 0, dstrect.y + srcrect.h - MENU_TOP, border);
		SDL_DrawLine16(screen, dstrect.x + srcrect.w, dstrect.y, 0, screen->h - MENU_BOTTOM - dstrect.y, border);
		SDL_DrawLine16(screen, dstrect.x, dstrect.y - 1, screen->w - dstrect.x - MENU_RIGHT, 0, border);
		SDL_DrawLine16(screen, MENU_LEFT, dstrect.y + srcrect.h, dstrect.x + srcrect.w - MENU_LEFT, 0, border);
	}

	// ── Blit menu bars (redraw >= 0 or redrawmenus) ───────────────────────────
	if ((redraw >= 0) || redrawmenus) {
		auto blitBar = [&](int idx, bool horizontal,
			int sz, int menuSz, int limit) {
				if (!menusurfaces[idx]) return;
				SDL_Rect r = {};
				if (horizontal) {
					r.x = static_cast<Sint16>((scroll[idx] + 1) * 1000 * (menusurfaces[idx]->w + 1 - sz) / sz / 1000);
					r.w = static_cast<Uint16>(sz);
					r.h = static_cast<Uint16>(menuSz);
					realscrollx[idx] = r.x;
					realscrolly[idx] = 0;
					if (r.x > menusurfaces[idx]->w - sz)
						r.x = static_cast<Sint16>(menusurfaces[idx]->w - sz);
				} else {
					r.y = static_cast<Sint16>((scroll[idx] + 1) * 1000 * (menusurfaces[idx]->h + 1 - sz) / sz / 1000);
					r.h = static_cast<Uint16>(sz);
					r.w = static_cast<Uint16>(menuSz);
					realscrollx[idx] = 0;
					realscrolly[idx] = r.y;
					if (r.y > menusurfaces[idx]->h - sz)
						r.y = static_cast<Sint16>(menusurfaces[idx]->h - sz);
				}
				SDL_BlitSurface(menusurfaces[idx], &r, screen, &menurects[idx]);
			};

		if (vmenutop->value != 0) blitBar(0, true, maxwidth, MENU_TOP, 0);
		if (vmenubottom->value != 0) blitBar(3, true, maxwidth, MENU_BOTTOM - vstatus->value * 19, 0);
		if (vmenuleft->value != 0) blitBar(1, false, maxheight, MENU_LEFT, 0);
		if (vmenuright->value != 0) blitBar(2, false, maxheight, MENU_RIGHT, 0);
	}

	// ── Preview draw ──────────────────────────────────────────────────────────
	static Var* vpreview = reinterpret_cast<Var*>(setVar("PREVIEW", 0));
	static bool lastpreview = false;
	if (oversand && vshowpreview->value) {
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

	// ── Sand surface rendering ────────────────────────────────────────────────
	if (!vview->value) {
		if (zoom == 1) {
			if (bglayer) {
				SDL_Rect r = {srcrect.x, static_cast<Sint16>(1 + srcrect.y), srcrect.w, srcrect.h};
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
				SDL_SetColorKey(sandsurface,
					bglayer ? SDL_SRCCOLORKEY : 0,
					SDL_MapRGB(sandsurface->format, getElement(0)->r, getElement(0)->g, getElement(0)->b));
				SDL_SetAlpha(sandsurface, SDL_RLEACCEL, 255);
				SDL_BlitSurface(sandsurface, &srcrect, screen, &dstrect);
			}
			if (fglayer) {
				SDL_Rect r = {srcrect.x, static_cast<Sint16>(1 + srcrect.y), srcrect.w, srcrect.h};
				SDL_SetColorKey(fglayer,
					fgtransparentcolor->value ? SDL_SRCCOLORKEY : 0,
					SDL_MapRGB(fglayer->format, fgtransparentcolorr->value, fgtransparentcolorg->value, fgtransparentcolorb->value));
				SDL_SetAlpha(fglayer, SDL_SRCALPHA | SDL_RLEACCEL, fglayeralpha->value);
				SDL_BlitSurface(fglayer, &r, screen, &dstrect);
			}
		} else {
			Uint16* p = static_cast<Uint16*>(screen->pixels);
			Uint16* pz = static_cast<Uint16*>(getSandSurface()->pixels);
			const int ypitch = screen->pitch / 2;
			const int ypitchz = getSandSurface()->pitch / 2;
			const int dy = scroll_top - MENU_TOP - 2 + zoom;
			const int dx = scroll_left - MENU_LEFT + zoom - 1;
			const int xto = dstrect.x + srcrect.w;
			for (int y = dstrect.y; y < dstrect.y + srcrect.h; y++) {
				Uint16* yp = p + y * ypitch;
				Uint16* ypz = pz + ((y + dy) / zoom) * ypitchz;
				for (int x = dstrect.x; x < xto; x++)
					*(yp + x) = *(ypz + (x + dx) / zoom);
			}
		}
	} else {
		const int view = vview->value;
		const int maxpos = vviewmaxpos->value;
		const int maxneg = -vviewmaxneg->value;

		Uint16* colormappositiv = new Uint16[maxpos];
		Uint16* colormapnegativ = new Uint16[-maxneg];
		Uint16 colormapneutral = 0;

		const int maxelements = getelementsmax();
		Uint16* colormap = new Uint16[maxelements + 1];
		Element* e = getElement(0);

		if (view > 0) {
			colormapneutral = SDL_MapRGB(screen->format,
				vviewr->value, vviewg->value, vviewb->value);
			for (int i = 0; i < maxpos; i++)
				colormappositiv[i] = SDL_MapRGB(screen->format, i * vviewposr->value / maxpos, i * vviewposg->value / maxpos, i * vviewposb->value / maxpos);
			for (int i = 0; i < -maxneg; i++)
				colormapnegativ[i] = SDL_MapRGB(screen->format, -(i * vviewnegr->value / maxneg), -(i * vviewnegg->value / maxneg), -(i * vviewnegb->value / maxneg));
		}
		if (view == -1)
			for (int i = 0; i < maxelements; i++)
				colormap[i] = SDL_MapRGB(screen->format, e[i].cr1, e[i].cg1, e[i].cb1);
		if (view == -2)
			for (int i = 0; i < maxelements; i++)
				colormap[i] = SDL_MapRGB(screen->format, e[i].cr2, e[i].cg2, e[i].cb2);
		if (view == -3)
			for (int i = 0; i < maxelements; i++)
				colormap[i] = SDL_MapRGB(screen->format, e[i].cr3, e[i].cg3, e[i].cb3);

		Uint16* p = static_cast<Uint16*>(screen->pixels);
		Uint16* pz;
		int ypitchz;
		if (view < 10) {
			pz = static_cast<Uint16*>(getRealSandSurface()->pixels);
			ypitchz = getRealSandSurface()->pitch / 2;
		} else {
			pz = static_cast<Uint16*>(getSandSurface()->pixels);
			ypitchz = getSandSurface()->pitch / 2;
		}
		const int ypitch = screen->pitch / 2;
		const int dy = scroll_top - MENU_TOP - 2 + zoom;
		const int dx = scroll_left - MENU_LEFT + zoom - 1;
		const int alpha1 = valpha1->value;
		const int alpha2 = valpha2->value;
		const int blurfactor1 = (view == 12) ? 3 : 1;
		const int blurfactor2 = 1;
		const int blurfactor = 2;

		for (int y = dstrect.y; y < dstrect.y + srcrect.h; y++) {
			Uint16* yp = p + y * ypitch;
			Uint16* ypz;
			if (zoom == 1)
				ypz = pz + (((y + dy) / zoom) + (view > 9 ? 2 : 3)) * ypitchz;
			else
				ypz = pz + (((y + dy) / zoom) + (view > 9 ? 0 : 1)) * ypitchz - 1;

			const int xto = dstrect.x + srcrect.w;

			if (view > 0 && view < 9) {
				for (int x = dstrect.x; x < xto; x++) {
					const int elem = *(ypz + (x + dx) / zoom);
					int t = 0;
					switch (view) {
					case 1: t = e[elem].weight;        break;
					case 2: t = e[elem].spray;         break;
					case 3: t = e[elem].slide;         break;
					case 4: t = e[elem].viscousity;    break;
					case 5: t = e[elem].dietotalrate;  break;
					case 6: t = e[elem].r;             break;
					case 7: t = e[elem].g;             break;
					case 8: t = e[elem].b;             break;
					}
					if (!t)                       *(yp + x) = colormapneutral;
					else if (t > 0 && t < maxpos)      *(yp + x) = colormappositiv[t];
					else if (t < 0 && t > maxneg)      *(yp + x) = colormapnegativ[-t];
					else if (t > 0)                    *(yp + x) = colormappositiv[maxpos - 1];
					else                               *(yp + x) = colormapnegativ[-(maxneg + 1)];
				}
			} else if (view == 10) {
				for (int x = dstrect.x; x < xto; x++) {
					const int b1 = *(yp + x);
					const int a1 = *(ypz + (x + dx) / zoom);
					if (b1 == a1) continue;
					int r = (a1 & 63488) - (b1 & 63488);
					int g = (a1 & 1984) - (b1 & 1984);
					int b = (a1 & 31) - (b1 & 31);

					if (r) {
						if (r > 2048 * alpha1) r = 2048 * alpha1;
						else if (r < -2048 * alpha2) r = -2048 * alpha2;
					}
					if (g) {
						if (g > 64 * alpha1) g = 64 * alpha1;
						else if (g < -64 * alpha2) g = -64 * alpha2;
					}
					if (b) {
						if (b > alpha1) b = alpha1;
						else if (b < -alpha2) b = -alpha2;
					}
					*(yp + x) = static_cast<Uint16>(b1 + r + g + b);
				}
			} else if (view == 11) {
				for (int x = dstrect.x; x < xto; x++)
					*(yp + x) = static_cast<Uint16>(
						(*(yp + x) + *(ypz + (x + dx) / zoom)) / 2);
			} else if (view == 12) {
				for (int x = dstrect.x; x < xto; x++) {
					const int a = *(yp + x);
					const int b = *(ypz + (x + dx) / zoom);
					*(yp + x) = static_cast<Uint16>(b
						+ (((a & 63488) * blurfactor1 - (b & 63488) * blurfactor2) / blurfactor & 63488)
						+ (((a & 2016) * blurfactor1 - (b & 2016) * blurfactor2) / blurfactor & 2016)
						+ (((a & 31) * blurfactor1 - (b & 31) * blurfactor2) / blurfactor & 31));
				}
			} else {
				for (int x = dstrect.x; x < xto; x++)
					*(yp + x) = colormap[*(ypz + (x + dx) / zoom)];
			}
		}

		delete[] colormappositiv;
		delete[] colormapnegativ;
		delete[] colormap;
	}

	if ((subMenu != -1) && (subMenu != -2) && (menusurfaces[4] || redrawmenus)) {
		if (menurects[4].x + menusurfaces[4]->w > screen->w)
			menurects[4].x = static_cast<Sint16>(screen->w - menusurfaces[4]->w);
		if (menurects[4].y + menusurfaces[4]->h + vstatus->value * 18 > screen->h)
			menurects[4].y = static_cast<Sint16>(screen->h - menusurfaces[4]->h - vstatus->value * 18);
		if (menurects[4].x < 0) menurects[4].x = 0;
		if (menurects[4].y < 0) menurects[4].y = 0;

		SDL_Rect r = {};
		const bool topOrBottom = (subMenu == MENU_BAR_TOP) || (subMenu == MENU_BAR_BOTTOM);
		if (topOrBottom) {
			r.y = static_cast<Sint16>((scroll[4] + 1) * 1000 * (menusurfaces[4]->h + 1 - maxheight) / maxheight / 1000);
			if (r.y < 0) r.y = 0;
			r.h = static_cast<Uint16>(maxheight);
			r.w = 32000;
			realscrolly[4] = r.y;
			realscrollx[4] = 0;
		} else {
			r.x = static_cast<Sint16>((scroll[4] + 1) * 1000 * (menusurfaces[4]->w + 1 - maxwidth) / maxwidth / 1000);
			if (r.x < 0) r.x = 0;
			r.w = static_cast<Uint16>(maxwidth);
			r.h = 32000;
			realscrollx[4] = r.x;
			realscrolly[4] = 0;
		}
		SDL_BlitSurface(menusurfaces[4], &r, screen, &menurects[4]);

		const int h = std::min(menusurfaces[4]->h, maxheight);
		const int w = std::min(menusurfaces[4]->w, maxwidth);

		if (subMenu != MENU_BAR_TOP) SDL_DrawLine16(screen, menurects[4].x, menurects[4].y, w - 1, 0, border);
		if (subMenu != MENU_BAR_LEFT) SDL_DrawLine16(screen, menurects[4].x - 1, menurects[4].y, 0, h - 1, border);
		if (subMenu != MENU_BAR_RIGHT) SDL_DrawLine16(screen, menurects[4].x + menusurfaces[4]->w, menurects[4].y, 0, h - 1, border);
		if (subMenu != MENU_BAR_BOTTOM) SDL_DrawLine16(screen, menurects[4].x, menurects[4].y + h, w - 1, 0, border);
	}

#ifdef COMPILER_SDL_TTF
	if (vconsole->value) {
		consolenews = false;
		dstrect.x = 0;
		dstrect.y = 0;
		SDL_Surface* console = getConsoleSurface();
		SDL_BlitSurface(console, nullptr, screen, &dstrect);
		SDL_DrawLine16(screen, screen->w - 15, console->h, 7, -7, border);
		SDL_DrawLine16(screen, screen->w - 1, console->h, -7, -7, border);
		SDL_DrawLine16(screen, 0, console->h, screen->w, 0, border);
	} else if (((redraw >= 2) || consolenews) && MENU_TOP && MENU_RIGHT) {
		if (consolenews) {
			for (int s = 1; s < 7; s++) {
				SDL_DrawLine16(screen, screen->w - 1 - s * 2, 0, s, s, border);
				SDL_DrawLine16(screen, screen->w - 1, 0, -s, s, border);
			}
		} else {
			constexpr int s = 5;
			SDL_DrawLine16(screen, screen->w - 1 - s * 2, 0, s, s, border);
			SDL_DrawLine16(screen, screen->w - 1, 0, -s, s, border);
		}
	}

	static char status_textold[4097] = {};
	if (vstatusold && (strcmp(status_text, status_textold) || (redraw >= 2))) {
		SDL_DrawLine16(screen, 0, screen->h - 18, screen->w, 0, border);
		SDL_Rect r = {0, static_cast<Sint16>(screen->h - 17), static_cast<Uint16>(screen->w),  17};
		SDL_FillRect(screen, &r, background);
		std::strcpy(status_textold, status_text);
		SDL_DrawText16(screen, status_font, status_text, 2, screen->h - 2, border);
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
	static int lx = 0, ly = 0;
	static bool nomousedown = false;
	static int nomousedownbutton = 0;

	const int drawx = (x - MENU_LEFT + scroll_left - 1) / zoomvar->value + 1;
	const int drawy = (zoomvar->value == 1) ? (y - MENU_TOP + scroll_top - 1) / zoomvar->value + 2 : (y - MENU_TOP + scroll_top - 2) / zoomvar->value + 2;

	if (nomousedown) {
		if (b != 0) return;
		nomousedown = false;
	}

	if (nomousedownbutton && !b) {
		nomousedownbutton = 0;
		findTrigger("MOUSEUP", 0)->exec(drawx, drawy, b, click);
	}

	if ((lx == x) && (ly == y) && !b && !click) return;
	lx = x;
	ly = y;

	int cursor = 0;
	if (x <= MENU_LEFT) cursor = 1;
	else if (x >= lastscreen->w - MENU_RIGHT) cursor = 1;
	else if (y <= MENU_TOP) cursor = 1;
	else if (y >= lastscreen->h - MENU_BOTTOM) cursor = 1;
	else if ((subMenu != -1) && (subMenu != -2))
		if ((x > menurects[4].x) && (x < menurects[4].x + menusurfaces[4]->w) && (y - 1 > menurects[4].y) && (y - 1 < menurects[4].y + menusurfaces[4]->h))
			cursor = 1;

	if (cursor) {
		oversand = false;
		SDL_SetCursor(menucursor);
		if (!nomousedownbutton) mouseover = "Nothing";
		vx->value = 0;
		vy->value = 0;
	} else {
		oversand = true;
		SDL_SetCursor(sandcursor[vcursor->value % MAX_CURSORS]);
		static Var* zoomvar = reinterpret_cast<Var*>(setVar("ZOOM", 1));
		const int zoom = zoomvar->value;
		if (!nomousedownbutton)
			mouseover = getElement(getPixel((x - MENU_LEFT + scroll_left - 1) / zoom + 1, (y - MENU_TOP + scroll_top - 1) / zoom + 2))->name;
		vx->value = drawx;
		vy->value = drawy;
	}

	int i = (subMenu != -1) ? 4 : 3;

	if (vconsole->value) {
		SDL_Surface* console = getConsoleSurface();
		if (y < console->h) {
			if (b && click && MENU_TOP && MENU_RIGHT &&
				(x > screen->w - 14) && (y > console->h - 7)) {
				if (b == 1) vconsole->value = !vconsole->value;
				else if (b == 2) ossystem("telnet", "localhost 7777", false);
				else if (b == 4) ossystem("notepad", "console.txt", false);
			}
			return;
		}
	} else if (b && click && MENU_TOP && MENU_RIGHT &&
		(x > screen->w - 14) && (y < 7)) {
		if (b == 1) vconsole->value = !vconsole->value;
		else if (b == 2) ossystem("telnet", "localhost 7777", false);
		else if (b == 4) ossystem("notepad", "console.txt", false);
		return;
	}

	int inmenu = 0;
	for (; i >= 0; i--) {
		const int dx0 = menurects[i].x;
		const int dy0 = menurects[i].y;
		if (!menusurfaces[i] || x <= dx0 || y <= dy0 || x >= dx0 + menurects[i].w || y >= dy0 + menurects[i].h)
			continue;

		inmenu = 1;

		if ((i == MENU_BAR_LEFT) || (i == MENU_BAR_RIGHT) || ((i == 4) && ((subMenu == MENU_BAR_TOP) || (subMenu == MENU_BAR_BOTTOM)))) {
			scroll[i] = y - menurects[i].y;
			redrawmenus = true;
		} else {
			scroll[i] = x - menurects[i].x;
			redrawmenus = true;
		}

		const int dx = dx0 - realscrollx[i];
		const int dy = dy0 - realscrolly[i];

		for (auto it = menubuttons[i].begin(); it != menubuttons[i].end(); ++it) {
			const char* itype = (*it)->icon->type;
			if (itype[0] == 'S' && (!strcmp(itype, "SEPERATOR") || !strcmp(itype, "SEPARATOR")))
				continue;

			SDL_Surface* icon = (*it)->icon->getpic();
			if (!icon) continue;

			const bool hit = ((*it)->left + dx < x) && ((*it)->left + icon->w + dx > x) && ((*it)->top + dy < y) && ((*it)->top + icon->h + dy > y);

			if (!hit) {
				SDL_FreeSurface(icon);
				continue;
			}

			if (!nomousedownbutton || ((*it)->id == nomousedownbutton))
				mouseover = (*it)->tiptext;

			if ((click || b) && (*it)->click != nullptr && (!nomousedownbutton || ((*it)->id == nomousedownbutton))) {

				if (!(*it)->mode) nomousedown = true;
				else nomousedownbutton = (*it)->id;

				addparams((*it)->params);
				reinterpret_cast<Trigger*>((*it)->click)->exec(x - dx - (*it)->left - 1, y - dy - (*it)->top - 1, b, click);
				removeparams(false);

				if (subMenu == -2) {
					static int oldi = 0;
					if (menubuttons[4].empty()) {
						subMenu = -1;
						SDL_FreeSurface(icon);
						return;
					}
					if (i == 4) subMenu = oldi;
					else subMenu = i;

					int oldw = 0, oldh = 0;
					if (menusurfaces[4] != nullptr) {
						oldw = menusurfaces[4]->w;
						oldh = menusurfaces[4]->h;
						SDL_FreeSurface(menusurfaces[4]);
					}

					const bool leftOrRight = (subMenu == MENU_BAR_LEFT) ||
						(subMenu == MENU_BAR_RIGHT);
					if (subMenuAlign) {
						menusurfaces[4] = leftOrRight
							? createmenu(&menubuttons[4], subMenuAlign, screen->w - MENU_RIGHT - MENU_LEFT, 1000000, true)
							: createmenu(&menubuttons[4], subMenuAlign, 1000000, screen->h - MENU_TOP - MENU_BOTTOM, true);
					} else {
						menusurfaces[4] = leftOrRight
							? createmenu(&menubuttons[4], MENU_ALIGN_H, screen->w - MENU_RIGHT - MENU_LEFT, 1000000, true)
							: createmenu(&menubuttons[4], MENU_ALIGN_V, 1000000, screen->h - MENU_TOP - MENU_BOTTOM, true);
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
						} else if (oldi == MENU_BAR_LEFT) {
							menurects[4].y -= (menusurfaces[4]->h - oldh) / 2;
							menurects[4].x = MENU_LEFT;
						} else if (oldi == MENU_BAR_RIGHT) {
							menurects[4].y -= (menusurfaces[4]->h - oldh) / 2;
							menurects[4].x = screen->w - MENU_RIGHT - menusurfaces[4]->w + 1;
						} else if (oldi == MENU_BAR_BOTTOM) {
							menurects[4].y = screen->h - MENU_BOTTOM - menusurfaces[4]->h + 1;
							menurects[4].x -= (menusurfaces[4]->w - oldw) / 2;
						}
					}
					if (i != 4) oldi = i;

				} else if (subMenu == -4) {
					subMenu = 4;
				} else if ((subMenu != -1) && (subMenuStay == 0)) {
					subMenu = -1;
					redrawmenu(3);
				}
			}

			SDL_FreeSurface(icon);
			return;
		}
	}

	if ((inmenu == 0) &&
		(x > MENU_LEFT) && (x < screen->w - MENU_RIGHT) &&
		(y > MENU_TOP) && (y < screen->h - MENU_BOTTOM) &&
		(nomousedownbutton <= 1)) {

		findTrigger("MOUSEMOVE", 0)->exec(drawx, drawy, b, false);

		if (b) {
			if (click) findTrigger("MOUSEDOWN", 0)->exec(drawx, drawy, b, true);
			findTrigger("DRAG", 0)->exec(drawx, drawy, b, click);

			if (!click && solid->value) {
				const int ddx = drawx - lastx;
				const int ddy = drawy - lasty;
				const int dist = static_cast<int>(std::sqrt(static_cast<float>(ddx * ddx + ddy * ddy))) + 1;
				for (int j = 0; j < dist; j++) {
					findTrigger("DRAW", 0)->exec(lastx + ddx * j / dist, lasty + ddy * j / dist, b, click);
					click = false;
				}
			} else findTrigger("DRAW", 0)->exec(drawx, drawy, b, click);
			nomousedownbutton = 1;
		}
	}

	lastx = drawx;
	lasty = drawy;

	if (b && (subMenu != -1)) {
		if (subMenu == -4) subMenu = 4;
		else if (subMenuStay == 0) subMenu = -1;
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
		menusurfaces[4] = createmenu(&(menubuttons[4]), align, 1000000, lastscreen->h, true);
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

SDL_Surface* createmenu(std::list<Button*>* buttons, int align, int width, int height, bool crop) {
	constexpr int MAXLINES = 32;
	int line = 0;

	int seperators[MAXLINES] = {};
	int minus[MAXLINES] = {};
	int w[MAXLINES] = {};
	int h[MAXLINES] = {};

	for (auto* btn : *buttons) {
		if (!btn || !btn->icon) continue;

		const char* itype = btn->icon->type;

		if (strcmp(itype, "NEWLINE") == 0 && line < MAXLINES - 1) {
			if (align == MENU_ALIGN_V) while (line > 0 && h[line] == 0) line--;
			if (align == MENU_ALIGN_H) while (line > 0 && w[line] == 0) line--;

			int i = 0;
			getVar(btn->icon->text, &i);
			if (i != 0) {
				if (align == MENU_ALIGN_H) h[line] = i;
				if (align == MENU_ALIGN_V) w[line] = i;
			}
			line++;

		} else if (strcmp(itype, "SEPERATOR") == 0 ||
			strcmp(itype, "SEPARATOR") == 0) {
			int i = 0;
			getVar(btn->icon->text, &i);
			if (i > 0) {
				if (align == MENU_ALIGN_H) w[line] += i;
				if (align == MENU_ALIGN_V) h[line] += i;
			} else if (i == 0) {
				seperators[line]++;
			} else {
				minus[line] -= i;
			}

		} else {
			SDL_Surface* icon = btn->icon->getpic();
			if (!icon) continue;

			if (align == MENU_ALIGN_H) {
				if (h[line] < icon->h) h[line] = icon->h;
				if (minus[line] <= 0) {
					w[line] += icon->w;
				} else if (minus[line] >= icon->w) {
					minus[line] -= icon->w;
				} else {
					w[line] += icon->w - minus[line];
					minus[line] = 0;
				}
			}
			if (align == MENU_ALIGN_V) {
				if (w[line] < icon->w) w[line] = icon->w;
				if (minus[line] <= 0) {
					h[line] += icon->h;
				} else if (minus[line] >= icon->h) {
					minus[line] -= icon->h;
				} else {
					h[line] += icon->h - minus[line];
					minus[line] = 0;
				}
			}
			SDL_FreeSurface(icon);
		}
	}

	if (align == MENU_ALIGN_V) while (line > 0 && h[line] == 0) line--;
	if (align == MENU_ALIGN_H) while (line > 0 && w[line] == 0) line--;

	if (h[line] == 0 || w[line] == 0)
		return nullptr;

	int maxwidth = 0;
	int maxheight = 0;

	if (align == MENU_ALIGN_H) {
		for (int i = 0; i < MAXLINES; i++) {
			if (w[i] > maxwidth) maxwidth = w[i];
			maxheight += h[i];
		}
	} else {
		for (int i = 0; i < MAXLINES; i++) {
			maxwidth += w[i];
			if (h[i] > maxheight) maxheight = h[i];
		}
	}

	if (width == 0) width = maxwidth;
	if (height == 0) height = maxheight;

	if (align == MENU_ALIGN_V && width < w[line]) maxwidth = width;
	if (align == MENU_ALIGN_H && height < h[line]) maxheight = height;

	SDL_Surface* s;
	if (crop) {
		s = SDL_CreateRGBSurface(SDL_SWSURFACE, maxwidth, maxheight, 16, 0, 0, 0, 0);
	} else {
		if (width < maxwidth)  width = maxwidth;
		if (height < maxheight) height = maxheight;
		s = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 16, 0, 0, 0, 0);
	}
	if (!s) return nullptr;
	SDL_FillRect(s, nullptr, background);

	SDL_Rect dstrect = { 0, 0, 0, 0 };
	line = 0;
	int leftoffset = 0;

	for (auto* btn : *buttons) {
		if (!btn || !btn->icon) continue;

		const char* itype = btn->icon->type;

		if (strcmp(itype, "NEWLINE") == 0 && line < MAXLINES - 1) {
			if (align == MENU_ALIGN_H) { dstrect.x = 0; dstrect.y += h[line]; }
			if (align == MENU_ALIGN_V) { leftoffset += w[line]; dstrect.y = 0; }
			line++;

		} else if (strcmp(itype, "SEPERATOR") == 0 ||
			strcmp(itype, "SEPARATOR") == 0) {
			int i = 0;
			getVar(btn->icon->text, &i);
			if (i == 0 && seperators[line] > 0) {
				const int total = crop ? (align == MENU_ALIGN_H ? maxwidth : maxheight) : (align == MENU_ALIGN_H ? width : height);
				const int used = (align == MENU_ALIGN_H) ? w[line] : h[line];
				i = (total - used) / seperators[line];
				if (i < 0) i = 0;
			}
			if (align == MENU_ALIGN_H) dstrect.x += static_cast<Sint16>(i);
			if (align == MENU_ALIGN_V) dstrect.y += static_cast<Sint16>(i);

		} else {
			SDL_Surface* tmpsurface = btn->icon->getpic();
			if (!tmpsurface) continue;

			if (align == MENU_ALIGN_V)
				dstrect.x = static_cast<Sint16>(leftoffset + (w[line] - tmpsurface->w) / 2);

			SDL_BlitSurface(tmpsurface, nullptr, s, &dstrect);

			if (btn->border) SDL_DrawRect16(s, dstrect.x, dstrect.y, tmpsurface->w - 1, tmpsurface->h - 1, SDL_MapRGB(s->format, btn->r, btn->g, btn->b));

			btn->top = dstrect.y;
			btn->left = dstrect.x;

			if (align == MENU_ALIGN_H) dstrect.x += static_cast<Sint16>(tmpsurface->w);
			if (align == MENU_ALIGN_V) dstrect.y += static_cast<Sint16>(tmpsurface->h);

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