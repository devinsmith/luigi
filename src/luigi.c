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

struct ui_ctx ui;

void UIElementRefresh(UIElement *element) {
	UIElementMessage(element, UI_MSG_LAYOUT, 0, 0);
	UIElementRepaint(element, NULL);
}

void UIElementRepaint(UIElement *element, UIRectangle *region) {
	if (!region) {
		region = &element->bounds;
	}

	UIRectangle r = UIRectangleIntersection(*region, element->clip);

	if (!UI_RECT_VALID(r)) {
		return;
	}

	if (UI_RECT_VALID(element->window->updateRegion)) {
		element->window->updateRegion = UIRectangleBounding(element->window->updateRegion, r);
	} else {
		element->window->updateRegion = r;
	}
}

bool UIElementAnimate(UIElement *element, bool stop) {
	if (stop) {
		if (ui.animating != element) {
			return false;
		}

		ui.animating = NULL;
	} else {
		if (ui.animating && ui.animating != element) {
			return false;
		}

		ui.animating = element;
	}

	return true;
}

uint64_t UIAnimateClock() {
	return (uint64_t) UI_CLOCK() * 1000 / UI_CLOCKS_PER_SECOND;
}

void _UIElementDestroyDescendents(UIElement *element, bool topLevel) {
	UIElement *child = element->children;

	while (child) {
		if (!topLevel || (~child->flags & UI_ELEMENT_NON_CLIENT)) {
			UIElementDestroy(child);
		}

		child = child->next;
	}

#ifdef UI_DEBUG
	_UIInspectorRefresh();
#endif
}

void UIElementDestroyDescendents(UIElement *element) {
	_UIElementDestroyDescendents(element, true);
}

void UIElementDestroy(UIElement *element) {
	if (element->flags & UI_ELEMENT_DESTROY) {
		return;
	}

	element->flags |= UI_ELEMENT_DESTROY | UI_ELEMENT_HIDE;

	UIElement *ancestor = element->parent;

	while (ancestor) {
		ancestor->flags |= UI_ELEMENT_DESTROY_DESCENDENT;
		ancestor = ancestor->parent;
	}

	_UIElementDestroyDescendents(element, false);
}

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

void UIElementMove(UIElement *element, UIRectangle bounds, bool alwaysLayout) {
	UIRectangle oldClip = element->clip;
	element->clip = UIRectangleIntersection(element->parent->clip, bounds);

	if (!UIRectangleEquals(element->bounds, bounds) || !UIRectangleEquals(element->clip, oldClip) || alwaysLayout) {
		element->bounds = bounds;
		UIElementMessage(element, UI_MSG_LAYOUT, 0, 0);
	}
}

int UIElementMessage(UIElement *element, UIMessage message, int di, void *dp) {
	if (message != UI_MSG_DESTROY && (element->flags & UI_ELEMENT_DESTROY)) {
		return 0;
	}

	if (message >= UI_MSG_INPUT_EVENTS_START && message <= UI_MSG_INPUT_EVENTS_END && (element->flags & UI_ELEMENT_DISABLED)) {
		return 0;
	}

	if (element->messageUser) {
		int result = element->messageUser(element, message, di, dp);

		if (result) {
			return result;
		}
	}

	if (element->messageClass) {
		return element->messageClass(element, message, di, dp);
	} else {
		return 0;
	}
}

void UIElementChangeParent(UIElement *element, UIElement *newParent, UIElement *insertBefore) {
	UIElement **link = &element->parent->children;

	while (true) {
		if (*link == element) {
			*link = element->next;
			break;
		} else {
			link = &(*link)->next;
		}
	}

	link = &newParent->children;
	element->next = insertBefore;

	while (true) {
		if ((*link) == insertBefore) {
			*link = element;
			break;
		} else {
			link = &(*link)->next;
		}
	}

	element->parent = newParent;
	element->window = newParent->window;
}

UIElement *UIElementCreate(size_t bytes, UIElement *parent, uint32_t flags, int (*message)(UIElement *, UIMessage, int, void *), const char *cClassName) {
	UI_ASSERT(bytes >= sizeof(UIElement));
	UIElement *element = (UIElement *) UI_CALLOC(bytes);
	element->flags = flags;
	element->messageClass = message;

	if (!parent && (~flags & UI_ELEMENT_WINDOW)) {
		UI_ASSERT(ui.parentStackCount);
		parent = ui.parentStack[ui.parentStackCount - 1];
	}

	if ((~flags & UI_ELEMENT_NON_CLIENT) && parent) {
		UIElementMessage(parent, UI_MSG_CLIENT_PARENT, 0, &parent);
	}

	if (parent) {
		element->window = parent->window;
		element->parent = parent;

		if (parent->children) {
			UIElement *sibling = parent->children;

			while (sibling->next) {
				sibling = sibling->next;
			}

			sibling->next = element;
		} else {
			parent->children = element;
		}

		UI_ASSERT(~parent->flags & UI_ELEMENT_DESTROY);
	}

	element->cClassName = cClassName;
	static uint32_t id = 0;
	element->id = ++id;

#ifdef UI_DEBUG
	_UIInspectorRefresh();
#endif

	if (flags & UI_ELEMENT_PARENT_PUSH) {
		UIParentPush(element);
	}

	return element;
}

UIElement *UIParentPush(UIElement *element) {
	UI_ASSERT(ui.parentStackCount != sizeof(ui.parentStack) / sizeof(ui.parentStack[0]));
	ui.parentStack[ui.parentStackCount++] = element;
	return element;
}

UIElement *UIParentPop() {
	UI_ASSERT(ui.parentStackCount);
	ui.parentStackCount--;
	return ui.parentStack[ui.parentStackCount];
}
bool _UIMenusClose() {
	UIWindow *window = ui.windows;
	bool anyClosed = false;

	while (window) {
		if (window->e.flags & UI_WINDOW_MENU) {
			UIElementDestroy(&window->e);
			anyClosed = true;
		}

		window = window->next;
	}

	return anyClosed;
}

int _UIMenuItemMessage(UIElement *element, UIMessage message, int di, void *dp) {
	if (message == UI_MSG_CLICKED) {
		_UIMenusClose();
	}

	return 0;
}

int _UIMenuMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIMenu *menu = (UIMenu *) element;

	if (message == UI_MSG_GET_WIDTH) {
		UIElement *child = element->children;
		int width = 0;

		while (child) {
			if (~child->flags & UI_ELEMENT_NON_CLIENT) {
				int w = UIElementMessage(child, UI_MSG_GET_WIDTH, 0, 0);
				if (w > width) width = w;
			}

			child = child->next;
		}

		return width + 4 + UI_SIZE_SCROLL_BAR;
	} else if (message == UI_MSG_GET_HEIGHT) {
		UIElement *child = element->children;
		int height = 0;

		while (child) {
			if (~child->flags & UI_ELEMENT_NON_CLIENT) {
				height += UIElementMessage(child, UI_MSG_GET_HEIGHT, 0, 0);
			}

			child = child->next;
		}

		return height + 4;
	} else if (message == UI_MSG_PAINT) {
		UIDrawBlock((UIPainter *) dp, element->bounds, ui.theme->border);
	} else if (message == UI_MSG_LAYOUT) {
		UIElement *child = element->children;
		int position = element->bounds.t + 2 - menu->vScroll->position;
		int totalHeight = 0;
		int scrollBarSize = (menu->e.flags & UI_MENU_NO_SCROLL) ? 0 : UI_SIZE_SCROLL_BAR;

		while (child) {
			if (~child->flags & UI_ELEMENT_NON_CLIENT) {
				int height = UIElementMessage(child, UI_MSG_GET_HEIGHT, 0, 0);
				UIElementMove(child, UI_RECT_4(element->bounds.l + 2, element->bounds.r - scrollBarSize - 2, 
							position, position + height), false);
				position += height;
				totalHeight += height;
			}

			child = child->next;
		}

		UIRectangle scrollBarBounds = element->bounds;
		scrollBarBounds.l = scrollBarBounds.r - scrollBarSize * element->window->scale;
		menu->vScroll->maximum = totalHeight;
		menu->vScroll->page = UI_RECT_HEIGHT(element->bounds);
		UIElementMove(&menu->vScroll->e, scrollBarBounds, true);
	} else if (message == UI_MSG_KEY_TYPED) {
		UIKeyTyped *m = (UIKeyTyped *) dp;

		if (m->code == UI_KEYCODE_ESCAPE) {
			_UIMenusClose();
			return 1;
		}
	} else if (message == UI_MSG_MOUSE_WHEEL) {
		return UIElementMessage(&menu->vScroll->e, message, di, dp);
	} else if (message == UI_MSG_SCROLLED) {
		UIElementRefresh(element);
	}

	return 0;
}

void UIMenuAddItem(UIMenu *menu, uint32_t flags, const char *label, ptrdiff_t labelBytes, void (*invoke)(void *cp), void *cp) {
	UIButton *button = UIButtonCreate(&menu->e, flags | UI_BUTTON_MENU_ITEM, label, labelBytes);
	button->invoke = invoke;
	button->e.messageUser = _UIMenuItemMessage;
	button->e.cp = cp;
}

void _UIMenuPrepare(UIMenu *menu, int *width, int *height) {
	*width = UIElementMessage(&menu->e, UI_MSG_GET_WIDTH, 0, 0);
	*height = UIElementMessage(&menu->e, UI_MSG_GET_HEIGHT, 0, 0);

	if (menu->e.flags & UI_MENU_PLACE_ABOVE) {
		menu->pointY -= *height;
	}
}

UIMenu *UIMenuCreate(UIElement *parent, uint32_t flags) {
	UIWindow *window = UIWindowCreate(parent->window, UI_WINDOW_MENU, 0, 0, 0);
	UIMenu *menu = (UIMenu *) UIElementCreate(sizeof(UIMenu), &window->e, flags, _UIMenuMessage, "Menu");
	menu->vScroll = UIScrollBarCreate(&menu->e, UI_ELEMENT_NON_CLIENT);

	if (parent->parent) {
		UIRectangle screenBounds = UIElementScreenBounds(parent);
		menu->pointX = screenBounds.l;
		menu->pointY = (flags & UI_MENU_PLACE_ABOVE) ? (screenBounds.t + 1) : (screenBounds.b - 1);
	} else {
		int x = 0, y = 0;
		_UIWindowGetScreenPosition(parent->window, &x, &y);

		menu->pointX = parent->window->cursorX + x;
		menu->pointY = parent->window->cursorY + y;
	}

	return menu;
}

UIRectangle UIElementScreenBounds(UIElement *element) {
	int x = 0, y = 0;
	_UIWindowGetScreenPosition(element->window, &x, &y);
	return UIRectangleAdd(element->bounds, UI_RECT_2(x, y));
}

void UIWindowRegisterShortcut(UIWindow *window, UIShortcut shortcut) {
	if (window->shortcutCount + 1 > window->shortcutAllocated) {
		window->shortcutAllocated = (window->shortcutCount + 1) * 2;
		window->shortcuts = (UIShortcut *) UI_REALLOC(window->shortcuts, window->shortcutAllocated * sizeof(UIShortcut));
	}

	window->shortcuts[window->shortcutCount++] = shortcut;
}

void _UIElementPaint(UIElement *element, UIPainter *painter) {
	if (element->flags & UI_ELEMENT_HIDE) {
		return;
	}

	// Clip painting to the element's clip.

	painter->clip = UIRectangleIntersection(element->clip, painter->clip);

	if (!UI_RECT_VALID(painter->clip)) {
		return;
	}

	// Paint the element.

	UIElementMessage(element, UI_MSG_PAINT, 0, painter);

	// Paint its children.

	UIElement *child = element->children;
	UIRectangle previousClip = painter->clip;

	while (child) {
		painter->clip = previousClip;
		_UIElementPaint(child, painter);
		child = child->next;
	}
}

void UIElementFocus(UIElement *element) {
	UIElement *previous = element->window->focused;
	if (previous == element) return;
	element->window->focused = element;
	if (previous) UIElementMessage(previous, UI_MSG_UPDATE, UI_UPDATE_FOCUSED, 0);
	if (element) UIElementMessage(element, UI_MSG_UPDATE, UI_UPDATE_FOCUSED, 0);

#ifdef UI_DEBUG
	_UIInspectorRefresh();
#endif
}

void _UIWindowSetPressed(UIWindow *window, UIElement *element, int button) {
	UIElement *previous = window->pressed;
	window->pressed = element;
	window->pressedButton = button;
	if (previous) UIElementMessage(previous, UI_MSG_UPDATE, UI_UPDATE_PRESSED, 0);
	if (element) UIElementMessage(element, UI_MSG_UPDATE, UI_UPDATE_PRESSED, 0);

	UIElement *ancestor = element;
	UIElement *child = NULL;

	while (ancestor) {
		UIElementMessage(ancestor, UI_MSG_PRESSED_DESCENDENT, 0, child);
		child = ancestor;
		ancestor = ancestor->parent;
	}
}

bool _UIDestroy(UIElement *element) {
	if (element->flags & UI_ELEMENT_DESTROY_DESCENDENT) {
		element->flags &= ~UI_ELEMENT_DESTROY_DESCENDENT;

		UIElement *child = element->children;
		UIElement **link = &element->children;

		while (child) {
			UIElement *next = child->next;

			if (_UIDestroy(child)) {
				*link = next;
			} else {
				link = &child->next;
			}

			child = next;
		}
	}

	if (element->flags & UI_ELEMENT_DESTROY) {
		UIElementMessage(element, UI_MSG_DESTROY, 0, 0);

		if (element->window->pressed == element) {
			_UIWindowSetPressed(element->window, NULL, 0);
		}

		if (element->window->hovered == element) {
			element->window->hovered = &element->window->e;
		}

		if (element->window->focused == element) {
			element->window->focused = NULL;
		}

		if (element->window->dialogOldFocus == element) {
			element->window->dialogOldFocus = NULL;
		}

		if (ui.animating == element) {
			ui.animating = NULL;
		}

		UI_FREE(element);
		return true;
	} else {
		return false;
	}
}

void _UIUpdate() {
	UIWindow *window = ui.windows;
	UIWindow **link = &ui.windows;

	while (window) {
		UIWindow *next = window->next;

		if (_UIDestroy(&window->e)) {
			*link = next;
		} else {
			link = &window->next;

			if (UI_RECT_VALID(window->updateRegion)) {
#ifdef __cplusplus
				UIPainter painter = {};
#else
				UIPainter painter = { 0 };
#endif
				painter.bits = window->bits;
				painter.width = window->width;
				painter.height = window->height;
				painter.clip = UIRectangleIntersection(UI_RECT_2S(window->width, window->height), window->updateRegion);
				_UIElementPaint(&window->e, &painter);
				_UIWindowEndPaint(window, &painter);
				window->updateRegion = UI_RECT_1(0);

#ifdef UI_DEBUG
				window->lastFullFillCount = (float) painter.fillCount / (UI_RECT_WIDTH(window->updateRegion) * UI_RECT_HEIGHT(window->updateRegion));
#endif
			}
		}

		window = next;
	}
}

UIElement *UIElementFindByPoint(UIElement *element, int x, int y) {
	UIFindByPoint m = { 0 };
	m.x = x, m.y = y;

	if (UIElementMessage(element, UI_MSG_FIND_BY_POINT, 0, &m)) {
		return m.result ? m.result : element;
	}

	UIElement *child = element->children;

	while (child) {
		if ((~child->flags & UI_ELEMENT_HIDE) && UIRectangleContains(child->clip, x, y)) {
			return UIElementFindByPoint(child, x, y);
		}

		child = child->next;
	}

	return element;
}

void _UIProcessAnimations() {
	if (ui.animating) {
		UIElementMessage(ui.animating, UI_MSG_ANIMATE, 0, 0);
		_UIUpdate();
	}
}

bool _UIMenusOpen() {
	UIWindow *window = ui.windows;

	while (window) {
		if (window->e.flags & UI_WINDOW_MENU) {
			return true;
		}

		window = window->next;
	}

	return false;
}

void _UIWindowDestroyCommon(UIWindow *window) {
	UI_FREE(window->bits);
	UI_FREE(window->shortcuts);
}

UIElement *_UIElementLastChild(UIElement *element) {
	if (!element->children) {
		return NULL;
	}

	UIElement *child = element->children;

	while (child->next) {
		child = child->next;
	}

	return child;
}

UIElement *_UIElementPreviousSibling(UIElement *element) {
	if (!element->parent) {
		return NULL;
	}

	UIElement *sibling = element->parent->children;

	if (sibling == element) {
		return NULL;
	}

	while (sibling->next != element) {
		sibling = sibling->next;
		UI_ASSERT(sibling);
	}
	
	return sibling;
}

bool _UIWindowInputEvent(UIWindow *window, UIMessage message, int di, void *dp) {
	bool handled = true;

	if (window->pressed) {
		if (message == UI_MSG_MOUSE_MOVE) {
			UIElementMessage(window->pressed, UI_MSG_MOUSE_DRAG, di, dp);
		} else if (message == UI_MSG_LEFT_UP && window->pressedButton == 1) {
			if (window->hovered == window->pressed) {
				UIElementMessage(window->pressed, UI_MSG_CLICKED, di, dp);
				if (ui.quit || ui.dialogResult) goto end;
			}

			if (window->pressed) {
				UIElementMessage(window->pressed, UI_MSG_LEFT_UP, di, dp);
				if (ui.quit || ui.dialogResult) goto end;
				_UIWindowSetPressed(window, NULL, 1);
			}
		} else if (message == UI_MSG_MIDDLE_UP && window->pressedButton == 2) {
			UIElementMessage(window->pressed, UI_MSG_MIDDLE_UP, di, dp);
			if (ui.quit || ui.dialogResult) goto end;
			_UIWindowSetPressed(window, NULL, 2);
		} else if (message == UI_MSG_RIGHT_UP && window->pressedButton == 3) {
			UIElementMessage(window->pressed, UI_MSG_RIGHT_UP, di, dp);
			if (ui.quit || ui.dialogResult) goto end;
			_UIWindowSetPressed(window, NULL, 3);
		}
	}

	if (window->pressed) {
		bool inside = UIRectangleContains(window->pressed->clip, window->cursorX, window->cursorY);

		if (inside && window->hovered == &window->e) {
			window->hovered = window->pressed;
			UIElementMessage(window->pressed, UI_MSG_UPDATE, UI_UPDATE_HOVERED, 0);
		} else if (!inside && window->hovered == window->pressed) {
			window->hovered = &window->e;
			UIElementMessage(window->pressed, UI_MSG_UPDATE, UI_UPDATE_HOVERED, 0);
		}

		if (ui.quit || ui.dialogResult) goto end;
	}

	if (!window->pressed) {
		UIElement *hovered = UIElementFindByPoint(&window->e, window->cursorX, window->cursorY);

		if (message == UI_MSG_MOUSE_MOVE) {
			UIElementMessage(hovered, UI_MSG_MOUSE_MOVE, di, dp);

			int cursor = UIElementMessage(window->hovered, UI_MSG_GET_CURSOR, di, dp);

			if (cursor != window->cursorStyle) {
				window->cursorStyle = cursor;
				_UIWindowSetCursor(window, cursor);
			}
		} else if (message == UI_MSG_LEFT_DOWN) {
			if ((window->e.flags & UI_WINDOW_MENU) || !_UIMenusClose()) {
				_UIWindowSetPressed(window, hovered, 1);
				UIElementMessage(hovered, UI_MSG_LEFT_DOWN, di, dp);
			}
		} else if (message == UI_MSG_MIDDLE_DOWN) {
			if ((window->e.flags & UI_WINDOW_MENU) || !_UIMenusClose()) {
				_UIWindowSetPressed(window, hovered, 2);
				UIElementMessage(hovered, UI_MSG_MIDDLE_DOWN, di, dp);
			}
		} else if (message == UI_MSG_RIGHT_DOWN) {
			if ((window->e.flags & UI_WINDOW_MENU) || !_UIMenusClose()) {
				_UIWindowSetPressed(window, hovered, 3);
				UIElementMessage(hovered, UI_MSG_RIGHT_DOWN, di, dp);
			}
		} else if (message == UI_MSG_MOUSE_WHEEL) {
			UIElement *element = hovered;

			while (element) {
				if (UIElementMessage(element, UI_MSG_MOUSE_WHEEL, di, dp)) {
					break;
				}

				element = element->parent;
			}
		} else if (message == UI_MSG_KEY_TYPED) {
			handled = false;

			if (window->focused) {
				UIElement *element = window->focused;

				while (element) {
					if (UIElementMessage(element, UI_MSG_KEY_TYPED, di, dp)) {
						handled = true;
						break;
					}

					element = element->parent;
				}
			} else {
				if (UIElementMessage(&window->e, UI_MSG_KEY_TYPED, di, dp)) {
					handled = true;
				}
			}

			if (!handled && !_UIMenusOpen()) {
				UIKeyTyped *m = (UIKeyTyped *) dp;

				if (m->code == UI_KEYCODE_TAB && !window->ctrl && !window->alt) {
					UIElement *start = window->focused ? window->focused : &window->e;
					UIElement *element = start;

					do {
						if (element->children && !(element->flags & (UI_ELEMENT_HIDE | UI_ELEMENT_DISABLED))) {
							element = window->shift ? _UIElementLastChild(element) : element->children;
							continue;
						} 

						while (element) {
							if (window->shift ? (element->parent && element->parent->children != element) : !!element->next) {
								element = window->shift ? _UIElementPreviousSibling(element) : element->next;
								break;
							} else {
								element = element->parent;
							}
						}

						if (!element) {
							element = &window->e;
						}
					} while (element != start && ((~element->flags & UI_ELEMENT_TAB_STOP) 
						|| (element->flags & (UI_ELEMENT_HIDE | UI_ELEMENT_DISABLED))));

					if (~element->flags & UI_ELEMENT_WINDOW) {
						UIElementFocus(element);
					}

					handled = true;
				} else if (!window->dialog) {
					for (intptr_t i = window->shortcutCount - 1; i >= 0; i--) {
						UIShortcut *shortcut = window->shortcuts + i;

						if (shortcut->code == m->code && shortcut->ctrl == window->ctrl 
								&& shortcut->shift == window->shift && shortcut->alt == window->alt) {
							shortcut->invoke(shortcut->cp);
							handled = true;
							break;
						}
					}
				}
			}
		}

		if (ui.quit || ui.dialogResult) goto end;

		if (hovered != window->hovered) {
			UIElement *previous = window->hovered;
			window->hovered = hovered;
			UIElementMessage(previous, UI_MSG_UPDATE, UI_UPDATE_HOVERED, 0);
			UIElementMessage(window->hovered, UI_MSG_UPDATE, UI_UPDATE_HOVERED, 0);
		}
	}

	end: _UIUpdate();
	return handled;
}


void _UIInitialiseCommon() {
  UISetDarkTheme();

#ifdef UI_FREETYPE
	FT_Init_FreeType(&ui.ft);
	UIFontActivate(UIFontCreate(_UI_TO_STRING_2(UI_FONT_PATH), 11));
#else
	UIFontActivate(UIFontCreate(0, 0));
#endif
}

void _UIWindowAdd(UIWindow *window) {
	window->scale = 1.0f;
	window->e.window = window;
	window->hovered = &window->e;
	window->next = ui.windows;
	ui.windows = window;
}

int _UIWindowMessageCommon(UIElement *element, UIMessage message, int di, void *dp) {
	if (message == UI_MSG_LAYOUT && element->children) {
		UIElementMove(element->children, element->bounds, false);
		if (element->window->dialog) UIElementMove(element->window->dialog, element->bounds, false);
		UIElementRepaint(element, NULL);
	} else if (message == UI_MSG_FIND_BY_POINT) {
		UIFindByPoint *m = (UIFindByPoint *) dp;
		if (element->window->dialog) m->result = UIElementFindByPoint(element->window->dialog, m->x, m->y);
		else if (!element->children) m->result = NULL;
		else m->result = UIElementFindByPoint(element->children, m->x, m->y);
		return 1;
	}

	return 0;
}

int UIMessageLoop() {
	_UIInspectorCreate();
	_UIUpdate();
#ifdef UI_AUTOMATION_TESTS
	return UIAutomationRunTests();
#else
	int result = 0;
	while (!ui.quit && _UIMessageLoopSingle(&result)) ui.dialogResult = NULL;
	return result;
#endif
}

#ifdef UI_LINUX

const int UI_KEYCODE_A = XK_a;
const int UI_KEYCODE_BACKSPACE = XK_BackSpace;
const int UI_KEYCODE_DELETE = XK_Delete;
const int UI_KEYCODE_DOWN = XK_Down;
const int UI_KEYCODE_END = XK_End;
const int UI_KEYCODE_ENTER = XK_Return;
const int UI_KEYCODE_ESCAPE = XK_Escape;
const int UI_KEYCODE_F1 = XK_F1;
const int UI_KEYCODE_HOME = XK_Home;
const int UI_KEYCODE_LEFT = XK_Left;
const int UI_KEYCODE_RIGHT = XK_Right;
const int UI_KEYCODE_SPACE = XK_space;
const int UI_KEYCODE_TAB = XK_Tab;
const int UI_KEYCODE_UP = XK_Up;
const int UI_KEYCODE_INSERT = XK_Insert;
const int UI_KEYCODE_0 = XK_0;

int _UIWindowMessage(UIElement *element, UIMessage message, int di, void *dp) {
	if (message == UI_MSG_DESTROY) {
		UIWindow *window = (UIWindow *) element;
		_UIWindowDestroyCommon(window);
		window->image->data = NULL;
		XDestroyImage(window->image);
		XDestroyIC(window->xic);
		XDestroyWindow(ui.display, ((UIWindow *) element)->window);
	}

	return _UIWindowMessageCommon(element, message, di, dp);
}

UIWindow *UIWindowCreate(UIWindow *owner, uint32_t flags, const char *cTitle, int _width, int _height) {
	_UIMenusClose();

	UIWindow *window = (UIWindow *) UIElementCreate(sizeof(UIWindow), NULL, flags | UI_ELEMENT_WINDOW, _UIWindowMessage, "Window");
	_UIWindowAdd(window);
	if (owner) window->scale = owner->scale;

	int width = (flags & UI_WINDOW_MENU) ? 1 : _width ? _width : 800;
	int height = (flags & UI_WINDOW_MENU) ? 1 : _height ? _height : 600;

	XSetWindowAttributes attributes = {};
	attributes.override_redirect = flags & UI_WINDOW_MENU;

	window->window = XCreateWindow(ui.display, DefaultRootWindow(ui.display), 0, 0, width, height, 0, 0, 
		InputOutput, CopyFromParent, CWOverrideRedirect, &attributes);
	if (cTitle) XStoreName(ui.display, window->window, cTitle);
	XSelectInput(ui.display, window->window, SubstructureNotifyMask | ExposureMask | PointerMotionMask 
		| ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask
		| EnterWindowMask | LeaveWindowMask | ButtonMotionMask | KeymapStateMask | FocusChangeMask | PropertyChangeMask);

	if (flags & UI_WINDOW_MAXIMIZE) {
		Atom atoms[2] = { XInternAtom(ui.display, "_NET_WM_STATE_MAXIMIZED_HORZ", 0), XInternAtom(ui.display, "_NET_WM_STATE_MAXIMIZED_VERT", 0) };
		XChangeProperty(ui.display, window->window, XInternAtom(ui.display, "_NET_WM_STATE", 0), XA_ATOM, 32, PropModeReplace, (unsigned char *) atoms, 2);
	}

	if (~flags & UI_WINDOW_MENU) {
		XMapRaised(ui.display, window->window);
	}

	if (flags & UI_WINDOW_CENTER_IN_OWNER) {
		int x = 0, y = 0;
		_UIWindowGetScreenPosition(owner, &x, &y);
		XMoveResizeWindow(ui.display, window->window, x + owner->width / 2 - width / 2, y + owner->height / 2 - height / 2, width, height);
	}

	XSetWMProtocols(ui.display, window->window, &ui.windowClosedID, 1);
	window->image = XCreateImage(ui.display, ui.visual, 24, ZPixmap, 0, NULL, 10, 10, 32, 0);

	window->xic = XCreateIC(ui.xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, window->window, XNFocusWindow, window->window, NULL);

	int dndVersion = 4;
	XChangeProperty(ui.display, window->window, ui.dndAwareID, XA_ATOM, 32 /* bits */, PropModeReplace, (uint8_t *) &dndVersion, 1);

	return window;
}

Display *_UIX11GetDisplay() {
	return ui.display;
}

UIWindow *X11FindWindow(Window window) {
	UIWindow *w = ui.windows;

	while (w) {
		if (w->window == window) {
			return w;
		}

		w = w->next;
	}

	return NULL;
}

void UIInitialise() {
	_UIInitialiseCommon();

	XInitThreads();

	ui.display = XOpenDisplay(NULL);
	ui.visual = XDefaultVisual(ui.display, 0);

	ui.windowClosedID = XInternAtom(ui.display, "WM_DELETE_WINDOW", 0);
	ui.primaryID = XInternAtom(ui.display, "PRIMARY", 0);
	ui.dndEnterID = XInternAtom(ui.display, "XdndEnter", 0);
	ui.dndPositionID = XInternAtom(ui.display, "XdndPosition", 0);
	ui.dndStatusID = XInternAtom(ui.display, "XdndStatus", 0);
	ui.dndActionCopyID = XInternAtom(ui.display, "XdndActionCopy", 0);
	ui.dndDropID = XInternAtom(ui.display, "XdndDrop", 0);
	ui.dndSelectionID = XInternAtom(ui.display, "XdndSelection", 0);
	ui.dndFinishedID = XInternAtom(ui.display, "XdndFinished", 0);
	ui.dndAwareID = XInternAtom(ui.display, "XdndAware", 0);
	ui.uriListID = XInternAtom(ui.display, "text/uri-list", 0);
	ui.plainTextID = XInternAtom(ui.display, "text/plain", 0);
	ui.clipboardID = XInternAtom(ui.display, "CLIPBOARD", 0);
	ui.xSelectionDataID = XInternAtom(ui.display, "XSEL_DATA", 0);
	ui.textID = XInternAtom(ui.display, "TEXT", 0);
	ui.targetID = XInternAtom(ui.display, "TARGETS", 0);
	ui.incrID = XInternAtom(ui.display, "INCR", 0);

	ui.cursors[UI_CURSOR_ARROW] = XCreateFontCursor(ui.display, XC_left_ptr);
	ui.cursors[UI_CURSOR_TEXT] = XCreateFontCursor(ui.display, XC_xterm);
	ui.cursors[UI_CURSOR_SPLIT_V] = XCreateFontCursor(ui.display, XC_sb_v_double_arrow);
	ui.cursors[UI_CURSOR_SPLIT_H] = XCreateFontCursor(ui.display, XC_sb_h_double_arrow);
	ui.cursors[UI_CURSOR_FLIPPED_ARROW] = XCreateFontCursor(ui.display, XC_right_ptr);
	ui.cursors[UI_CURSOR_CROSS_HAIR] = XCreateFontCursor(ui.display, XC_crosshair);
	ui.cursors[UI_CURSOR_HAND] = XCreateFontCursor(ui.display, XC_hand1);
	ui.cursors[UI_CURSOR_RESIZE_UP] = XCreateFontCursor(ui.display, XC_top_side);
	ui.cursors[UI_CURSOR_RESIZE_LEFT] = XCreateFontCursor(ui.display, XC_left_side);
	ui.cursors[UI_CURSOR_RESIZE_UP_RIGHT] = XCreateFontCursor(ui.display, XC_top_right_corner);
	ui.cursors[UI_CURSOR_RESIZE_UP_LEFT] = XCreateFontCursor(ui.display, XC_top_left_corner);
	ui.cursors[UI_CURSOR_RESIZE_DOWN] = XCreateFontCursor(ui.display, XC_bottom_side);
	ui.cursors[UI_CURSOR_RESIZE_RIGHT] = XCreateFontCursor(ui.display, XC_right_side);
	ui.cursors[UI_CURSOR_RESIZE_DOWN_LEFT] = XCreateFontCursor(ui.display, XC_bottom_left_corner);
	ui.cursors[UI_CURSOR_RESIZE_DOWN_RIGHT] = XCreateFontCursor(ui.display, XC_bottom_right_corner);

	XSetLocaleModifiers("");

	ui.xim = XOpenIM(ui.display, 0, 0, 0);

	if(!ui.xim){
		XSetLocaleModifiers("@im=none");
		ui.xim = XOpenIM(ui.display, 0, 0, 0);
	}
}

void _UIWindowSetCursor(UIWindow *window, int cursor) {
	XDefineCursor(ui.display, window->window, ui.cursors[cursor]);
}

void _UIX11ResetCursor(UIWindow *window) {
	XDefineCursor(ui.display, window->window, ui.cursors[UI_CURSOR_ARROW]);
}

void _UIWindowEndPaint(UIWindow *window, UIPainter *painter) {
	(void) painter;

	XPutImage(ui.display, window->window, DefaultGC(ui.display, 0), window->image, 
		UI_RECT_TOP_LEFT(window->updateRegion), UI_RECT_TOP_LEFT(window->updateRegion),
		UI_RECT_SIZE(window->updateRegion));
}

void _UIWindowGetScreenPosition(UIWindow *window, int *_x, int *_y) {
	Window child;
	XTranslateCoordinates(ui.display, window->window, DefaultRootWindow(ui.display), 0, 0, _x, _y, &child);
}

void UIMenuShow(UIMenu *menu) {
	int width, height;
	_UIMenuPrepare(menu, &width, &height);

	for (int i = 0; i < ScreenCount(ui.display); i++) {
		Screen *screen = ScreenOfDisplay(ui.display, i);

		int x, y;
		Window child;
		XTranslateCoordinates(ui.display, screen->root, DefaultRootWindow(ui.display), 0, 0, &x, &y, &child);

		if (menu->pointX >= x && menu->pointX < x + screen->width 
				&& menu->pointY >= y && menu->pointY < y + screen->height) {
			if (menu->pointX + width > x + screen->width) menu->pointX = x + screen->width - width;
			if (menu->pointY + height > y + screen->height) menu->pointY = y + screen->height - height;
			if (menu->pointX < x) menu->pointX = x;
			if (menu->pointY < y) menu->pointY = y;
			if (menu->pointX + width > x + screen->width) width = x + screen->width - menu->pointX;
			if (menu->pointY + height > y + screen->height) height = y + screen->height - menu->pointY;
			break;
		}
	}

	Atom properties[] = {
		XInternAtom(ui.display, "_NET_WM_WINDOW_TYPE", true),
		XInternAtom(ui.display, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", true),
		XInternAtom(ui.display, "_MOTIF_WM_HINTS", true),
	};

	XChangeProperty(ui.display, menu->e.window->window, properties[0], XA_ATOM, 32, PropModeReplace, (uint8_t *) properties, 2);
	XSetTransientForHint(ui.display, menu->e.window->window, DefaultRootWindow(ui.display));

	struct Hints {
		int flags;
		int functions;
		int decorations;
		int inputMode;
		int status;
	};

	struct Hints hints = { 0 };
	hints.flags = 2;
	XChangeProperty(ui.display, menu->e.window->window, properties[2], properties[2], 32, PropModeReplace, (uint8_t *) &hints, 5);

	XMapWindow(ui.display, menu->e.window->window);
	XMoveResizeWindow(ui.display, menu->e.window->window, menu->pointX, menu->pointY, width, height);
}

void UIWindowPack(UIWindow *window, int _width) {
	int width = _width ? _width : UIElementMessage(window->e.children, UI_MSG_GET_WIDTH, 0, 0);
	int height = UIElementMessage(window->e.children, UI_MSG_GET_HEIGHT, width, 0);
	XResizeWindow(ui.display, window->window, width, height);
}

bool _UIProcessEvent(XEvent *event) {
	if (event->type == ClientMessage && (Atom) event->xclient.data.l[0] == ui.windowClosedID) {
		UIWindow *window = X11FindWindow(event->xclient.window);
		if (!window) return false;
		bool exit = !UIElementMessage(&window->e, UI_MSG_WINDOW_CLOSE, 0, 0);
		if (exit) return true;
		_UIUpdate();
		return false;
	} else if (event->type == Expose) {
		UIWindow *window = X11FindWindow(event->xexpose.window);
		if (!window) return false;
		XPutImage(ui.display, window->window, DefaultGC(ui.display, 0), window->image, 0, 0, 0, 0, window->width, window->height);
	} else if (event->type == ConfigureNotify) {
		UIWindow *window = X11FindWindow(event->xconfigure.window);
		if (!window) return false;

		if (window->width != event->xconfigure.width || window->height != event->xconfigure.height) {
			window->width = event->xconfigure.width;
			window->height = event->xconfigure.height;
			window->bits = (uint32_t *) UI_REALLOC(window->bits, window->width * window->height * 4);
			window->image->width = window->width;
			window->image->height = window->height;
			window->image->bytes_per_line = window->width * 4;
			window->image->data = (char *) window->bits;
			window->e.bounds = UI_RECT_2S(window->width, window->height);
			window->e.clip = UI_RECT_2S(window->width, window->height);
#ifdef UI_DEBUG
			for (int i = 0; i < window->width * window->height; i++) window->bits[i] = 0xFF00FF;
#endif
			UIElementMessage(&window->e, UI_MSG_LAYOUT, 0, 0);
			_UIUpdate();
		}
	} else if (event->type == MotionNotify) {
		UIWindow *window = X11FindWindow(event->xmotion.window);
		if (!window) return false;
		window->cursorX = event->xmotion.x;
		window->cursorY = event->xmotion.y;
		_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
	} else if (event->type == LeaveNotify) {
		UIWindow *window = X11FindWindow(event->xcrossing.window);
		if (!window) return false;

		if (!window->pressed) {
			window->cursorX = -1;
			window->cursorY = -1;
		}

		_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
	} else if (event->type == ButtonPress || event->type == ButtonRelease) {
		UIWindow *window = X11FindWindow(event->xbutton.window);
		if (!window) return false;
		window->cursorX = event->xbutton.x;
		window->cursorY = event->xbutton.y;

		if (event->xbutton.button >= 1 && event->xbutton.button <= 3) {
			_UIWindowInputEvent(window, (UIMessage) ((event->type == ButtonPress ? UI_MSG_LEFT_DOWN : UI_MSG_LEFT_UP) 
				+ event->xbutton.button * 2 - 2), 0, 0);
		} else if (event->xbutton.button == 4) {
			_UIWindowInputEvent(window, UI_MSG_MOUSE_WHEEL, -72, 0);
		} else if (event->xbutton.button == 5) {
			_UIWindowInputEvent(window, UI_MSG_MOUSE_WHEEL, 72, 0);
		}

		_UIInspectorSetFocusedWindow(window);
	} else if (event->type == KeyPress) {
		UIWindow *window = X11FindWindow(event->xkey.window);
		if (!window) return false;

		if (event->xkey.x == 0x7123 && event->xkey.y == 0x7456) {
			// HACK! See UIWindowPostMessage.
			UIElementMessage(&window->e, (UIMessage) event->xkey.state, 0, 
				(void *) (((uintptr_t) (event->xkey.time & 0xFFFFFFFF) << 32) 
					| ((uintptr_t) (event->xkey.x_root & 0xFFFF) << 0) 
					| ((uintptr_t) (event->xkey.y_root & 0xFFFF) << 16)));
			_UIUpdate();
		} else {
			char text[32];
			KeySym symbol = NoSymbol;
			Status status;
			// printf("%ld, %s\n", symbol, text);
			UIKeyTyped m = { 0 };
			m.textBytes = Xutf8LookupString(window->xic, &event->xkey, text, sizeof(text) - 1, &symbol, &status); 
			m.text = text;
			m.code = XLookupKeysym(&event->xkey, 0);

			if (symbol == XK_Control_L || symbol == XK_Control_R) {
				window->ctrl = true;
				window->ctrlCode = event->xkey.keycode;
				_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
			} else if (symbol == XK_Shift_L || symbol == XK_Shift_R) {
				window->shift = true;
				window->shiftCode = event->xkey.keycode;
				_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
			} else if (symbol == XK_Alt_L || symbol == XK_Alt_R) {
				window->alt = true;
				window->altCode = event->xkey.keycode;
				_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
			} else if (symbol == XK_KP_Left) {
				m.code = UI_KEYCODE_LEFT;
			} else if (symbol == XK_KP_Right) {
				m.code = UI_KEYCODE_RIGHT;
			} else if (symbol == XK_KP_Up) {
				m.code = UI_KEYCODE_UP;
			} else if (symbol == XK_KP_Down) {
				m.code = UI_KEYCODE_DOWN;
			} else if (symbol == XK_KP_Home) {
				m.code = UI_KEYCODE_HOME;
			} else if (symbol == XK_KP_End) {
				m.code = UI_KEYCODE_END;
			} else if (symbol == XK_KP_Enter) {
				m.code = UI_KEYCODE_ENTER;
			} else if (symbol == XK_KP_Delete) {
				m.code = UI_KEYCODE_DELETE;
			}

			_UIWindowInputEvent(window, UI_MSG_KEY_TYPED, 0, &m);
		}
	} else if (event->type == KeyRelease) {
		UIWindow *window = X11FindWindow(event->xkey.window);
		if (!window) return false;

		if (event->xkey.keycode == window->ctrlCode) {
			window->ctrl = false;
			_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
		} else if (event->xkey.keycode == window->shiftCode) {
			window->shift = false;
			_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
		} else if (event->xkey.keycode == window->altCode) {
			window->alt = false;
			_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
		}
	} else if (event->type == FocusIn) {
		UIWindow *window = X11FindWindow(event->xfocus.window);
		if (!window) return false;
		window->ctrl = window->shift = window->alt = false;
		UIElementMessage(&window->e, UI_MSG_WINDOW_ACTIVATE, 0, 0);
	} else if (event->type == ClientMessage && event->xclient.message_type == ui.dndEnterID) {
		UIWindow *window = X11FindWindow(event->xclient.window);
		if (!window) return false;
		window->dragSource = (Window) event->xclient.data.l[0];
	} else if (event->type == ClientMessage && event->xclient.message_type == ui.dndPositionID) {
		UIWindow *window = X11FindWindow(event->xclient.window);
		if (!window) return false;
		XClientMessageEvent m = { 0 };
		m.type = ClientMessage;
		m.display = event->xclient.display;
		m.window = (Window) event->xclient.data.l[0];
		m.message_type = ui.dndStatusID;
		m.format = 32;
		m.data.l[0] = window->window;
		m.data.l[1] = true;
		m.data.l[4] = ui.dndActionCopyID;
		XSendEvent(ui.display, m.window, False, NoEventMask, (XEvent *) &m);
		XFlush(ui.display);
	} else if (event->type == ClientMessage && event->xclient.message_type == ui.dndDropID) {
		UIWindow *window = X11FindWindow(event->xclient.window);
		if (!window) return false;

		// TODO Dropping text.

		if (!XConvertSelection(ui.display, ui.dndSelectionID, ui.uriListID, ui.primaryID, window->window, event->xclient.data.l[2])) {
			XClientMessageEvent m = { 0 };
			m.type = ClientMessage;
			m.display = ui.display;
			m.window = window->dragSource;
			m.message_type = ui.dndFinishedID;
			m.format = 32;
			m.data.l[0] = window->window;
			m.data.l[1] = 0;
			m.data.l[2] = ui.dndActionCopyID;
			XSendEvent(ui.display, m.window, False, NoEventMask, (XEvent *) &m);
			XFlush(ui.display);
		}
	} else if (event->type == SelectionNotify) {
		UIWindow *window = X11FindWindow(event->xselection.requestor);
		if (!window) return false;
		if (!window->dragSource) return false;

		Atom type = None;
		int format = 0;
		uint64_t count = 0, bytesLeft = 0;
		uint8_t *data = NULL;
		XGetWindowProperty(ui.display, window->window, ui.primaryID, 0, 65536, False, AnyPropertyType, &type, &format, &count, &bytesLeft, &data);

		if (format == 8 /* bits per character */) {
			if (event->xselection.target == ui.uriListID) {
				char *copy = (char *) UI_MALLOC(count);
				int fileCount = 0;

				for (int i = 0; i < (int) count; i++) {
					copy[i] = data[i];

					if (i && data[i - 1] == '\r' && data[i] == '\n') {
						fileCount++;
					}
				}

				char **files = (char **) UI_MALLOC(sizeof(char *) * fileCount);
				fileCount = 0;

				for (int i = 0; i < (int) count; i++) {
					char *s = copy + i;
					while (!(i && data[i - 1] == '\r' && data[i] == '\n' && i < (int) count)) i++;
					copy[i - 1] = 0;

					for (int j = 0; s[j]; j++) {
						if (s[j] == '%' && s[j + 1] && s[j + 2]) {
							char n[3];
							n[0] = s[j + 1], n[1] = s[j + 2], n[2] = 0;
							s[j] = strtol(n, NULL, 16);
							if (!s[j]) break;
							memmove(s + j + 1, s + j + 3, strlen(s) - j - 2);
						}
					}

					if (s[0] == 'f' && s[1] == 'i' && s[2] == 'l' && s[3] == 'e' && s[4] == ':' && s[5] == '/' && s[6] == '/') {
						files[fileCount++] = s + 7;
					}
				}

				UIElementMessage(&window->e, UI_MSG_WINDOW_DROP_FILES, fileCount, files);

				UI_FREE(files);
				UI_FREE(copy);
			} else if (event->xselection.target == ui.plainTextID) {
				// TODO.
			}
		}

		XFree(data);

		XClientMessageEvent m = { 0 };
		m.type = ClientMessage;
		m.display = ui.display;
		m.window = window->dragSource;
		m.message_type = ui.dndFinishedID;
		m.format = 32;
		m.data.l[0] = window->window;
		m.data.l[1] = true;
		m.data.l[2] = ui.dndActionCopyID;
		XSendEvent(ui.display, m.window, False, NoEventMask, (XEvent *) &m);
		XFlush(ui.display);

		window->dragSource = 0; // Drag complete.
		_UIUpdate();
	} else if (event->type == SelectionRequest) {
		UIWindow *window = X11FindWindow(event->xclient.window);
		if (!window) return false;

		if ((XGetSelectionOwner(ui.display, ui.clipboardID) == window->window) 
				&& (event->xselectionrequest.selection == ui.clipboardID)) {
			XSelectionRequestEvent requestEvent = event->xselectionrequest;
			Atom utf8ID = XInternAtom(ui.display, "UTF8_STRING", 1);
			if (utf8ID == None) utf8ID = XA_STRING;

			Atom type = requestEvent.target;
			type = (type == ui.textID) ? XA_STRING : type;
			int changePropertyResult = 0;

			if(requestEvent.target == XA_STRING || requestEvent.target == ui.textID || requestEvent.target == utf8ID) {
				changePropertyResult = XChangeProperty(requestEvent.display, requestEvent.requestor, requestEvent.property, 
						type, 8, PropModeReplace, (const unsigned char *) ui.pasteText, strlen(ui.pasteText));
			} else if (requestEvent.target == ui.targetID) {
				changePropertyResult = XChangeProperty(requestEvent.display, requestEvent.requestor, requestEvent.property, 
						XA_ATOM, 32, PropModeReplace, (unsigned char *) &utf8ID, 1);
			}

			if(changePropertyResult == 0 || changePropertyResult == 1) {
				XSelectionEvent sendEvent = {
					.type = SelectionNotify,
					.serial = requestEvent.serial,
					.send_event = requestEvent.send_event,
					.display = requestEvent.display,
					.requestor = requestEvent.requestor,
					.selection = requestEvent.selection,
					.target = requestEvent.target,
					.property = requestEvent.property,
					.time = requestEvent.time
				};

				XSendEvent(ui.display, requestEvent.requestor, 0, 0, (XEvent *) &sendEvent);
			}
		}
	}

	return false;
}

bool _UIMessageLoopSingle(int *result) {
	XEvent events[64];

	if (ui.animating) {
		if (XPending(ui.display)) {
			XNextEvent(ui.display, events + 0);
		} else {
			_UIProcessAnimations();
			return true;
		}
	} else {
		XNextEvent(ui.display, events + 0);
	}

	int p = 1;

	int configureIndex = -1, motionIndex = -1, exposeIndex = -1;

	while (p < 64 && XPending(ui.display)) {
		XNextEvent(ui.display, events + p);

#define _UI_MERGE_EVENTS(a, b) \
	if (events[p].type == a) { \
		if (b != -1) events[b].type = 0; \
		b = p; \
	}

		_UI_MERGE_EVENTS(ConfigureNotify, configureIndex);
		_UI_MERGE_EVENTS(MotionNotify, motionIndex);
		_UI_MERGE_EVENTS(Expose, exposeIndex);

		p++;
	}

	for (int i = 0; i < p; i++) {
		if (!events[i].type) {
			continue;
		}

		if (_UIProcessEvent(events + i)) {
			return false;
		}
	}

	return true;
}

void UIWindowPostMessage(UIWindow *window, UIMessage message, void *_dp) {
	// HACK! Xlib doesn't seem to have a nice way to do this,
	// so send a specially crafted key press event instead.
	// TODO Maybe ClientMessage is what this should use?
	uintptr_t dp = (uintptr_t) _dp;
	XKeyEvent event = { 0 };
	event.display = ui.display;
	event.window = window->window;
	event.root = DefaultRootWindow(ui.display);
	event.subwindow = None;
	event.time = dp >> 32;
	event.x = 0x7123;
	event.y = 0x7456;
	event.x_root = (dp >> 0) & 0xFFFF;
	event.y_root = (dp >> 16) & 0xFFFF;
	event.same_screen = True;
	event.keycode = 1;
	event.state = message;
	event.type = KeyPress;
	XSendEvent(ui.display, window->window, True, KeyPressMask, (XEvent *) &event);
	XFlush(ui.display);
}

#endif

#ifdef UI_WINDOWS

const int UI_KEYCODE_A = 'A';
const int UI_KEYCODE_0 = '0';
const int UI_KEYCODE_BACKSPACE = VK_BACK;
const int UI_KEYCODE_DELETE = VK_DELETE;
const int UI_KEYCODE_DOWN = VK_DOWN;
const int UI_KEYCODE_END = VK_END;
const int UI_KEYCODE_ENTER = VK_RETURN;
const int UI_KEYCODE_ESCAPE = VK_ESCAPE;
const int UI_KEYCODE_F1 = VK_F1;
const int UI_KEYCODE_HOME = VK_HOME;
const int UI_KEYCODE_LEFT = VK_LEFT;
const int UI_KEYCODE_RIGHT = VK_RIGHT;
const int UI_KEYCODE_SPACE = VK_SPACE;
const int UI_KEYCODE_TAB = VK_TAB;
const int UI_KEYCODE_UP = VK_UP;
const int UI_KEYCODE_INSERT = VK_INSERT;

int _UIWindowMessage(UIElement *element, UIMessage message, int di, void *dp) {
	if (message == UI_MSG_DESTROY) {
		UIWindow *window = (UIWindow *) element;
		_UIWindowDestroyCommon(window);
		SetWindowLongPtr(window->hwnd, GWLP_USERDATA, 0);
		DestroyWindow(window->hwnd);
	}

	return _UIWindowMessageCommon(element, message, di, dp);
}

LRESULT CALLBACK _UIWindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	UIWindow *window = (UIWindow *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (!window || ui.assertionFailure) {
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	if (message == WM_CLOSE) {
		if (UIElementMessage(&window->e, UI_MSG_WINDOW_CLOSE, 0, 0)) {
			_UIUpdate();
			return 0;
		} else {
			PostQuitMessage(0);
		}
	} else if (message == WM_SIZE) {
		RECT client;
		GetClientRect(hwnd, &client);
		window->width = client.right;
		window->height = client.bottom;
		window->bits = (uint32_t *) UI_REALLOC(window->bits, window->width * window->height * 4);
		window->e.bounds = UI_RECT_2S(window->width, window->height);
		window->e.clip = UI_RECT_2S(window->width, window->height);
		UIElementMessage(&window->e, UI_MSG_LAYOUT, 0, 0);
		_UIUpdate();
	} else if (message == WM_MOUSEMOVE) {
		if (!window->trackingLeave) {
			window->trackingLeave = true;
			TRACKMOUSEEVENT leave = { 0 };
			leave.cbSize = sizeof(TRACKMOUSEEVENT);
			leave.dwFlags = TME_LEAVE;
			leave.hwndTrack = hwnd;
			TrackMouseEvent(&leave);
		}

		POINT cursor;
		GetCursorPos(&cursor);
		ScreenToClient(hwnd, &cursor);
		window->cursorX = cursor.x;
		window->cursorY = cursor.y;
		_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
	} else if (message == WM_MOUSELEAVE) {
		window->trackingLeave = false;

		if (!window->pressed) {
			window->cursorX = -1;
			window->cursorY = -1;
		}

		_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
	} else if (message == WM_LBUTTONDOWN) {
		SetCapture(hwnd);
		_UIWindowInputEvent(window, UI_MSG_LEFT_DOWN, 0, 0);
	} else if (message == WM_LBUTTONUP) {
		if (window->pressedButton == 1) ReleaseCapture();
		_UIWindowInputEvent(window, UI_MSG_LEFT_UP, 0, 0);
	} else if (message == WM_MBUTTONDOWN) {
		SetCapture(hwnd);
		_UIWindowInputEvent(window, UI_MSG_MIDDLE_DOWN, 0, 0);
	} else if (message == WM_MBUTTONUP) {
		if (window->pressedButton == 2) ReleaseCapture();
		_UIWindowInputEvent(window, UI_MSG_MIDDLE_UP, 0, 0);
	} else if (message == WM_RBUTTONDOWN) {
		SetCapture(hwnd);
		_UIWindowInputEvent(window, UI_MSG_RIGHT_DOWN, 0, 0);
	} else if (message == WM_RBUTTONUP) {
		if (window->pressedButton == 3) ReleaseCapture();
		_UIWindowInputEvent(window, UI_MSG_RIGHT_UP, 0, 0);
	} else if (message == WM_MOUSEWHEEL) {
		int delta = (int) wParam >> 16;
		_UIWindowInputEvent(window, UI_MSG_MOUSE_WHEEL, -delta, 0);
	} else if (message == WM_KEYDOWN) {
		window->ctrl = GetKeyState(VK_CONTROL) & 0x8000;
		window->shift = GetKeyState(VK_SHIFT) & 0x8000;
		window->alt = GetKeyState(VK_MENU) & 0x8000;

		UIKeyTyped m = { 0 };
		m.code = wParam;
		_UIWindowInputEvent(window, UI_MSG_KEY_TYPED, 0, &m);
	} else if (message == WM_CHAR) {
		UIKeyTyped m = { 0 };
		char c = wParam;
		m.text = &c;
		m.textBytes = 1;
		_UIWindowInputEvent(window, UI_MSG_KEY_TYPED, 0, &m);
	} else if (message == WM_PAINT) {
		PAINTSTRUCT paint;
		HDC dc = BeginPaint(hwnd, &paint);
		BITMAPINFOHEADER info = { 0 };
		info.biSize = sizeof(info);
		info.biWidth = window->width, info.biHeight = -window->height;
		info.biPlanes = 1, info.biBitCount = 32;
		StretchDIBits(dc, 0, 0, UI_RECT_SIZE(window->e.bounds), 0, 0, UI_RECT_SIZE(window->e.bounds),
			window->bits, (BITMAPINFO *) &info, DIB_RGB_COLORS, SRCCOPY);
		EndPaint(hwnd, &paint);
	} else if (message == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT) {
		SetCursor(ui.cursors[window->cursorStyle]);
		return 1;
	} else if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
		_UIMenusClose();

		if (message == WM_SETFOCUS) {
			_UIInspectorSetFocusedWindow(window);
			UIElementMessage(&window->e, UI_MSG_WINDOW_ACTIVATE, 0, 0);
		}
	} else if (message == WM_MOUSEACTIVATE && (window->e.flags & UI_WINDOW_MENU)) {
		return MA_NOACTIVATE;
	} else if (message == WM_DROPFILES) {
		HDROP drop = (HDROP) wParam;
		int count = DragQueryFile(drop, 0xFFFFFFFF, NULL, 0);
		char **files = (char **) UI_MALLOC(sizeof(char *) * count);
		
		for (int i = 0; i < count; i++) {
			int length = DragQueryFile(drop, i, NULL, 0);
			files[i] = (char *) UI_MALLOC(length + 1);
			files[i][length] = 0;
			DragQueryFile(drop, i, files[i], length + 1);
		}
		
		UIElementMessage(&window->e, UI_MSG_WINDOW_DROP_FILES, count, files);
		for (int i = 0; i < count; i++) UI_FREE(files[i]);		
		UI_FREE(files);
		DragFinish(drop);
		_UIUpdate();
	} else if (message == WM_APP + 1) {
		UIElementMessage(&window->e, (UIMessage) wParam, 0, (void *) lParam);
		_UIUpdate();
	} else {
		if (message == WM_NCLBUTTONDOWN || message == WM_NCMBUTTONDOWN || message == WM_NCRBUTTONDOWN) {
			if (~window->e.flags & UI_WINDOW_MENU) {
				_UIMenusClose();
				_UIUpdate();
			}
		}

		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}

void UIInitialise() {
	ui.heap = GetProcessHeap();
	
	_UIInitialiseCommon();

	ui.cursors[UI_CURSOR_ARROW] = LoadCursor(NULL, IDC_ARROW);
	ui.cursors[UI_CURSOR_TEXT] = LoadCursor(NULL, IDC_IBEAM);
	ui.cursors[UI_CURSOR_SPLIT_V] = LoadCursor(NULL, IDC_SIZENS);
	ui.cursors[UI_CURSOR_SPLIT_H] = LoadCursor(NULL, IDC_SIZEWE);
	ui.cursors[UI_CURSOR_FLIPPED_ARROW] = LoadCursor(NULL, IDC_ARROW);
	ui.cursors[UI_CURSOR_CROSS_HAIR] = LoadCursor(NULL, IDC_CROSS);
	ui.cursors[UI_CURSOR_HAND] = LoadCursor(NULL, IDC_HAND);
	ui.cursors[UI_CURSOR_RESIZE_UP] = LoadCursor(NULL, IDC_SIZENS);
	ui.cursors[UI_CURSOR_RESIZE_LEFT] = LoadCursor(NULL, IDC_SIZEWE);
	ui.cursors[UI_CURSOR_RESIZE_UP_RIGHT] = LoadCursor(NULL, IDC_SIZENESW);
	ui.cursors[UI_CURSOR_RESIZE_UP_LEFT] = LoadCursor(NULL, IDC_SIZENWSE);
	ui.cursors[UI_CURSOR_RESIZE_DOWN] = LoadCursor(NULL, IDC_SIZENS);
	ui.cursors[UI_CURSOR_RESIZE_RIGHT] = LoadCursor(NULL, IDC_SIZEWE);
	ui.cursors[UI_CURSOR_RESIZE_DOWN_LEFT] = LoadCursor(NULL, IDC_SIZENESW);
	ui.cursors[UI_CURSOR_RESIZE_DOWN_RIGHT] = LoadCursor(NULL, IDC_SIZENWSE);

	WNDCLASS windowClass = { 0 };
	windowClass.lpfnWndProc = _UIWindowProcedure;
	windowClass.lpszClassName = "normal";
	RegisterClass(&windowClass);
	windowClass.style |= CS_DROPSHADOW;
	windowClass.lpszClassName = "shadow";
	RegisterClass(&windowClass);
}

bool _UIMessageLoopSingle(int *result) {
	MSG message = { 0 };

	if (ui.animating) {
		if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT) {
				*result = message.wParam;
				return false;
			}

			TranslateMessage(&message);
			DispatchMessage(&message);
		} else {
			_UIProcessAnimations();
		}
	} else {
		if (!GetMessage(&message, NULL, 0, 0)) {
			*result = message.wParam;
			return false;
		}

		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return true;
}

void UIMenuShow(UIMenu *menu) {
	int width, height;
	_UIMenuPrepare(menu, &width, &height);
	MoveWindow(menu->e.window->hwnd, menu->pointX, menu->pointY, width, height, FALSE);
	ShowWindow(menu->e.window->hwnd, SW_SHOWNOACTIVATE);
}

UIWindow *UIWindowCreate(UIWindow *owner, uint32_t flags, const char *cTitle, int width, int height) {
	_UIMenusClose();

	UIWindow *window = (UIWindow *) UIElementCreate(sizeof(UIWindow), NULL, flags | UI_ELEMENT_WINDOW, _UIWindowMessage, "Window");
	_UIWindowAdd(window);
	if (owner) window->scale = owner->scale;

	if (flags & UI_WINDOW_MENU) {
		UI_ASSERT(owner);

		window->hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_NOACTIVATE, "shadow", 0, WS_POPUP, 
			0, 0, 0, 0, owner->hwnd, NULL, NULL, NULL);
	} else {
		window->hwnd = CreateWindowEx(WS_EX_ACCEPTFILES, "normal", cTitle, WS_OVERLAPPEDWINDOW, 
			CW_USEDEFAULT, CW_USEDEFAULT, width ? width : CW_USEDEFAULT, height ? height : CW_USEDEFAULT,
			owner ? owner->hwnd : NULL, NULL, NULL, NULL);
	}

	SetWindowLongPtr(window->hwnd, GWLP_USERDATA, (LONG_PTR) window);

	if (~flags & UI_WINDOW_MENU) {
		ShowWindow(window->hwnd, SW_SHOW);
		PostMessage(window->hwnd, WM_SIZE, 0, 0);
	}

	return window;
}

void _UIWindowEndPaint(UIWindow *window, UIPainter *painter) {
	HDC dc = GetDC(window->hwnd);
	BITMAPINFOHEADER info = { 0 };
	info.biSize = sizeof(info);
	info.biWidth = window->width, info.biHeight = window->height;
	info.biPlanes = 1, info.biBitCount = 32;
	StretchDIBits(dc, 
		UI_RECT_TOP_LEFT(window->updateRegion), UI_RECT_SIZE(window->updateRegion), 
		window->updateRegion.l, window->updateRegion.b + 1, 
		UI_RECT_WIDTH(window->updateRegion), -UI_RECT_HEIGHT(window->updateRegion),
		window->bits, (BITMAPINFO *) &info, DIB_RGB_COLORS, SRCCOPY);
	ReleaseDC(window->hwnd, dc);
}

void _UIWindowSetCursor(UIWindow *window, int cursor) {
	SetCursor(ui.cursors[cursor]);
}

void _UIWindowGetScreenPosition(UIWindow *window, int *_x, int *_y) {
	POINT p;
	p.x = 0;
	p.y = 0;
	ClientToScreen(window->hwnd, &p);
	*_x = p.x;
	*_y = p.y;
}

void UIWindowPostMessage(UIWindow *window, UIMessage message, void *_dp) {
	PostMessage(window->hwnd, WM_APP + 1, (WPARAM) message, (LPARAM) _dp);
}

void *_UIHeapReAlloc(void *pointer, size_t size) {
	if (pointer) {
		if (size) {
			return HeapReAlloc(ui.heap, 0, pointer, size);
		} else {
			UI_FREE(pointer);
			return NULL;
		}
	} else {
		if (size) {
			return UI_MALLOC(size);
		} else {
			return NULL;
		}
	}
}

#endif

