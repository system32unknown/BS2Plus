#include "sdlbasics.h"
#include <stack>
#include <iostream>
#include "elements.h"

static constexpr double BF_PI = 3.141592654;
SDL_Surface** stamps = 0;

void SDL_DrawSavePoint16(SDL_Surface* screen, int x, int y, Uint16 color) {
	if (x >= 0 && x < screen->w && y >= 0 && y < screen->h)
		*(static_cast<Uint16*>(screen->pixels) + y * screen->pitch / 2 + x) = color;
}

void SDL_DrawLine16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color) {
	const int length = static_cast<int>(sqrt(static_cast<float>(dx * dx + dy * dy)));
	if (length == 0) {
		SDL_DrawSavePoint16(screen, x, y, color);
		return;
	}
	const int w = screen->w;
	const int h = screen->h;
	const int pitch = screen->pitch / 2;
	Uint16* const pixels = static_cast<Uint16*>(screen->pixels);
	for (int i = 0; i <= length; i++) {
		const int tx = x + dx * i / length;
		const int ty = y + dy * i / length;
		if (tx >= 0 && tx < w && ty >= 0 && ty < h)
			*(pixels + ty * pitch + tx) = color;
	}
}

void SDL_ReplaceLine16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color, Sint32 replace) {
	const int length = static_cast<int>(sqrt(static_cast<float>(dx * dx + dy * dy)));
	if (length == 0) {
		SDL_DrawSavePoint16(screen, x, y, color);
		return;
	}

	const int w = screen->w;
	const int h = screen->h;
	const int pitch = screen->pitch / 2;
	Uint16* const pixels = static_cast<Uint16*>(screen->pixels);
	const Element* const e = (replace < 0) ? getElement(0) : nullptr;

	for (int i = 0; i <= length; i++) {
		const int tx = x + dx * i / length;
		const int ty = y + dy * i / length;
		if (tx < 0 || tx >= w || ty < 0 || ty >= h) continue;
		Uint16& px = *(pixels + ty * pitch + tx);
		if (replace >= 0 && px == static_cast<Uint16>(replace)) px = color;
		else if (replace == -1 && e[px].weight) px = color;
		else if (replace == -2 && !e[px].weight) px = color;
	}
}

void SDL_DrawRect16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color) {
	if (dx < 0) { dx = -dx; x -= dx; }
	if (dy < 0) { dy = -dy; y -= dy; }

	const int x2 = x + dx;
	const int y2 = y + dy;

	if (x2 < 0 || x >= screen->w) return;
	if (y2 < 0 || y >= screen->h) return;

	const int fromx = (x < 0) ? 0 : x;
	const int fromy = (y < 0) ? 0 : y;
	const int tox = (x2 >= screen->w) ? screen->w - 1 : x2;
	const int toy = (y2 >= screen->h) ? screen->h - 1 : y2;
	const int pitch = screen->pitch / 2;

	Uint16* pixels = static_cast<Uint16*>(screen->pixels);

	if (x >= 0 && x < screen->w)
		for (int i = fromy; i <= toy; i++)
			*(pixels + i * pitch + x) = color;

	if (x2 > 0 && x2 < screen->w)
		for (int i = fromy; i <= toy; i++)
			*(pixels + i * pitch + x2) = color;

	if (y >= 0 && y < screen->h)
		for (int i = fromx; i < tox; i++)
			*(pixels + y * pitch + i) = color;

	if (y2 > 0 && y2 < screen->h)
		for (int i = fromx; i < tox; i++)
			*(pixels + y2 * pitch + i) = color;
}

void SDL_DrawFilledRect16(SDL_Surface* screen, int x, int y, int dx, int dy, Uint16 color) {
	if (dx == 0 || dy == 0) return;

	if (x >= screen->w || y >= screen->h) return;
	if (x + dx <= 0 || y + dy <= 0) return;

	if (x < 0) { dx += x; x = 0; }
	if (y < 0) { dy += y; y = 0; }
	if (x + dx > screen->w) dx = screen->w - x;
	if (y + dy > screen->h) dy = screen->h - y;

	if (dx <= 0 || dy <= 0) return;

	SDL_Rect rect = { static_cast<Sint16>(x),  static_cast<Sint16>(y),
					  static_cast<Uint16>(dx), static_cast<Uint16>(dy) };
	SDL_FillRect(screen, &rect, color);
}

void SDL_DrawCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, Uint16 color) {
	const int max = (int)(sqrt((double)rx)) * 4 + 1;
	int lastx = 0, lasty = ry;
	for (int i = 0; i <= max; i++) {
		const double angle = i * BF_PI / 2.0 / max;
		const int nextx = (int)(sin(angle) * rx);
		const int nexty = (int)(cos(angle) * ry);
		SDL_DrawLine16(screen, x + lastx, y + lasty, nextx - lastx, nexty - lasty, color);
		SDL_DrawLine16(screen, x + lastx, y - lasty, nextx - lastx, -nexty + lasty, color);
		SDL_DrawLine16(screen, x - lastx, y + lasty, -nextx + lastx, nexty - lasty, color);
		SDL_DrawLine16(screen, x - lastx, y - lasty, -nextx + lastx, -nexty + lasty, color);
		lastx = nextx;
		lasty = nexty;
	}
}

void SDL_DrawFilledCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, Uint16 color) {
	if (rx == 0 && ry == 0) {
		if (x >= 0 && x < screen->w && y >= 0 && y < screen->h)
			*(static_cast<Uint16*>(screen->pixels) + y * screen->pitch / 2 + x) = color;
		return;
	}

	const int pitch = screen->pitch / 2;
	const int w = screen->w;
	const int sr = ry * ry;
	const float factor = ry ? (float)rx / (float)ry : 1.0f;
	int y2 = (y - ry < 0) ? 0 : y - ry;
	const int end = (y + ry > screen->h - 1) ? screen->h - 1 : y + ry;

	for (; y2 <= end; y2++) {
		const int dx = (int)(sqrt((float)(sr - (y2 - y) * (y2 - y))) * factor);
		int x2 = (x - dx < 0) ? 0 : x - dx;
		Uint16* row = static_cast<Uint16*>(screen->pixels) + y2 * pitch;
		for (; x2 <= x + dx && x2 < w; x2++) *(row + x2) = color;
	}
}

void SDL_DrawRandomFilledCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, int rate, Uint16 color) {
	const int pitch = screen->pitch / 2;
	const int w = screen->w;
	const int sr = ry * ry;
	const float factor = ry ? (float)rx / (float)ry : 1.0f;
	int y2 = (y - ry < 0) ? 0 : y - ry;
	const int end = (y + ry > screen->h - 1) ? screen->h - 1 : y + ry;

	for (; y2 <= end; y2++) {
		const int dx = (int)(sqrt((float)(sr - (y2 - y) * (y2 - y))) * factor);
		int x2 = (x - dx < 0) ? 0 : x - dx;
		Uint16* row = static_cast<Uint16*>(screen->pixels) + y2 * pitch;
		for (; x2 <= x + dx && x2 < w; x2++)
			if (RANDOMNUMBER < rate) *(row + x2) = color;
	}
}

void SDL_ReplaceFilledCircle16(SDL_Surface* screen, int x, int y, int rx, int ry, Uint16 color, Sint32 replace) {
	const int w = screen->w;
	const int pitch = screen->pitch / 2;
	const int sr = ry * ry;
	const float factor = ry ? (float)rx / (float)ry : 1.0f;
	const Element* e = (replace < 0) ? getElement(0) : nullptr;
	int y2 = (y - ry < 0) ? 0 : y - ry;
	const int end = (y + ry > screen->h - 1) ? screen->h - 1 : y + ry;

	for (; y2 <= end; y2++) {
		const int dx = (int)(sqrt((float)(sr - (y2 - y) * (y2 - y))) * factor);
		int x2 = (x - dx < 0) ? 0 : x - dx;
		Uint16* row = static_cast<Uint16*>(screen->pixels) + y2 * pitch;
		if (replace >= 0) {
			for (; x2 <= x + dx && x2 < w; x2++)
				if (*(row + x2) == static_cast<Uint16>(replace)) *(row + x2) = color;
		} else if (replace == -1) {
			for (; x2 <= x + dx && x2 < w; x2++)
				if (e[*(row + x2)].weight) *(row + x2) = color;
		} else if (replace == -2) {
			for (; x2 <= x + dx && x2 < w; x2++)
				if (!e[*(row + x2)].weight) *(row + x2) = color;
		}
	}
}

void SDL_CopyRect16(SDL_Surface* screen, int x, int y, int dx, int dy, int tox, int toy) {
	SDL_Rect srcrect = { static_cast<Sint16>(x), static_cast<Sint16>(y),
						 static_cast<Uint16>(dx), static_cast<Uint16>(dy) };
	SDL_Rect dstrect = { static_cast<Sint16>(tox), static_cast<Sint16>(toy), 0, 0 };

	SDL_Surface* tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, dx, dy, 16, 0, 0, 0, 0);
	if (!tmp) return;
	SDL_BlitSurface(screen, &srcrect, tmp, nullptr);
	SDL_BlitSurface(tmp, nullptr, screen, &dstrect);
	SDL_FreeSurface(tmp);
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
	int xx_to = static_cast<int>(destsurf->clip_rect.w);
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
	if (width) *width = destsurf->w;

	SDL_FreeSurface(destsurf);
#endif
}

void SDL_Fill16(SDL_Surface* screen, int x, int y, Uint16 color) {
	if (x < 0 || x >= screen->w || y < 0 || y >= screen->h) return;

	const int p = screen->pitch / 2;
	Uint16* const base = static_cast<Uint16*>(screen->pixels);
	const Uint16  replace = *(base + y * p + x);

	if (replace == color) return;

	const int width = screen->w - 1;
	const int height = screen->h - 1;

	std::stack<std::pair<int, int>> fillStack;
	fillStack.push({ x, y });

	while (!fillStack.empty()) {
		auto [tx, ty] = fillStack.top();
		fillStack.pop();

		Uint16* px = base + ty * p + tx;
		if (*px == color) continue;
		if (*px != replace) continue;
		*px = color;

		if (tx < width && *(px + 1) == replace) fillStack.push({ tx + 1, ty });
		if (tx > 0 && *(px - 1) == replace) fillStack.push({ tx - 1, ty });
		if (ty < height && *(px + p) == replace) fillStack.push({ tx, ty + 1 });
		if (ty > 0 && *(px - p) == replace) fillStack.push({ tx, ty - 1 });
	}
}

void SDL_RotateRect16(SDL_Surface* screen, int x, int y, int dx, int dy, int direction) {
	if (dx <= 0 || dy <= 0) return;

	const int absd = std::abs(direction);

	SDL_Rect rect;
	rect.x = static_cast<Sint16>(x);
	rect.y = static_cast<Sint16>(y);

	SDL_Surface* s2;
	if (absd < 4) {
		rect.w = static_cast<Uint16>(dy);
		rect.h = static_cast<Uint16>(dx);
		if (dx < dy) dy = dx; else dx = dy;
		s2 = SDL_CreateRGBSurface(SDL_SWSURFACE, dy, dx, 16, 0, 0, 0, 0);
	} else {
		rect.w = static_cast<Uint16>(dx);
		rect.h = static_cast<Uint16>(dy);
		s2 = SDL_CreateRGBSurface(SDL_SWSURFACE, dx, dy, 16, 0, 0, 0, 0);
	}

	SDL_Surface* s1 = SDL_CreateRGBSurface(SDL_SWSURFACE, dx, dy, 16, 0, 0, 0, 0);
	if (!s1 || !s2) {
		if (s1) SDL_FreeSurface(s1);
		if (s2) SDL_FreeSurface(s2);
		return;
	}

	SDL_BlitSurface(screen, &rect, s1, nullptr);

	const int p1 = s1->pitch / 2;
	const int p2 = s2->pitch / 2;
	Uint16* const px1 = static_cast<Uint16*>(s1->pixels);
	Uint16* const px2 = static_cast<Uint16*>(s2->pixels);

	auto src = [&](int col, int row) -> Uint16 { return *(px1 + row * p1 + col); };
	auto dst = [&](int col, int row) -> Uint16& { return *(px2 + row * p2 + col); };

	if (direction == -1)
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				*(px2 + x1 * p2 + (dy - y1 - 1)) = src(x1, y1);

	if (direction == 1)
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				*(px2 + (dx - x1 - 1) * p2 + y1) = src(x1, y1);

	if (absd == 2) {
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				*(px2 + (dx - x1 - 1) * p2 + y1) = src(x1, y1);
		std::swap(s1, s2);

		const int np1 = s1->pitch / 2;
		const int np2 = s2->pitch / 2;
		Uint16* ns1 = static_cast<Uint16*>(s1->pixels);
		Uint16* ns2 = static_cast<Uint16*>(s2->pixels);
		for (int x2 = 0; x2 < dx; x2++)
			for (int y1 = 0; y1 < dy; y1++)
				*(ns2 + (dx - x2 - 1) * np2 + y1) = *(ns1 + y1 * np1 + x2);
	}

	if (absd == 3) {
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				*(px2 + y1 * p2 + (dx - x1 - 1)) = src(x1, y1);
		std::swap(s1, s2);
		const int np1 = s1->pitch / 2;
		const int np2 = s2->pitch / 2;
		Uint16* ns1 = static_cast<Uint16*>(s1->pixels);
		Uint16* ns2 = static_cast<Uint16*>(s2->pixels);
		for (int x2 = 0; x2 < dx; x2++)
			for (int y1 = 0; y1 < dy; y1++)
				*(ns2 + (dy - y1 - 1) * np2 + x2) = *(ns1 + y1 * np1 + x2);
	}

	if (absd == 4)
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				dst(dx - x1 - 1, y1) = src(x1, y1);

	if (absd == 5)
		for (int x1 = 0; x1 < dx; x1++)
			for (int y1 = 0; y1 < dy; y1++)
				dst(x1, dy - y1 - 1) = src(x1, y1);

	SDL_BlitSurface(s2, nullptr, screen, &rect);
	SDL_FreeSurface(s1);
	SDL_FreeSurface(s2);
}

void SDL_CopyStamp16(SDL_Surface* screen, int x, int y, int dx, int dy, int stamp) {
	if (stamp < 0 || stamp >= MAX_STAMPS * 2) return;

	if (stamps[stamp]) {
		SDL_FreeSurface(stamps[stamp]);
		stamps[stamp] = nullptr;
	}

	SDL_Rect rect = { static_cast<Sint16>(x), static_cast<Sint16>(y),
					  static_cast<Uint16>(dx), static_cast<Uint16>(dy) };

	stamps[stamp] = SDL_CreateRGBSurface(SDL_SWSURFACE, dx, dy, 16, 0, 0, 0, 0);
	if (!stamps[stamp]) return;

	SDL_BlitSurface(screen, &rect, stamps[stamp], nullptr);
}

void SDL_PasteStamp16(SDL_Surface* screen, int x, int y, int dx, int dy, int stamp, Uint16 transparent) {
	if (!stamps) return;
	if (stamp < 0 || stamp >= MAX_STAMPS * 2) return;
	if (!stamps[stamp]) return;

	SDL_Rect rect = { static_cast<Sint16>(x), static_cast<Sint16>(y),
					  static_cast<Uint16>(dx), static_cast<Uint16>(dy) };

	if (y + dy >= screen->h)
		rect.h = static_cast<Uint16>(screen->h - y);

	SDL_Rect srcrect = { 0, 0, rect.w, rect.h };

	SDL_SetColorKey(stamps[stamp], SDL_SRCCOLORKEY, transparent);
	SDL_SetAlpha(stamps[stamp], SDL_RLEACCEL, 255);
	SDL_BlitSurface(stamps[stamp], &srcrect, screen, &rect);
}

void SDL_SwapPoints16(SDL_Surface* screen, int x, int y, int x2, int y2, bool sand) {
	if (x <= 0 || x >= screen->w || y <= 0 || y >= screen->h) return;
	if (x2 <= 0 || x2 >= screen->w || y2 <= 0 || y2 >= screen->h) return;

	const int pitch = screen->pitch / 2;
	Uint16* const pixels = static_cast<Uint16*>(screen->pixels);
	Uint16& pa = *(pixels + y * pitch + x);
	Uint16& pb = *(pixels + y2 * pitch + x2);

	if (sand && (pa == 1 || pb == 1)) return;

	const Uint16 t = pa;
	pa = pb;
	pb = t;
}