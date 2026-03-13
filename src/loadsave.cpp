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
#include <string.h>
#include <stdio.h>

// FIX #17: Zero-initialize the array at definition instead of using a fragile firstquick flag.
SDL_Surface *quicksaves[MAX_QUICKSAVES] = {};

void quicksave(int slot)
{
	if ((slot >= MAX_QUICKSAVES) || (slot < 0))
		return;
	// FIX #1: Null-check getRealSandSurface() before dereferencing.
	SDL_Surface *screen = getRealSandSurface();
	if (!screen)
		return;
	if (quicksaves[slot])
		SDL_FreeSurface(quicksaves[slot]);
	quicksaves[slot] = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 16, 0, 0, 0, 0);
	if (quicksaves[slot])
		SDL_BlitSurface(screen, 0, quicksaves[slot], 0);
}

void quickload(int slot)
{
	if ((slot >= MAX_QUICKSAVES) || (slot < 0))
		return;
	if (!quicksaves[slot])
		return;
	// FIX #1: Null-check getRealSandSurface() before dereferencing.
	SDL_Surface *screen = getRealSandSurface();
	if (!screen)
		return;
	if ((quicksaves[slot]->w != screen->w) || (quicksaves[slot]->h != screen->h))
		resize(quicksaves[slot]->w, quicksaves[slot]->h);
	screen = getRealSandSurface();
	if (!screen)
		return;
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
		// FIX #3: Use a larger buffer to avoid overflow for large screen dimensions.
		char tmp[512];
		// FIX #2: Check that fopen succeeded before using fp.
		FILE *fp = fopen(checkfilename(filename), "wb");
		if (!fp)
			return -1;
		snprintf(tmp, sizeof(tmp), "RESIZE %i %i\n", screen->w - 2, screen->h - 2);
		fputs(tmp, fp);
		snprintf(tmp, sizeof(tmp), "DRAW 0 FILLEDRECT 0 0 %i %i\n", screen->w, screen->h);
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
							snprintf(tmp, sizeof(tmp), "%i %i ", x, y);
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
		// FIX #4: Null-check SDL_CreateRGBSurface before use.
		SDL_Surface *p = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 32, 0, 0, 0, 0);
		if (!p)
			return -1;
		Uint32 *color = new Uint32[elementsmaxnew];
		for (int i = elementsmaxnew - 1; i >= 0; i--)
			color[i] = SDL_MapRGB(p->format, elements[i].r, elements[i].g, elements[i].b);
		for (int y = 0; y < screen->h; y++)
			for (int x = 0; x < screen->w; x++)
				*((Uint32 *)p->pixels + y * p->pitch / 4 + x) = color[*((Uint16 *)screen->pixels + y * screen->pitch / 2 + x)];
		SDL_SaveBMP(p, checkfilename(filename));
		// FIX #5: Use delete[] for array allocation, and free the SDL surface.
		delete[] color;
		SDL_FreeSurface(p);
	}
	// FIX #16: Second comparison corrected from ".png" to ".PNG" to match uppercase extensions.
	if ((strlen(filename) > 4) && ((!strcmp(filename + strlen(filename) - 4, ".png")) || (!strcmp(filename + strlen(filename) - 4, ".PNG"))))
	{
#ifdef USE_PNG
		// FIX #6: Check fopen result before proceeding.
		FILE *fp = fopen(checkfilename(filename), "wb");
		if (!fp)
			return -1;

		png_structp png_ptr;
		png_infop info_ptr;
		png_bytep *row_pointers;

		// FIX #6 + #8: Null-check libpng structs and set up setjmp error handling.
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr)
		{
			fclose(fp);
			return -1;
		}
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			png_destroy_write_struct(&png_ptr, NULL);
			fclose(fp);
			return -1;
		}
		// FIX #8: setjmp for libpng error recovery to avoid longjmp into nowhere.
		if (setjmp(png_jmpbuf(png_ptr)))
		{
			png_destroy_write_struct(&png_ptr, &info_ptr);
			fclose(fp);
			return -1;
		}

		row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * screen->h);
		if (!row_pointers)
		{
			png_destroy_write_struct(&png_ptr, &info_ptr);
			fclose(fp);
			return -1;
		}
		// FIX #9: Initialize pointers to NULL so partial-alloc cleanup is safe.
		for (int y = 0; y < screen->h; y++)
			row_pointers[y] = NULL;

		for (int y = 0; y < screen->h; y++)
		{
			// Each pixel is 3 channels * 2 bytes = 6 bytes.
			row_pointers[y] = (png_byte *)malloc(screen->w * 6);
			if (!row_pointers[y])
			{
				for (int j = 0; j < y; j++)
					free(row_pointers[j]);
				free(row_pointers);
				png_destroy_write_struct(&png_ptr, &info_ptr);
				fclose(fp);
				return -1;
			}
			for (int x = 0; x < screen->w; x++)
			{
				Uint16 t = *((Uint16 *)screen->pixels + y * screen->pitch / 2 + x);
				// Each 16-bit channel stored high-byte then low-byte (big-endian per PNG spec).
				// Red channel: element color r, with high byte of t interleaved.
				row_pointers[y][x * 6 + 0] = elements[t].r;
				row_pointers[y][x * 6 + 1] = t & 0xFF;
				// Green channel: element color g, with high byte of t interleaved.
				row_pointers[y][x * 6 + 2] = elements[t].g;
				row_pointers[y][x * 6 + 3] = (t & 0xFF00) >> 8;
				// FIX #7: Blue channel was missing its second byte (index +5). Now fully written.
				row_pointers[y][x * 6 + 4] = elements[t].b;
				row_pointers[y][x * 6 + 5] = 0;
			}
		}

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
		// FIX #10: Null-check SDL_LoadBMP before dereferencing.
		SDL_Surface *ptmp = SDL_LoadBMP(checkfilename(filename));
		if (!ptmp)
			return -1;
		SDL_Surface *p = SDL_CreateRGBSurface(SDL_SWSURFACE, ptmp->w, ptmp->h, 24, 0, 0, 0, 0);
		// FIX #11: Check p before using it (moved check to before resize/dereference).
		if (!p)
		{
			SDL_FreeSurface(ptmp);
			return -1;
		}
		SDL_BlitSurface(ptmp, 0, p, 0);
		SDL_FreeSurface(ptmp);
		resize(p->w - 2, p->h - 2);
		SDL_Surface *screen = getRealSandSurface();
		// FIX #1: Null-check screen.
		if (screen)
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
						int dr = (int)elements[i].r - (int)r;
						int dg = (int)elements[i].g - (int)g;
						int db = (int)elements[i].b - (int)b;
						diftmp = (Uint32)(dr * dr + dg * dg + db * db);
						if (diftmp < dif)
						{
							dif = diftmp;
							t = i;
							if (!dif)
								break;
						}
					}

					if (t == 1) t = 0;
					*((Uint16 *)screen->pixels + y * screen->pitch / 2 + x) = t;
				}
		}
		SDL_FreeSurface(p);
	}
	// FIX #16: Second comparison corrected from ".png" to ".PNG".
	if ((strlen(filename) > 4) && ((!strcmp(filename + strlen(filename) - 4, ".png")) || (!strcmp(filename + strlen(filename) - 4, ".PNG"))))
	{
#ifdef USE_PNG
		char header[8];
		png_structp png_ptr;
		png_infop info_ptr;
		png_bytep *row_pointers;

		FILE *fp = fopen(checkfilename(filename), "rb");
		if (!fp)
			return -1;

		fread(header, 1, 8, fp);

		// FIX #13: Null-check libpng structs before use.
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr)
		{
			fclose(fp);
			return -1;
		}
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			png_destroy_read_struct(&png_ptr, NULL, NULL);
			fclose(fp);
			return -1;
		}
		// FIX #14: setjmp for libpng error recovery.
		if (setjmp(png_jmpbuf(png_ptr)))
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			fclose(fp);
			return -1;
		}

		png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, 8);
		png_read_info(png_ptr, info_ptr);

		// Use libpng 1.5+ accessor functions instead of direct info_ptr field access.
		png_uint_32 img_width = png_get_image_width(png_ptr, info_ptr);
		png_uint_32 img_height = png_get_image_height(png_ptr, info_ptr);
		png_size_t img_rowbytes = png_get_rowbytes(png_ptr, info_ptr);
		int img_bit_depth = png_get_bit_depth(png_ptr, info_ptr);
		int img_channels = png_get_channels(png_ptr, info_ptr);
		png_size_t pixel_bytes = (png_size_t)(img_bit_depth / 8) * img_channels;

		resize((int)img_width - 2, (int)img_height - 2);
		SDL_Surface *screen = getRealSandSurface();

		row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * img_height);
		if (!row_pointers)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			fclose(fp);
			return -1;
		}
		for (png_uint_32 y = 0; y < img_height; y++)
			row_pointers[y] = (png_byte *)malloc(img_rowbytes);
		png_read_image(png_ptr, row_pointers);

		// FIX #1: Null-check screen before writing pixels.
		if (screen)
		{
			Uint16 t;
			Uint16 max = getelementscount();
			for (png_uint_32 y = 0; y < img_height; y++)
				for (png_uint_32 x = 0; x < img_width; x++)
				{
					t = row_pointers[y][x * pixel_bytes + 1] + row_pointers[y][x * pixel_bytes + 3] * 256;
					if (t < max)
						*((Uint16 *)screen->pixels + y * screen->pitch / 2 + x) = t;
				}
		}

		fclose(fp);
		for (png_uint_32 y = 0; y < img_height; y++)
			free(row_pointers[y]);
		free(row_pointers);

		// FIX #15: Destroy the read struct to avoid memory leak.
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

		recalccolors();
		recalcused();
#endif
	}
	return 0;
}