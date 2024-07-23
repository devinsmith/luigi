//
// Copyright (c) 2020 nakst
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
// IN THE SOFTWARE.
//

#include "luigi.h"
#include "luigi_private.h"

void UIDrawBlock(UIPainter *painter, UIRectangle rectangle, uint32_t color) {
	rectangle = UIRectangleIntersection(painter->clip, rectangle);

	if (!UI_RECT_VALID(rectangle)) {
		return;
	}

#ifdef UI_SSE2
	__m128i color4 = _mm_set_epi32(color, color, color, color);
#endif

	for (int line = rectangle.t; line < rectangle.b; line++) {
		uint32_t *bits = painter->bits + line * painter->width + rectangle.l;
		int count = UI_RECT_WIDTH(rectangle);

#ifdef UI_SSE2
		while (count >= 4) {
			_mm_storeu_si128((__m128i *) bits, color4);
			bits += 4;
			count -= 4;
		} 
#endif

		while (count--) {
			*bits++ = color;
		}
	}

#ifdef UI_DEBUG
	painter->fillCount += UI_RECT_WIDTH(rectangle) * UI_RECT_HEIGHT(rectangle);
#endif
}

bool UIDrawLine(UIPainter *painter, int x0, int y0, int x1, int y1, uint32_t color) {
	// Apply the clip.

	UIRectangle c = painter->clip;
	if (!UI_RECT_VALID(c)) return false;
	int dx = x1 - x0, dy = y1 - y0;
	const int p[4] = { -dx, dx, -dy, dy };
	const int q[4] = { x0 - c.l, c.r - 1 - x0, y0 - c.t, c.b - 1 - y0 };
	float t0 = 0.0f, t1 = 1.0f; // How far along the line the points end up.

	for (int i = 0; i < 4; i++) {
		if (!p[i] && q[i] < 0) return false;
		float r = (float) q[i] / p[i];
		if (p[i] < 0 && r > t1) return false;
		if (p[i] > 0 && r < t0) return false;
		if (p[i] < 0 && r > t0) t0 = r;
		if (p[i] > 0 && r < t1) t1 = r;
	}

	x1 = x0 + t1 * dx, y1 = y0 + t1 * dy;
	x0 += t0 * dx, y0 += t0 * dy;

	// Calculate the delta X and delta Y.

	if (y1 < y0) {
		int t;
		t = x0, x0 = x1, x1 = t;
		t = y0, y0 = y1, y1 = t;
	}

	dx = x1 - x0, dy = y1 - y0;
	int dxs = dx < 0 ? -1 : 1;
	if (dx < 0) dx = -dx;

	// Draw the line using Bresenham's line algorithm.

	uint32_t *bits = painter->bits + y0 * painter->width + x0;

	if (dy * dy < dx * dx) {
		int m = 2 * dy - dx;

		for (int i = 0; i < dx; i++, bits += dxs) {
			*bits = color;
			if (m > 0) bits += painter->width, m -= 2 * dx;
			m += 2 * dy;
		}
	} else {
		int m = 2 * dx - dy;

		for (int i = 0; i < dy; i++, bits += painter->width) {
			*bits = color;
			if (m > 0) bits += dxs, m -= 2 * dy;
			m += 2 * dx;
		}
	}

	return true;
}

void UIDrawTriangle(UIPainter *painter, int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
	// Step 1: Sort the points by their y-coordinate.
	if (y1 < y0) { int xt = x0; x0 = x1, x1 = xt; int yt = y0; y0 = y1, y1 = yt; }
	if (y2 < y1) { int xt = x1; x1 = x2, x2 = xt; int yt = y1; y1 = y2, y2 = yt; }
	if (y1 < y0) { int xt = x0; x0 = x1, x1 = xt; int yt = y0; y0 = y1, y1 = yt; }
	if (y2 == y0) return;

	// Step 2: Clip the triangle.
	if (x0 < painter->clip.l && x1 < painter->clip.l && x2 < painter->clip.l) return;
	if (x0 >= painter->clip.r && x1 >= painter->clip.r && x2 >= painter->clip.r) return;
	if (y2 < painter->clip.t || y0 >= painter->clip.b) return;
	bool needsXClip = x0 < painter->clip.l + 1 || x0 >= painter->clip.r - 1
		|| x1 < painter->clip.l + 1 || x1 >= painter->clip.r - 1
		|| x2 < painter->clip.l + 1 || x2 >= painter->clip.r - 1;
	bool needsYClip = y0 < painter->clip.t + 1 || y2 >= painter->clip.b - 1;
#define _UI_DRAW_TRIANGLE_APPLY_CLIP(xo, yo) \
	if (needsYClip && (yi + yo < painter->clip.t || yi + yo >= painter->clip.b)) continue; \
	if (needsXClip && xf + xo < painter->clip.l) xf = painter->clip.l - xo; \
	if (needsXClip && xt + xo > painter->clip.r) xt = painter->clip.r - xo;

	// Step 3: Split into 2 triangles with bases aligned with the x-axis.
	float xm0 = (x2 - x0) * (y1 - y0) / (y2 - y0), xm1 = x1 - x0;
	if (xm1 < xm0) { float xmt = xm0; xm0 = xm1, xm1 = xmt; }
	float xe0 = xm0 + x0 - x2, xe1 = xm1 + x0 - x2;
	int ym = y1 - y0, ye = y2 - y1;
	float ymr = 1.0f / ym, yer = 1.0f / ye;

	// Step 4: Draw the top part.
	for (float y = 0; y < ym; y++) {
		int xf = xm0 * y * ymr, xt = xm1 * y * ymr, yi = (int) y;
		_UI_DRAW_TRIANGLE_APPLY_CLIP(x0, y0);
		uint32_t *b = &painter->bits[(yi + y0) * painter->width + x0];
		for (int x = xf; x < xt; x++) b[x] = color;
	}

	// Step 5: Draw the bottom part.
	for (float y = 0; y < ye; y++) {
		int xf = xe0 * (ye - y) * yer, xt = xe1 * (ye - y) * yer, yi = (int) y;
		_UI_DRAW_TRIANGLE_APPLY_CLIP(x2, y1);
		uint32_t *b = &painter->bits[(yi + y1) * painter->width + x2];
		for (int x = xf; x < xt; x++) b[x] = color;
	}
}

void UIDrawTriangleOutline(UIPainter *painter, int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
	UIDrawLine(painter, x0, y0, x1, y1, color);
	UIDrawLine(painter, x1, y1, x2, y2, color);
	UIDrawLine(painter, x2, y2, x0, y0, color);
}

void UIDrawInvert(UIPainter *painter, UIRectangle rectangle) {
	rectangle = UIRectangleIntersection(painter->clip, rectangle);

	if (!UI_RECT_VALID(rectangle)) {
		return;
	}

	for (int line = rectangle.t; line < rectangle.b; line++) {
		uint32_t *bits = painter->bits + line * painter->width + rectangle.l;
		int count = UI_RECT_WIDTH(rectangle);

		while (count--) {
			uint32_t in = *bits;
			*bits = in ^ 0xFFFFFF;
			bits++;
		}
	}
}

void UIDrawGlyph(UIPainter *painter, int x0, int y0, int c, uint32_t color) {
#ifdef UI_FREETYPE
	UIFont *font = ui.activeFont;

	if (font->isFreeType) {
		if (c < 0 || c > 127) c = '?';
		if (c == '\r') c = ' ';

		if (!font->glyphsRendered[c]) {
			FT_Load_Char(font->font, c == 24 ? 0x2191 : c == 25 ? 0x2193 : c == 26 ? 0x2192 : c == 27 ? 0x2190 : c, FT_LOAD_DEFAULT);
#ifdef UI_FREETYPE_SUBPIXEL
			FT_Render_Glyph(font->font->glyph, FT_RENDER_MODE_LCD);
#else
			FT_Render_Glyph(font->font->glyph, FT_RENDER_MODE_NORMAL);
#endif
			FT_Bitmap_Copy(ui.ft, &font->font->glyph->bitmap, &font->glyphs[c]);
			font->glyphOffsetsX[c] = font->font->glyph->bitmap_left;
			font->glyphOffsetsY[c] = font->font->size->metrics.ascender / 64 - font->font->glyph->bitmap_top;
			font->glyphsRendered[c] = true;
		}

		FT_Bitmap *bitmap = &font->glyphs[c];
		x0 += font->glyphOffsetsX[c], y0 += font->glyphOffsetsY[c];

		for (int y = 0; y < (int) bitmap->rows; y++) {
			if (y0 + y < painter->clip.t) continue;
			if (y0 + y >= painter->clip.b) break;

			int width = bitmap->width;
#ifdef UI_FREETYPE_SUBPIXEL
			width /= 3;
#endif

			for (int x = 0; x < width; x++) {
				if (x0 + x < painter->clip.l) continue;
				if (x0 + x >= painter->clip.r) break;

				uint32_t *destination = painter->bits + (x0 + x) + (y0 + y) * painter->width;
				uint32_t original = *destination;

#ifdef UI_FREETYPE_SUBPIXEL
				uint32_t ra = ((uint8_t *) bitmap->buffer)[x * 3 + y * bitmap->pitch + 0];
				uint32_t ga = ((uint8_t *) bitmap->buffer)[x * 3 + y * bitmap->pitch + 1];
				uint32_t ba = ((uint8_t *) bitmap->buffer)[x * 3 + y * bitmap->pitch + 2];
				ra += (ga - ra) / 2, ba += (ga - ba) / 2;
#else
				uint32_t ra = ((uint8_t *) bitmap->buffer)[x + y * bitmap->pitch];
				uint32_t ga = ra, ba = ra;
#endif
				uint32_t r2 = (255 - ra) * ((original & 0x000000FF) >> 0);
				uint32_t g2 = (255 - ga) * ((original & 0x0000FF00) >> 8);
				uint32_t b2 = (255 - ba) * ((original & 0x00FF0000) >> 16);
				uint32_t r1 = ra * ((color & 0x000000FF) >> 0);
				uint32_t g1 = ga * ((color & 0x0000FF00) >> 8);
				uint32_t b1 = ba * ((color & 0x00FF0000) >> 16);

				uint32_t result = 0xFF000000 | (0x00FF0000 & ((b1 + b2) << 8)) 
					| (0x0000FF00 & ((g1 + g2) << 0)) 
					| (0x000000FF & ((r1 + r2) >> 8));
				*destination = result;
			}
		}

		return;
	}
#endif

	if (c < 0 || c > 127) c = '?';

	UIRectangle rectangle = UIRectangleIntersection(painter->clip, UI_RECT_4(x0, x0 + 8, y0, y0 + 16));

//	const uint8_t *data = UIGet(const uint8_t *) _uiFont + c * 16;
	const uint8_t *data = UIFontGetBuiltInData() + c * 16;

	for (int i = rectangle.t; i < rectangle.b; i++) {
		uint32_t *bits = painter->bits + i * painter->width + rectangle.l;
		uint8_t byte = data[i - y0];

		for (int j = rectangle.l; j < rectangle.r; j++) {
			if (byte & (1 << (j - x0))) {
				*bits = color;
			}

			bits++;
		}
	}
}

void UIDrawString(UIPainter *painter, UIRectangle r, const char *string, ptrdiff_t bytes, uint32_t color, int align, UIStringSelection *selection) {
	UIRectangle oldClip = painter->clip;
	painter->clip = UIRectangleIntersection(r, oldClip);

	if (!UI_RECT_VALID(painter->clip)) {
		painter->clip = oldClip;
		return;
	}

	if (bytes == -1) {
		bytes = UIStringLength(string);
	}

	int width = UIMeasureStringWidth(string, bytes);
	int height = UIMeasureStringHeight();
	int x = align == UI_ALIGN_CENTER ? ((r.l + r.r - width) / 2) : align == UI_ALIGN_RIGHT ? (r.r - width) : r.l;
	int y = (r.t + r.b - height) / 2;
	int i = 0, j = 0;

	int selectFrom = -1, selectTo = -1;

	if (selection) {
		selectFrom = selection->carets[0];
		selectTo = selection->carets[1];
		
		if (selectFrom > selectTo) {
			UI_SWAP(int, selectFrom, selectTo);
		}
	}

	for (; j < bytes; j++) {
		char c = *string++;
		uint32_t colorText = color;

		if (j >= selectFrom && j < selectTo) {
			UIDrawBlock(painter, UI_RECT_4(x, x + ui.activeFont->glyphWidth, y, y + height), selection->colorBackground);
			colorText = selection->colorText;
		}

		if (c != '\t') {
			UIDrawGlyph(painter, x, y, c, colorText);
		}

		if (selection && selection->carets[0] == j) {
			UIDrawInvert(painter, UI_RECT_4(x, x + 1, y, y + height));
		}

		x += ui.activeFont->glyphWidth, i++;

		if (c == '\t') {
			while (i & 3) x += ui.activeFont->glyphWidth, i++;
		}
	}

	if (selection && selection->carets[0] == j) {
		UIDrawInvert(painter, UI_RECT_4(x, x + 1, y, y + height));
	}

	painter->clip = oldClip;
}

void UIDrawBorder(UIPainter *painter, UIRectangle r, uint32_t borderColor, UIRectangle borderSize) {
	UIDrawBlock(painter, UI_RECT_4(r.l, r.r, r.t, r.t + borderSize.t), borderColor);
	UIDrawBlock(painter, UI_RECT_4(r.l, r.l + borderSize.l, r.t + borderSize.t, r.b - borderSize.b), borderColor);
	UIDrawBlock(painter, UI_RECT_4(r.r - borderSize.r, r.r, r.t + borderSize.t, r.b - borderSize.b), borderColor);
	UIDrawBlock(painter, UI_RECT_4(r.l, r.r, r.b - borderSize.b, r.b), borderColor);
}

void UIDrawRectangle(UIPainter *painter, UIRectangle r, uint32_t mainColor, uint32_t borderColor, UIRectangle borderSize) {
	UIDrawBorder(painter, r, borderColor, borderSize);
	UIDrawBlock(painter, UI_RECT_4(r.l + borderSize.l, r.r - borderSize.r, r.t + borderSize.t, r.b - borderSize.b), mainColor);
}
