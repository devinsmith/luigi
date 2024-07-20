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

#ifdef WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#include <luigi.h>

#include <luigi_private.h>

static int uiScrollBarMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIScrollBar *scrollBar = (UIScrollBar *) element;

	if (message == UI_MSG_GET_WIDTH || message == UI_MSG_GET_HEIGHT) {
		return UI_SIZE_SCROLL_BAR * element->window->scale;
	} else if (message == UI_MSG_LAYOUT) {
		UIElement *up = element->children;
		UIElement *thumb = up->next;
		UIElement *down = thumb->next;

		if (scrollBar->page >= scrollBar->maximum || scrollBar->maximum <= 0 || scrollBar->page <= 0) {
			up->flags |= UI_ELEMENT_HIDE;
			thumb->flags |= UI_ELEMENT_HIDE;
			down->flags |= UI_ELEMENT_HIDE;

			scrollBar->position = 0;
		} else {
			up->flags &= ~UI_ELEMENT_HIDE;
			thumb->flags &= ~UI_ELEMENT_HIDE;
			down->flags &= ~UI_ELEMENT_HIDE;

			int size = scrollBar->horizontal ? UI_RECT_WIDTH(element->bounds) : UI_RECT_HEIGHT(element->bounds);
			int thumbSize = size * scrollBar->page / scrollBar->maximum;

			if (thumbSize < UI_SIZE_SCROLL_MINIMUM_THUMB * element->window->scale) {
				thumbSize = UI_SIZE_SCROLL_MINIMUM_THUMB * element->window->scale;
			}

			if (scrollBar->position < 0) {
				scrollBar->position = 0;
			} else if (scrollBar->position > scrollBar->maximum - scrollBar->page) {
				scrollBar->position = scrollBar->maximum - scrollBar->page;
			}

			int thumbPosition = scrollBar->position / (scrollBar->maximum - scrollBar->page) * (size - thumbSize);

			if (scrollBar->position == scrollBar->maximum - scrollBar->page) {
				thumbPosition = size - thumbSize;
			}

			if (scrollBar->horizontal) {
				UIRectangle r = element->bounds;
				r.r = r.l + thumbPosition;
				UIElementMove(up, r, false);
				r.l = r.r, r.r = r.l + thumbSize;
				UIElementMove(thumb, r, false);
				r.l = r.r, r.r = element->bounds.r;
				UIElementMove(down, r, false);
			} else {
				UIRectangle r = element->bounds;
				r.b = r.t + thumbPosition;
				UIElementMove(up, r, false);
				r.t = r.b, r.b = r.t + thumbSize;
				UIElementMove(thumb, r, false);
				r.t = r.b, r.b = element->bounds.b;
				UIElementMove(down, r, false);
			}
		}
	} else if (message == UI_MSG_PAINT) {
		if (scrollBar->page >= scrollBar->maximum || scrollBar->maximum <= 0 || scrollBar->page <= 0) {
			UIDrawBlock((UIPainter *) dp, element->bounds, ui.theme->panel1);
		}
	} else if (message == UI_MSG_MOUSE_WHEEL) {
		scrollBar->position += di;
		UIElementRefresh(element);
		UIElementMessage(element->parent, UI_MSG_SCROLLED, 0, 0);
		return 1;
	}

	return 0;
}

static int uiScrollUpDownMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIScrollBar *scrollBar = (UIScrollBar *) element->parent;
	bool isDown = element->cp;

	if (message == UI_MSG_PAINT) {
		UIPainter *painter = (UIPainter *) dp;
		uint32_t color = element == element->window->pressed ? ui.theme->buttonPressed
			: element == element->window->hovered ? ui.theme->buttonHovered : ui.theme->panel2;
		UIDrawRectangle(painter, element->bounds, color, ui.theme->border, UI_RECT_1(0));
		
		if (scrollBar->horizontal) {
			UIDrawGlyph(painter, isDown ? (element->bounds.r - ui.activeFont->glyphWidth - 2 * element->window->scale) 
					: (element->bounds.l + 2 * element->window->scale), 
				(element->bounds.t + element->bounds.b - ui.activeFont->glyphHeight) / 2,
				isDown ? 26 : 27, ui.theme->text);
		} else {
			UIDrawGlyph(painter, (element->bounds.l + element->bounds.r - ui.activeFont->glyphWidth) / 2 + 1, 
				isDown ? (element->bounds.b - ui.activeFont->glyphHeight - 2 * element->window->scale) 
					: (element->bounds.t + 2 * element->window->scale), 
				isDown ? 25 : 24, ui.theme->text);
		}
	} else if (message == UI_MSG_UPDATE) {
		UIElementRepaint(element, NULL);
	} else if (message == UI_MSG_LEFT_DOWN) {
		UIElementAnimate(element, false);
#if WIN32
		scrollBar->lastAnimateTime = GetTickCount();
#else
    scrollBar->lastAnimateTime = clock();
#endif
	} else if (message == UI_MSG_LEFT_UP) {
		UIElementAnimate(element, true);
	} else if (message == UI_MSG_ANIMATE) {
		UI_CLOCK_T previous = scrollBar->lastAnimateTime;
		UI_CLOCK_T current = UI_CLOCK();
		UI_CLOCK_T delta = current - previous;
		double deltaSeconds = (double) delta / UI_CLOCKS_PER_SECOND;
		if (deltaSeconds > 0.1) deltaSeconds = 0.1;
		double deltaPixels = deltaSeconds * scrollBar->page * 3;
		scrollBar->lastAnimateTime = current;
		if (isDown) scrollBar->position += deltaPixels;
		else scrollBar->position -= deltaPixels;
		UIElementRefresh(&scrollBar->e);
		UIElementMessage(scrollBar->e.parent, UI_MSG_SCROLLED, 0, 0);
	}

	return 0;
}

static int uiScrollThumbMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIScrollBar *scrollBar = (UIScrollBar *) element->parent;

	if (message == UI_MSG_PAINT) {
		UIPainter *painter = (UIPainter *) dp;
		uint32_t color = element == element->window->pressed ? ui.theme->buttonPressed
			: element == element->window->hovered ? ui.theme->buttonHovered : ui.theme->buttonNormal;
		UIDrawRectangle(painter, element->bounds, color, ui.theme->border, UI_RECT_1(2));
	} else if (message == UI_MSG_UPDATE) {
		UIElementRepaint(element, NULL);
	} else if (message == UI_MSG_MOUSE_DRAG && element->window->pressedButton == 1) {
		if (!scrollBar->inDrag) {
			scrollBar->inDrag = true;
			
			if (scrollBar->horizontal) {
				scrollBar->dragOffset = element->bounds.l - scrollBar->e.bounds.l - element->window->cursorX;
			} else {
				scrollBar->dragOffset = element->bounds.t - scrollBar->e.bounds.t - element->window->cursorY;
			}
		}

		int thumbPosition = (scrollBar->horizontal ? element->window->cursorX : element->window->cursorY) + scrollBar->dragOffset;
		int size = scrollBar->horizontal ? (UI_RECT_WIDTH(scrollBar->e.bounds) - UI_RECT_WIDTH(element->bounds))
				: (UI_RECT_HEIGHT(scrollBar->e.bounds) - UI_RECT_HEIGHT(element->bounds));
		scrollBar->position = (double) thumbPosition / size * (scrollBar->maximum - scrollBar->page);
		UIElementRefresh(&scrollBar->e);
		UIElementMessage(scrollBar->e.parent, UI_MSG_SCROLLED, 0, 0);
	} else if (message == UI_MSG_LEFT_UP) {
		scrollBar->inDrag = false;
	}

	return 0;
}

UIScrollBar *UIScrollBarCreate(UIElement *parent, uint32_t flags) {
	UIScrollBar *scrollBar = (UIScrollBar *) UIElementCreate(sizeof(UIScrollBar), parent, flags, uiScrollBarMessage, "Scroll Bar");
	bool horizontal = scrollBar->horizontal = flags & UI_SCROLL_BAR_HORIZONTAL;
	UIElementCreate(sizeof(UIElement), &scrollBar->e, flags, uiScrollUpDownMessage, !horizontal ? "Scroll Up" : "Scroll Left")->cp = (void *) (uintptr_t) 0;
	UIElementCreate(sizeof(UIElement), &scrollBar->e, flags, uiScrollThumbMessage, "Scroll Thumb");
	UIElementCreate(sizeof(UIElement), &scrollBar->e, flags, uiScrollUpDownMessage, !horizontal ? "Scroll Down" : "Scroll Right")->cp = (void *) (uintptr_t) 1;
	return scrollBar;
}

