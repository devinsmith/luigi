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
