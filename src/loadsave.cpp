#include "loadsave.h"
#include "elements.h"
#include "config.h"
#include <fstream>
#include "win.h"

#define MAX_QUICKSAVES 10001

#ifdef USE_PNG
#include "png.h"
#endif

#include <stdlib.h>
SDL_Surface *quicksaves[MAX_QUICKSAVES];
int firstquick = 0;

void quicksave(int slot)
{
	if (firstquick == 0)
		for (int i = 0; i < MAX_QUICKSAVES; i++)
			quicksaves[i] = 0;
	firstquick = 1;
	if ((slot >= MAX_QUICKSAVES) || (slot < 0))
		return;
	SDL_Surface *screen = getRealSandSurface();
	if (quicksaves[slot])
		SDL_FreeSurface(quicksaves[slot]);
	quicksaves[slot] = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 16, 0, 0, 0, 0);
	SDL_BlitSurface(screen, 0, quicksaves[slot], 0);
}

void quickload(int slot)
{
	if (firstquick == 0)
		for (int i = 0; i < MAX_QUICKSAVES; i++)
			quicksaves[i] = 0;
	firstquick = 1;
	if ((slot >= MAX_QUICKSAVES) || (slot < 0))
		return;
	if (!quicksaves[slot])
		return;
	SDL_Surface *screen = getRealSandSurface();
	if ((quicksaves[slot]->w != screen->w) || (quicksaves[slot]->h != screen->h))
		resize(quicksaves[slot]->w, quicksaves[slot]->h);
	screen = getRealSandSurface();
	SDL_BlitSurface(quicksaves[slot], 0, screen, 0);
	recalccolors();
	recalcused();
}

int save(SDL_Surface *screen, char *filename)
{
	Element *elements = getElement(0);
	int elementsmaxnew = getelementsmax();
	if ((strlen(filename) > 4) && ((!strcmp(filename + strlen(filename) - 4, ".bs2")) || (!strcmp(filename + strlen(filename) - 4, ".BS2"))))
	{
		char tmp[255];
		FILE *fp = fopen(checkfilename(filename), "wb");
		sprintf(tmp, "RESIZE %i %i\n", screen->w - 2, screen->h - 2);
		fputs(tmp, fp);
		sprintf(tmp, "DRAW 0 FILLEDRECT 0 0 %i %i\n", screen->w, screen->h);
		fputs(tmp, fp);
		for (int i = 2; i < elementsmaxnew; i++)
		{
			if (elements[i].name)
			{
				bool first = true;
				for (int y = 0; y < screen->h; y++)
					for (int x = 0; x < screen->w; x++)
						if (*((Uint16 *)screen->pixels + y * screen->pitch / 2 + x) == i)
						{
							if (first)
							{
								fputs("DRAW \"ELEMENT:", fp);
								fputs(elements[i].name, fp);
								fputs("\" POINTS 0 0 ", fp);
								first = false;
							}
							sprintf(tmp, "%i %i ", x, y);
							fputs(tmp, fp);
						}
				if (!first)
					fputs("\n", fp);
			}
		}
		fclose(fp);
	}
	if ((strlen(filename) > 4) && ((!strcmp(filename + strlen(filename) - 4, ".bmp")) || (!strcmp(filename + strlen(filename) - 4, ".BMP"))))
	{
		SDL_Surface *p = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 32, 0, 0, 0, 0);
		Uint32 *color = new Uint32[elementsmaxnew];
		for (int i = elementsmaxnew - 1; i >= 0; i--)
			color[i] = SDL_MapRGB(p->format, elements[i].r, elements[i].g, elements[i].b);
		for (int y = 0; y < screen->h; y++)
			for (int x = 0; x < screen->w; x++)
				*((Uint32 *)p->pixels + y * p->pitch / 4 + x) = color[*((Uint16 *)screen->pixels + y * screen->pitch / 2 + x)];
		SDL_SaveBMP(p, checkfilename(filename));
		delete (color);
	}
	if ((strlen(filename) > 4) && ((!strcmp(filename + strlen(filename) - 4, ".png")) || (!strcmp(filename + strlen(filename) - 4, ".png"))))
	{
#ifdef USE_PNG
		FILE *fp = fopen(checkfilename(filename), "wb");

		png_structp png_ptr;
		png_infop info_ptr;
		png_bytep *row_pointers;
		row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * screen->h);

		for (int y = 0; y < screen->h; y++)
		{
			row_pointers[y] = (png_byte *)malloc(screen->w * 6);
			for (int x = 0; x < screen->w; x++)
			{
				Uint16 t = *((Uint16 *)screen->pixels + y * screen->pitch / 2 + x);
				row_pointers[y][x * 6] = elements[t].r;
				row_pointers[y][x * 6 + 1] = t & 0xFF;
				row_pointers[y][x * 6 + 2] = elements[t].g;
				row_pointers[y][x * 6 + 3] = (t & 0xFF00) / 256;
				row_pointers[y][x * 6 + 4] = elements[t].b;
			}
		}

		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		info_ptr = png_create_info_struct(png_ptr);
		png_init_io(png_ptr, fp);

		png_set_IHDR(png_ptr, info_ptr, screen->w, screen->h,
					 16, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
					 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);

		png_write_info(png_ptr, info_ptr);
		png_write_image(png_ptr, row_pointers);
		png_write_end(png_ptr, NULL);
		for (int y = 0; y < screen->h; y++)
			free(row_pointers[y]);
		free(row_pointers);

		fclose(fp);

		png_destroy_write_struct(&png_ptr, &info_ptr);

#endif
	}
	return 0;
}

int load(char *filename)
{
	if ((strlen(filename) > 4) && ((!strcmp(filename + strlen(filename) - 4, ".bs2")) || (!strcmp(filename + strlen(filename) - 4, ".BS2"))))
	{
		parsefile(filename, 0);
	}
	if ((strlen(filename) > 4) && ((!strcmp(filename + strlen(filename) - 4, ".bmp")) || (!strcmp(filename + strlen(filename) - 4, ".BMP"))))
	{
		SDL_Surface *ptmp = SDL_LoadBMP(checkfilename(filename));
		SDL_Surface *p = SDL_CreateRGBSurface(SDL_SWSURFACE, ptmp->w, ptmp->h, 24, 0, 0, 0, 0);
		SDL_BlitSurface(ptmp, 0, p, 0);
		SDL_FreeSurface(ptmp);
		resize(p->w - 2, p->h - 2);
		SDL_Surface *screen = getRealSandSurface();
		if (p)
		{
			Element *elements = getElement(0);
			Uint8 r, g, b;
			Uint8 *tp;
			Uint16 max = getelementscount();
			Uint16 t;
			Uint32 dif, diftmp;
			for (int y = 0; y < p->h; y++)
				for (int x = 0; x < p->w; x++)
				{
					tp = ((Uint8 *)p->pixels + y * p->pitch + x * 3);
					b = *(tp + 0);
					g = *(tp + 1);
					r = *(tp + 2);
					t = 0;
					dif = 0xFFFFFFFF;
					for (int i = 0; i < max; i++)
					{
						diftmp = (elements[i].r - r) * (elements[i].r - r) + (elements[i].g - g) * (elements[i].g - g) + (elements[i].b - b) * (elements[i].b - b);
						if (diftmp < dif)
						{
							dif = diftmp;
							t = i;
							if (!dif)
								break;
						}
					}
					//						if ((elements[i].r == r) && (elements[i].g == g) && (elements[i].b == b)) { t = i; break; }
					if (t == 1)
						t = 0;
					*((Uint16 *)screen->pixels + y * screen->pitch / 2 + x) = t;
				}
			SDL_FreeSurface(p);
		}
	}
	if ((strlen(filename) > 4) && ((!strcmp(filename + strlen(filename) - 4, ".png")) || (!strcmp(filename + strlen(filename) - 4, ".png"))))
	{
#ifdef USE_PNG
		char header[8];
		png_structp png_ptr;
		png_infop info_ptr;
		png_bytep *row_pointers;

		FILE *fp = fopen(checkfilename(filename), "rb");
		if (fp == 0)
			return 0;

		fread(header, 1, 8, fp);
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		info_ptr = png_create_info_struct(png_ptr);
		png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, 8);
		png_read_info(png_ptr, info_ptr);

		resize(info_ptr->width - 2, info_ptr->height - 2);
		SDL_Surface *screen = getRealSandSurface();

		row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * info_ptr->height);
		for (unsigned int y = 0; y < info_ptr->height; y++)
			row_pointers[y] = (png_byte *)malloc(info_ptr->rowbytes);
		png_read_image(png_ptr, row_pointers);

		Uint16 t;
		Uint16 max = getelementscount();
		for (unsigned int y = 0; y < info_ptr->height; y++)
			for (unsigned int x = 0; x < info_ptr->width; x++)
			{
				t = row_pointers[y][(x * (info_ptr->pixel_depth / 8)) + 1] + row_pointers[y][(x * (info_ptr->pixel_depth / 8)) + 3] * 256;
				if (t < max)
					*((Uint16 *)screen->pixels + y * screen->pitch / 2 + x) = t;
			}
		fclose(fp);
		for (unsigned int y = 0; y < info_ptr->height; y++)
			free(row_pointers[y]);
		free(row_pointers);
		recalccolors();
		recalcused();
#endif
	}
	return 0;
}
