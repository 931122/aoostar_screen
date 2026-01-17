/* generate by chatgpt */
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "trans.h"
#include "draw.h"
#include "widget.h"

#define DEFAULT_FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define SCREEN_WIDTH   960
#define SCREEN_HEIGHT  376
#define BYTES_PER_PIXEL 2
#define BLOCK_SIZE     47

static const char* utf8_decode(const char *s, uint32_t *out_cp)
{
	uint8_t c = (uint8_t)*s;
	if (c < 0x80) {
		*out_cp = c;
		return s + 1;
	} else if ((c >> 5) == 0x6) {
		*out_cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
		return s + 2;
	} else if ((c >> 4) == 0xE) {
		*out_cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
		return s + 3;
	} else if ((c >> 3) == 0x1E) {
		*out_cp = ((s[0] & 0x07) << 18) |
			((s[1] & 0x3F) << 12) |
			((s[2] & 0x3F) << 6) |
			(s[3] & 0x3F);
		return s + 4;
	} else {
		*out_cp = '?';
		return s + 1;
	}
}

void img2bg(uint8_t *bg, const uint8_t *image, int img_w, int img_h, int dst_x, int dst_y)
{
	for (int y = 0; y < img_h; y++) {
		int fb_y = dst_y + y;
		if (fb_y < 0 || fb_y >= SCREEN_HEIGHT) continue;

		for (int x = 0; x < img_w; x++) {
			int fb_x = dst_x + x;
			if (fb_x < 0 || fb_x >= SCREEN_WIDTH) continue;

			int fb_index = (fb_y * SCREEN_WIDTH + fb_x) * PIXEL_SIZE;
			int img_index = (y * img_w + x) * PIXEL_SIZE;

			bg[fb_index + 0] = image[img_index + 0];
			bg[fb_index + 1] = image[img_index + 1];
		}
	}
}

void draw_str(struct widget *w)
{
	const char *utf8_text = (const char *)w->txt;
	const char *font_path = w->font ? w->font: DEFAULT_FONT_PATH;
	int font_size = w->font_size;
	int start_x = w->x;
	int start_y = w->y;
	uint16_t fg_color = w->color;
	const uint8_t *bg = w->bg;
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) return;

	FT_Face face;
	if (FT_New_Face(ft, font_path, 0, &face)) {
		FT_Done_FreeType(ft);
		return;
	}

	FT_Set_Pixel_Sizes(face, 0, font_size);
	int ascent = face->size->metrics.ascender >> 6;
	int descent = -(face->size->metrics.descender >> 6);
	int line_height = ascent + descent + 2;
	int baseline = start_y + ascent;

	uint8_t *overlay = calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(uint8_t));
	if (!overlay) {
		FT_Done_Face(face);
		FT_Done_FreeType(ft);
		return;
	}

	int x = start_x;
	int y = baseline;
	int min_x = SCREEN_WIDTH, min_y = SCREEN_HEIGHT;
	int max_x = 0, max_y = 0;

	const char *p = utf8_text;
	while (*p) {
		if (*p == '\n') {
			x = start_x;
			y += line_height;
			p++;
			continue;
		}

		uint32_t codepoint;
		p = utf8_decode(p, &codepoint);

		if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) {
			FT_Load_Char(face, 0x25A1, FT_LOAD_RENDER);
		}

		FT_GlyphSlot g = face->glyph;
		FT_Bitmap *bmp = &g->bitmap;
		int x_offset = g->bitmap_left;
		int y_offset = g->bitmap_top;

		for (int row = 0; row < bmp->rows; row++) {
			int dst_y = y - y_offset + row;
			if (dst_y < 0 || dst_y >= SCREEN_HEIGHT) continue;

			for (int col = 0; col < bmp->width; col++) {
				int dst_x = x + x_offset + col;
				if (dst_x < 0 || dst_x >= SCREEN_WIDTH) continue;

				int gray = bmp->buffer[row * bmp->pitch + col];
				if (gray == 0) continue;

				overlay[dst_y * SCREEN_WIDTH + dst_x] = 1;

				if (dst_x < min_x) min_x = dst_x;
				if (dst_y < min_y) min_y = dst_y;
				if (dst_x > max_x) max_x = dst_x;
				if (dst_y > max_y) max_y = dst_y;
			}
		}
		x += (g->advance.x >> 6);
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	if (min_x > max_x || min_y > max_y) {
		free(overlay);
		return;
	}

	int region_w = max_x - min_x + 1;
	int region_h = max_y - min_y + 1;
	w->w = region_w;

	uint8_t *line_buf = malloc(region_w * PIXEL_SIZE);
	if (!line_buf) {
		free(overlay);
		return;
	}

	start_drawing();
	int screen_y;
	uint32_t addr;
	for (int row = 0; row < region_h; row++) {
		screen_y = min_y + row;
		if (w->multi_color) {
			fg_color = w->multi_color[row / line_height];
			//printf("%d ", fg_color);
		}
		// 复制背景一行
		memcpy(line_buf,
				bg + (screen_y * SCREEN_WIDTH + min_x) * PIXEL_SIZE,
				region_w * PIXEL_SIZE);

		// 覆盖前景色
		for (int col = 0; col < region_w; col++) {
			if (overlay[screen_y * SCREEN_WIDTH + min_x + col]) {
				line_buf[col * 2] = fg_color >> 8;
				line_buf[col * 2 + 1] = fg_color & 0xFF;
			}
		}

		int row_bytes = region_w * PIXEL_SIZE;
		int offset = 0;
		while (offset < row_bytes) {
			int send_len = (row_bytes - offset >= BLOCK_SIZE) ? BLOCK_SIZE : (row_bytes - offset);
			uint8_t send_buf[BLOCK_SIZE];

			// 先拷贝当前有效像素数据
			memcpy(send_buf, line_buf + offset, send_len);

			if (send_len < BLOCK_SIZE) {
				// 补齐剩余字节，从当前行末尾之后的bg数据开始补
				int remain = BLOCK_SIZE - send_len;
				const uint8_t *bg_ptr = bg + (screen_y * SCREEN_WIDTH + min_x) * PIXEL_SIZE + row_bytes + (offset - row_bytes);
				// 注意：offset - row_bytes <= 0 或 0，所以简化为：
				bg_ptr = bg + (screen_y * SCREEN_WIDTH + min_x) * PIXEL_SIZE + row_bytes;
				memcpy(send_buf + send_len, bg_ptr, remain);
			}

			addr = (screen_y * SCREEN_WIDTH + min_x) * PIXEL_SIZE + offset;
			send_image_block(addr, send_buf);
			offset += BLOCK_SIZE;
		}
	}
	w->h = screen_y - start_y+1;

	show_image();

	free(line_buf);
	free(overlay);
}

void restore_region(struct widget *w)
{
	int base_addr = 0;
	char buf[SCREEN_WIDTH*PIXEL_SIZE];
	int count = (w->w* PIXEL_SIZE + 47)/47*47;

	start_drawing();
	for (int i= 0; i < w->h; i++) {
		base_addr = ((w->y + i) * SCREEN_WIDTH + w->x) * PIXEL_SIZE;
		memcpy(buf, w->bg+base_addr, count);
		for (int j = 0;j < count/BLOCK_SIZE;j++)
			send_image_block(base_addr+j*BLOCK_SIZE, buf+j*BLOCK_SIZE);
	}

	//show_image();
}

void draw_str_to_bg(uint16_t *bg, int x, int y, const char *utf8_text, const char *font_path, int font_size, uint16_t color_rgb565)
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) return;

	FT_Face face;
	if (FT_New_Face(ft, font_path, 0, &face)) {
		FT_Done_FreeType(ft);
		return;
	}

	FT_Set_Pixel_Sizes(face, 0, font_size);

	int pen_x = x;
	int pen_y = y + (face->size->metrics.ascender >> 6); // baseline

	const char *p = utf8_text;

	while (*p) {
		if (*p == '\n') {
			pen_x = x;
			pen_y += font_size + 2;
			p++;
			continue;
		}

		uint32_t codepoint;
		p = utf8_decode(p, &codepoint);

		if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) {
			FT_Load_Char(face, 0x25A1, FT_LOAD_RENDER);  // fallback to white square
		}

		FT_GlyphSlot g = face->glyph;
		FT_Bitmap *bmp = &g->bitmap;
		int bmp_x = pen_x + g->bitmap_left;
		int bmp_y = pen_y - g->bitmap_top;

		for (int row = 0; row < bmp->rows; row++) {
			int dst_y = bmp_y + row;
			if (dst_y < 0 || dst_y >= SCREEN_HEIGHT) continue;

			for (int col = 0; col < bmp->width; col++) {
				int dst_x = bmp_x + col;
				if (dst_x < 0 || dst_x >= SCREEN_WIDTH) continue;

				uint8_t gray = bmp->buffer[row * bmp->pitch + col];
				if (gray == 0) continue;

				// 混合/不透明地写入颜色
				bg[dst_y * SCREEN_WIDTH + dst_x] = color_rgb565;
			}
		}

		pen_x += (g->advance.x >> 6);
	}

	FT_Done_Face(face);
    FT_Done_FreeType(ft);
}
#ifdef __cplusplus
};
#endif
/********************* End Of File: draw.c *********************/
