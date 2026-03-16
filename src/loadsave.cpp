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

SDL_Surface* quicksaves[MAX_QUICKSAVES] = {};

void quicksave(int slot) {
	if ((slot >= MAX_QUICKSAVES) || (slot < 0)) return;

	SDL_Surface* screen = getRealSandSurface();
	if (!screen) return;
	if (quicksaves[slot])
		SDL_FreeSurface(quicksaves[slot]);
	quicksaves[slot] = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 16, 0, 0, 0, 0);
	if (quicksaves[slot])
		SDL_BlitSurface(screen, 0, quicksaves[slot], 0);
}

void quickload(int slot) {
	if ((slot >= MAX_QUICKSAVES) || (slot < 0)) return;
	if (!quicksaves[slot]) return;

	SDL_Surface* screen = getRealSandSurface();
	if (!screen) return;
	if ((quicksaves[slot]->w != screen->w) || (quicksaves[slot]->h != screen->h))
		resize(quicksaves[slot]->w, quicksaves[slot]->h);
	screen = getRealSandSurface();
	if (!screen) return;
	SDL_BlitSurface(quicksaves[slot], 0, screen, 0);
	recalccolors();
	recalcused();
}

int save(SDL_Surface* screen, char* filename) {
    Element* elements = getElement(0);
    const int elementsmaxnew = getelementsmax();
    const std::size_t flen = strlen(filename);

    auto endsWith = [&](const char* ext) {
        const std::size_t elen = strlen(ext);
        return (flen >= elen) && !strcmp(filename + flen - elen, ext);
    };

    if (endsWith(".bs2") || endsWith(".BS2")) {
        char tmp[512];
        FILE* fp = fopen(checkfilename(filename), "wb");
        if (!fp) return -1;

        snprintf(tmp, sizeof(tmp), "RESIZE %i %i\n", screen->w - 2, screen->h - 2);
        fputs(tmp, fp);
        snprintf(tmp, sizeof(tmp), "DRAW 0 FILLEDRECT 0 0 %i %i\n", screen->w, screen->h);
        fputs(tmp, fp);

        for (int i = 2; i < elementsmaxnew; i++) {
            if (!elements[i].name) continue;
            bool first = true;
            for (int y = 0; y < screen->h; y++) {
                for (int x = 0; x < screen->w; x++) {
                    if (*((Uint16*)screen->pixels + y * screen->pitch / 2 + x) != i) continue;
                    if (first) {
                        fputs("DRAW \"ELEMENT:", fp);
                        fputs(elements[i].name, fp);
                        fputs("\" POINTS 0 0 ", fp);
                        first = false;
                    }
                    snprintf(tmp, sizeof(tmp), "%i %i ", x, y);
                    fputs(tmp, fp);
                }
            }
            if (!first) fputs("\n", fp);
        }
        fclose(fp);
    }

    if (endsWith(".bmp") || endsWith(".BMP")) {
        SDL_Surface* p = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 32, 0, 0, 0, 0);
        if (!p) return -1;

        Uint32* color = new Uint32[elementsmaxnew];
        for (int i = elementsmaxnew - 1; i >= 0; i--)
            color[i] = SDL_MapRGB(p->format, elements[i].r, elements[i].g, elements[i].b);

        for (int y = 0; y < screen->h; y++)
            for (int x = 0; x < screen->w; x++)
                *((Uint32*)p->pixels + y * p->pitch / 4 + x) =
                    color[*((Uint16*)screen->pixels + y * screen->pitch / 2 + x)];

        SDL_SaveBMP(p, checkfilename(filename));
        delete[] color;
        SDL_FreeSurface(p);
    }

    if (endsWith(".png") || endsWith(".PNG")) {
#ifdef USE_PNG
        FILE* fp = fopen(checkfilename(filename), "wb");
        if (!fp) return -1;

        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png_ptr) {
            fclose(fp);
            return -1;
        }

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_write_struct(&png_ptr, nullptr);
            fclose(fp);
            return -1;
        }

        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_write_struct(&png_ptr, &info_ptr);
            fclose(fp);
            return -1;
        }

        png_bytep* row_pointers = static_cast<png_bytep*>(malloc(sizeof(png_bytep) * screen->h));
        if (!row_pointers) {
            png_destroy_write_struct(&png_ptr, &info_ptr);
            fclose(fp);
            return -1;
        }
        for (int y = 0; y < screen->h; y++)
            row_pointers[y] = nullptr;

        for (int y = 0; y < screen->h; y++) {

            row_pointers[y] = static_cast<png_byte*>(malloc(screen->w * 6));
            if (!row_pointers[y]) {
                for (int j = 0; j < y; j++) free(row_pointers[j]);
                free(row_pointers);
                png_destroy_write_struct(&png_ptr, &info_ptr);
                fclose(fp);
                return -1;
            }
            for (int x = 0; x < screen->w; x++) {
                const Uint16 t = *((Uint16*)screen->pixels + y * screen->pitch / 2 + x);

                row_pointers[y][x * 6 + 0] = elements[t].r;
                row_pointers[y][x * 6 + 1] = static_cast<png_byte>(t & 0xFF);
                row_pointers[y][x * 6 + 2] = elements[t].g;
                row_pointers[y][x * 6 + 3] = static_cast<png_byte>((t & 0xFF00) >> 8);
                row_pointers[y][x * 6 + 4] = elements[t].b;
                row_pointers[y][x * 6 + 5] = 0;
            }
        }

        png_init_io(png_ptr, fp);
        png_set_IHDR(png_ptr, info_ptr,
            static_cast<png_uint_32>(screen->w),
            static_cast<png_uint_32>(screen->h),
            16, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png_ptr, info_ptr);
        png_write_image(png_ptr, row_pointers);
        png_write_end(png_ptr, nullptr);

        for (int y = 0; y < screen->h; y++) free(row_pointers[y]);
        free(row_pointers);
        fclose(fp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
#endif
    }

    return 0;
}

int load(char* filename) {
	if ((strlen(filename) > 4) && ((!strcmp(filename + strlen(filename) - 4, ".bs2")) || (!strcmp(filename + strlen(filename) - 4, ".BS2")))) {
		parsefile(filename, 0);
	}
	if ((strlen(filename) > 4) && ((!strcmp(filename + strlen(filename) - 4, ".bmp")) || (!strcmp(filename + strlen(filename) - 4, ".BMP")))) {
		SDL_Surface* ptmp = SDL_LoadBMP(checkfilename(filename));
		if (!ptmp) return -1;
		SDL_Surface* p = SDL_CreateRGBSurface(SDL_SWSURFACE, ptmp->w, ptmp->h, 24, 0, 0, 0, 0);

		if (!p) {
			SDL_FreeSurface(ptmp);
			return -1;
		}
		SDL_BlitSurface(ptmp, 0, p, 0);
		SDL_FreeSurface(ptmp);
		resize(p->w - 2, p->h - 2);
		SDL_Surface* screen = getRealSandSurface();

		if (screen) {
			Element* elements = getElement(0);
			Uint8 r, g, b;
			Uint8* tp;
			Uint16 max = getelementscount();
			Uint16 t;
			Uint32 dif, diftmp;
			for (int y = 0; y < p->h; y++)
				for (int x = 0; x < p->w; x++) {
					tp = ((Uint8*)p->pixels + y * p->pitch + x * 3);
					b = *(tp + 0);
					g = *(tp + 1);
					r = *(tp + 2);
					t = 0;
					dif = 0xFFFFFFFF;
					for (int i = 0; i < max; i++) {
						int dr = (int)elements[i].r - (int)r;
						int dg = (int)elements[i].g - (int)g;
						int db = (int)elements[i].b - (int)b;
						diftmp = (Uint32)(dr * dr + dg * dg + db * db);
						if (diftmp < dif) {
							dif = diftmp;
							t = i;
							if (!dif)
								break;
						}
					}

					if (t == 1) t = 0;
					*((Uint16*)screen->pixels + y * screen->pitch / 2 + x) = t;
				}
		}
		SDL_FreeSurface(p);
	}

	if ((strlen(filename) > 4) && ((!strcmp(filename + strlen(filename) - 4, ".png")) || (!strcmp(filename + strlen(filename) - 4, ".PNG")))) {
#ifdef USE_PNG
		char header[8];
		png_structp png_ptr;
		png_infop info_ptr;
		png_bytep* row_pointers;

		FILE* fp = fopen(checkfilename(filename), "rb");
		if (!fp) return -1;

		fread(header, 1, 8, fp);

		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr) {
			fclose(fp);
			return -1;
		}
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			png_destroy_read_struct(&png_ptr, NULL, NULL);
			fclose(fp);
			return -1;
		}

		if (setjmp(png_jmpbuf(png_ptr))) {
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			fclose(fp);
			return -1;
		}

		png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, 8);
		png_read_info(png_ptr, info_ptr);

		png_uint_32 img_width = png_get_image_width(png_ptr, info_ptr);
		png_uint_32 img_height = png_get_image_height(png_ptr, info_ptr);
		png_size_t img_rowbytes = png_get_rowbytes(png_ptr, info_ptr);
		int img_bit_depth = png_get_bit_depth(png_ptr, info_ptr);
		int img_channels = png_get_channels(png_ptr, info_ptr);
		png_size_t pixel_bytes = (png_size_t)(img_bit_depth / 8) * img_channels;

		resize((int)img_width - 2, (int)img_height - 2);
		SDL_Surface* screen = getRealSandSurface();

		row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * img_height);
		if (!row_pointers) {
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			fclose(fp);
			return -1;
		}
		for (png_uint_32 y = 0; y < img_height; y++)
			row_pointers[y] = (png_byte*)malloc(img_rowbytes);
		png_read_image(png_ptr, row_pointers);

		if (screen) {
			Uint16 t;
			Uint16 max = getelementscount();
			for (png_uint_32 y = 0; y < img_height; y++)
				for (png_uint_32 x = 0; x < img_width; x++) {
					t = row_pointers[y][x * pixel_bytes + 1] + row_pointers[y][x * pixel_bytes + 3] * 256;
					if (t < max) *((Uint16*)screen->pixels + y * screen->pitch / 2 + x) = t;
				}
		}

		fclose(fp);
		for (png_uint_32 y = 0; y < img_height; y++)
			free(row_pointers[y]);
		free(row_pointers);

		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

		recalccolors();
		recalcused();
#endif
	}
	return 0;
}