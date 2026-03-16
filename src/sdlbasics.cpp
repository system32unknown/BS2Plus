#include "sdlbasics.h"
#include <stack>
#include <iostream>
#include "elements.h"

SDL_Surface** stamps = 0;

void SDL_DrawPoint16(SDL_Surface* screen, int x, int y, Uint16 color) {
	*((Uint16*)screen->pixels + y * screen->pitch / 2 + x) = color;
}

void SDL_DrawSavePoint16(SDL_Surface* screen, int x, int y, Uint16 color) {
	if ((x >= 0) && (x < screen->w) && (y >= 0) && (y < screen->h))
		*((Uint16*)screen->pixels + y * screen->pitch / 2 + x) = color;
}

void SDL_DrawLine16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color) {
	int length;
	if ((length = (int)sqrt((float)dx * dx + dy * dy)) == 0) {
		if ((x >= 0) && (x < screen->w) && (y >= 0) && (y < screen->h))
			*((Uint16*)screen->pixels + y * screen->pitch / 2 + x) = color;
		return;
	}
	int tmpx, tmpy;
	int w = screen->w;
	int h = screen->h;
	for (int i = 0; i <= length; i++)
		if (((tmpx = x + dx * i / length) >= 0) && (tmpx < w) && ((tmpy = y + dy * i / length) >= 0) && (tmpy < h))
			*((Uint16*)screen->pixels + (tmpy)*screen->pitch / 2 + tmpx) = color;
}

void SDL_ReplaceLine16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color, Sint32 replace) {
	int length;
	if ((length = (int)sqrt((float)dx * dx + dy * dy)) == 0) {
		if ((x >= 0) && (x < screen->w) && (y >= 0) && (y < screen->h))
			*((Uint16*)screen->pixels + y * screen->pitch / 2 + x) = color;
		return;
	}
	int tmpx, tmpy;
	int w = screen->w;
	int h = screen->h;
	int pitch = screen->pitch / 2;
	Uint16* pixels = (Uint16*)screen->pixels;

	Element* e = 0;
	if (replace < 0)
		e = getElement(0);

	if (replace >= 0)
		for (int i = 0; i <= length; i++)
			if (((tmpx = x + dx * i / length) >= 0) && (tmpx < w) && ((tmpy = y + dy * i / length) >= 0) && (tmpy < h))
				if (*(pixels + (tmpy)*pitch + tmpx) == replace)
					*(pixels + (tmpy)*pitch + tmpx) = color;
	if (replace == -1)
		for (int i = 0; i <= length; i++)
			if (((tmpx = x + dx * i / length) >= 0) && (tmpx < w) && ((tmpy = y + dy * i / length) >= 0) && (tmpy < h))
				if ((e[*(pixels + (tmpy)*pitch + tmpx)].weight))
					*(pixels + (tmpy)*pitch + tmpx) = color;
	if (replace == -2)
		for (int i = 0; i <= length; i++)
			if (((tmpx = x + dx * i / length) >= 0) && (tmpx < w) && ((tmpy = y + dy * i / length) >= 0) && (tmpy < h))
				if (!(e[*(pixels + (tmpy)*pitch + tmpx)].weight))
					*(pixels + (tmpy)*pitch + tmpx) = color;
}

void SDL_DrawRect16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color) {
	int y2, x2;
	if (dx < 0) {
		dx = -dx;
		x = x - dx;
	}
	if (dy < 0) {
		dy = -dy;
		y = y - dy;
	}
	int fromx = x;
	if (x < 0)
		fromx = 0;
	if (x >= screen->w)
		return;
	int fromy = y;
	if (y < 0)
		fromy = 0;
	if (y >= screen->h)
		return;
	int tox = x2 = x + dx;
	if (x2 < 0)
		return;
	if (x2 >= screen->w)
		tox = screen->w - 1;
	int toy = y2 = y + dy;
	if (y2 < 0)
		return;
	if (y2 >= screen->h)
		toy = screen->h - 1;
	Uint16* tmp = (Uint16*)screen->pixels + x;
	if ((x >= 0) && screen->w > (x))
		for (int i = fromy; i <= toy; i++) {
			*(tmp + i * screen->pitch / 2) = color;
		}
	tmp = (Uint16*)screen->pixels + x2;
	if ((x2 > 0) && screen->w > (x2))
		for (int i = fromy; i <= toy; i++) {
			*(tmp + i * screen->pitch / 2) = color;
		}
	tmp = (Uint16*)screen->pixels + y * screen->pitch / 2;
	if ((y >= 0) && screen->h > (y))
		for (int i = fromx; i < tox; i++) {
			*(tmp + i) = color;
		}
	tmp = (Uint16*)screen->pixels + y2 * screen->pitch / 2;
	if ((y2 > 0) && screen->h > (y2))
		for (int i = fromx; i < tox; i++) {
			*(tmp + i) = color;
		}
}

void SDL_DrawFilledRect16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color) {
	if (!(dx * dy))
		return;
	SDL_Rect rect;

	if (y < 0) {
		dy += y;
		y = 0;
	}
	if (y >= screen->h)
		return;
	if (y + dy >= screen->h)
		dy = screen->h - y;
	if (dy <= 0)
		return;
	rect.x = x;
	rect.y = y;
	rect.w = dx;
	rect.h = dy;
	SDL_FillRect(screen, &rect, color);
}

void SDL_DrawCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, Uint16 color) {
	int max = (int)sqrt((float)rx) * 4 + 1;
	int lastx, lasty, nextx, nexty;
	lastx = 0;
	lasty = ry;
	for (int i = 0; i <= max; i++) {
		nextx = (int)(sin(i * 3.141592654 / 2 / max) * rx);
		nexty = (int)(cos(i * 3.141592654 / 2 / max) * ry);
		SDL_DrawLine16(screen, x + lastx, y + lasty, nextx - lastx, nexty - lasty, color);
		SDL_DrawLine16(screen, x + lastx, y - lasty, nextx - lastx, -nexty + lasty, color);
		SDL_DrawLine16(screen, x - lastx, y + lasty, -nextx + lastx, nexty - lasty, color);
		SDL_DrawLine16(screen, x - lastx, y - lasty, -nextx + lastx, -nexty + lasty, color);
		lastx = nextx;
		lasty = nexty;
	}
}

void SDL_DrawFilledCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, Uint16 color) {
	if ((rx == 0) && (ry == 0)) {
		if ((x >= 0) && (x < screen->w) && (y >= 0) && (y < screen->h))
			*((Uint16*)screen->pixels + y * screen->pitch / 2 + x) = color;
		return;
	}
	int y2 = y - ry;
	if (y2 < 0)
		y2 = 0;
	int w = screen->w;
	int dx, x2;
	int pitch = screen->pitch / 2;
	int end = y + ry;
	if (end > screen->h - 1)
		end = screen->h - 1;
	int sr = ry * ry;
	float factor = 1;
	if (ry != 0)
		factor = (float)rx / (float)ry;
	for (; y2 <= end; y2++) {
		dx = (int)(sqrt((float)((sr - (y2 - y) * (y2 - y)))) * factor);
		x2 = x - dx;
		if (x2 < 0)
			x2 = 0;
		for (; (x2 <= x + dx) && (x2 < w); x2++)
			*((Uint16*)screen->pixels + y2 * pitch + x2) = color;
	}
}

void SDL_DrawRandomFilledCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, int rate, Uint16 color) {
	int y2 = y - ry;
	if (y2 < 0)
		y2 = 0;
	int w = screen->w;
	int dx, x2;
	int pitch = screen->pitch / 2;
	int sr = ry * ry;
	int end = y + ry;
	if (end > screen->h - 1)
		end = screen->h - 1;
	float factor = 1;
	if (ry != 0)
		factor = (float)rx / (float)ry;
	for (; y2 <= end; y2++) {
		dx = (int)(sqrt((float)((sr - (y2 - y) * (y2 - y)))) * factor);
		x2 = x - dx;
		if (x2 < 0)
			x2 = 0;
		for (; (x2 <= x + dx) && (x2 < w); x2++)
			if (RANDOMNUMBER < rate)
				*((Uint16*)screen->pixels + y2 * pitch + x2) = color;
	}
}

void SDL_ReplaceFilledCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, Uint16 color, Sint32 replace) {
	int y2 = y - ry;
	if (y2 < 0)
		y2 = 0;
	int w = screen->w;
	int dx, x2;
	int end = y + ry;
	if (end > screen->h - 1)
		end = screen->h - 1;
	int sr = ry * ry;
	float factor = 1;
	if (ry != 0)
		factor = (float)rx / (float)ry;
	Element* e = 0;
	if (replace < 0)
		e = getElement(0);
	for (; y2 <= end; y2++) {
		dx = (int)(sqrt((float)((sr - (y2 - y) * (y2 - y)))) * factor);
		x2 = x - dx;
		if (x2 < 0)
			x2 = 0;
		if (replace >= 0) {
			for (; (x2 <= x + dx) && (x2 < w); x2++)
				if (*((Uint16*)screen->pixels + y2 * screen->pitch / 2 + x2) == replace)
					*((Uint16*)screen->pixels + y2 * screen->pitch / 2 + x2) = color;
		} else {
			if (replace == -1)
				for (; (x2 <= x + dx) && (x2 < w); x2++)
					if ((e[*((Uint16*)screen->pixels + y2 * screen->pitch / 2 + x2)].weight))
						*((Uint16*)screen->pixels + y2 * screen->pitch / 2 + x2) = color;
			if (replace == -2)
				for (; (x2 <= x + dx) && (x2 < w); x2++)
					if (!(e[*((Uint16*)screen->pixels + y2 * screen->pitch / 2 + x2)].weight))
						*((Uint16*)screen->pixels + y2 * screen->pitch / 2 + x2) = color;
		}
	}
}

void SDL_CopyRect16(SDL_Surface* screen, int x, int y, int dx, int dy, int tox, int toy) {
	SDL_Rect srcrect, dstrect;
	srcrect.x = x;
	srcrect.y = y;
	srcrect.w = dx;
	srcrect.h = dy;
	SDL_Surface* tmpsurface = SDL_CreateRGBSurface(SDL_SWSURFACE, dx, dy, 16, 0, 0, 0, 0);
	SDL_BlitSurface(screen, &srcrect, tmpsurface, NULL);
	dstrect.x = tox;
	dstrect.y = toy;
	SDL_BlitSurface(tmpsurface, 0, screen, &dstrect);
	SDL_FreeSurface(tmpsurface);
	return;
}

void SDL_DrawText16(SDL_Surface* screen, void* f, char* t, int x, int y, Uint16 color, int* width, int* height, int align) {
    if (t == nullptr || t[0] == '\0') return;

#ifdef COMPILER_SDL_TTF
    TTF_Font* font = reinterpret_cast<TTF_Font*>(f);

    static const SDL_Color fontcolor = { 255, 255, 255, 255 };
    SDL_Surface* destsurf = TTF_RenderText_Solid(font, t, fontcolor);
    if (!destsurf) return;

    if (align == 1) x -= destsurf->w / 2;

    int xx = (x < 0) ? -x : 0;
    int xx_to  = static_cast<int>(destsurf->clip_rect.w);
    if (xx_to + x > screen->w) xx_to = screen->w - x;

    const int dy = destsurf->clip_rect.h - y + TTF_FontDescent(font);

	const int yy_start = std::max(0, dy);
	const int yy_end = std::min(static_cast<int>(destsurf->clip_rect.h), screen->h + dy);

    const int pitch = destsurf->pitch;

    for (int xx_cur = xx; xx_cur < xx_to; xx_cur++) {
        const Uint8* col_base = static_cast<const Uint8*>(destsurf->pixels) + xx_cur;
        for (int yy = yy_start; yy < yy_end; yy++) {
            if (*(col_base + yy * pitch)) {
                *((Uint16*)screen->pixels + (yy - dy) * screen->pitch / 2 + x + xx_cur) = color;
            }
        }
    }

    if (height) *height = destsurf->h;
    if (width) *width  = destsurf->w;

    SDL_FreeSurface(destsurf);
#endif
}

void SDL_Fill16(SDL_Surface* screen, int x, int y, Uint16 color) {
	int replace = *((Uint16*)screen->pixels + y * screen->pitch / 2 + x);
	std::stack<int> fillStackx;
	std::stack<int> fillStacky;
	fillStackx.push(x);
	fillStacky.push(y);
	int tx, ty;
	int p = screen->pitch / 2;
	int width = screen->w - 1;
	int height = screen->h - 1;
	while (!fillStackx.empty()) {
		tx = fillStackx.top();
		ty = fillStacky.top();
		fillStackx.pop();
		fillStacky.pop();
		if (*((Uint16*)screen->pixels + ty * p + tx) != color) {
			if ((tx < width) && (*((Uint16*)screen->pixels + ty * p + tx + 1) == replace)) {
				fillStackx.push(tx + 1);
				fillStacky.push(ty);
			}
			if ((tx > 0) && (*((Uint16*)screen->pixels + ty * p + tx - 1) == replace)) {
				fillStackx.push(tx - 1);
				fillStacky.push(ty);
			}
			if ((ty < height) && (*((Uint16*)screen->pixels + (ty + 1) * p + tx) == replace)) {
				fillStackx.push(tx);
				fillStacky.push(ty + 1);
			}
			if ((ty > 0) && (*((Uint16*)screen->pixels + (ty - 1) * p + tx) == replace)) {
				fillStackx.push(tx);
				fillStacky.push(ty - 1);
			}
			*((Uint16*)screen->pixels + ty * p + tx) = color;
		}
	}
}

void SDL_RotateRect16(SDL_Surface* screen, int x, int y, int dx, int dy, int direction) {
	if ((dx <= 0) || (dy <= 0))
		return;
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	SDL_Surface* s2;
	if (abs(direction) < 4) {
		rect.w = dy;
		rect.h = dx;
		if (dx < dy)
			dy = dx;
		else
			dx = dy;
		s2 = SDL_CreateRGBSurface(SDL_SWSURFACE, dy, dx, 16, 0, 0, 0, 0);
	} else {
		rect.w = dx;
		rect.h = dy;
		s2 = SDL_CreateRGBSurface(SDL_SWSURFACE, dx, dy, 16, 0, 0, 0, 0);
	}
	SDL_Surface* s1 = SDL_CreateRGBSurface(SDL_SWSURFACE, dx, dy, 16, 0, 0, 0, 0);
	SDL_BlitSurface(screen, &rect, s1, NULL);
	int p1 = s1->pitch / 2;
	int p2 = s2->pitch / 2;
	if (direction == -1)
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				*((Uint16*)s2->pixels + x1 * p2 + (dy - y1 - 1)) = *((Uint16*)s1->pixels + y1 * p1 + x1);
	if (direction == 1)
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				*((Uint16*)s2->pixels + (dx - x1 - 1) * p2 + y1) = *((Uint16*)s1->pixels + y1 * p1 + x1);
	if (abs(direction) == 2) {
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				*((Uint16*)s2->pixels + (dx - x1 - 1) * p2 + y1) = *((Uint16*)s1->pixels + y1 * p1 + x1);
		SDL_Surface* s3 = s2;
		s2 = s1;
		s1 = s3;
		for (int x2 = 0; x2 < dx; x2++)
			for (int y1 = 0; y1 < dy; y1++)
				*((Uint16*)s2->pixels + (dx - x2 - 1) * p2 + y1) = *((Uint16*)s1->pixels + y1 * p1 + x2);
	}
	if (abs(direction) == 3) {
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				*((Uint16*)s2->pixels + y1 * p2 + (dx - x1 - 1)) = *((Uint16*)s1->pixels + y1 * p1 + x1);
		SDL_Surface* s3 = s2;
		s2 = s1;
		s1 = s3;
		for (int x2 = 0; x2 < dx; x2++)
			for (int y1 = 0; y1 < dy; y1++)
				*((Uint16*)s2->pixels + (dy - y1 - 1) * p2 + x2) = *((Uint16*)s1->pixels + y1 * p1 + x2);
	}
	if (abs(direction) == 4)
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				*((Uint16*)s2->pixels + y1 * p2 + (dx - x1 - 1)) = *((Uint16*)s1->pixels + y1 * p1 + x1);
	if (abs(direction) == 5)
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				*((Uint16*)s2->pixels + (dy - y1 - 1) * p2 + x1) = *((Uint16*)s1->pixels + y1 * p1 + x1);
	SDL_BlitSurface(s2, NULL, screen, &rect);
	SDL_FreeSurface(s1);
	SDL_FreeSurface(s2);
	return;
}

void SDL_CopyStamp16(SDL_Surface* screen, int x, int y, int dx, int dy, int stamp) {
	if (stamp >= MAX_STAMPS * 2)
		return;
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = dx;
	rect.h = dy;
	if (stamps[stamp])
		SDL_FreeSurface(stamps[stamp]);
	stamps[stamp] = SDL_CreateRGBSurface(SDL_SWSURFACE, dx, dy, 16, 0, 0, 0, 0);
	SDL_BlitSurface(screen, &rect, stamps[stamp], NULL);
	return;
}

void SDL_PasteStamp16(SDL_Surface* screen, int x, int y, int dx, int dy, int stamp, Uint16 transparent) {
	if (!stamps) return;
	if (stamp >= MAX_STAMPS * 2) return;
	if (!stamps[stamp]) return;
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = dy;
	rect.h = dx;
	if ((y + dy) >= screen->h)
		rect.h = screen->h - y;
	SDL_Rect srcrect;
	srcrect.x = 0;
	srcrect.y = 0;
	srcrect.w = rect.w;
	srcrect.h = rect.h;
	SDL_SetColorKey(stamps[stamp], SDL_SRCCOLORKEY, transparent);
	SDL_SetAlpha(stamps[stamp], SDL_RLEACCEL, 255);
	SDL_BlitSurface(stamps[stamp], &srcrect, screen, &rect);
	return;
}

void SDL_SwapPoints16(SDL_Surface* screen, int x, int y, int x2, int y2, bool sand) {
	if ((x > 0) && (x < screen->w) && (y > 0) && (y < screen->h))
		if ((x2 > 0) && (x2 < screen->w) && (y2 > 0) && (y2 < screen->h)) {
			Uint16 t = *((Uint16*)screen->pixels + y * screen->pitch / 2 + x);
			if (sand && ((t == 1) || (*((Uint16*)screen->pixels + y2 * screen->pitch / 2 + x2) == 1)))
				return;
			*((Uint16*)screen->pixels + y * screen->pitch / 2 + x) = *((Uint16*)screen->pixels + y2 * screen->pitch / 2 + x2);
			*((Uint16*)screen->pixels + y2 * screen->pitch / 2 + x2) = t;
		}
}