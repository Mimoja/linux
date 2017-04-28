#ifndef __ASM_ARM_ARCH_UNCOMPRESS_H
#define __ASM_ARM_ARCH_UNCOMPRESS_H

//TODO What is the official way for something like this?
#include <linux/../../lib/fonts/font_8x8.c>


#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240
#define FRAMEBUFFER_BASE 0x18000000

static void draw_pixel(int x, int y, int active)
{
	char *base = (char*) FRAMEBUFFER_BASE;
	int position = ((SCREEN_HEIGHT-y-1)+x*SCREEN_HEIGHT);
	char *fb = base + position*3;
	fb[0] = fb[1] = fb[2] = active * 0xFF;
}

struct {
	unsigned long x;
	unsigned long y;
}cursor;

static void putc(char c)
{

	const void* font = font_vga_8x8.data;
	char *char_ptr = (char*)(font + c * 8);
	int line, pixel;

	if(cursor.x > (SCREEN_WIDTH-8) || c == '\n' || c == '\r'){
		cursor.x = 0;
		cursor.y += 8;
		return;
	}

	for (line = 0; line < 8; line++) {
		for (pixel = 0; pixel < 8; pixel++) {
			draw_pixel(cursor.x+pixel, 
				   cursor.y+line, 
				   char_ptr[line] & (1<<(8-pixel)));
		}
	}
	cursor.x += 8;
}

static void flush(void) 
{
}

static inline void arch_decomp_setup(void)
{
	cursor.x = cursor.y = 0;
}

#endif
