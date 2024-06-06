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

#include <luigi.h>

#include <luigi_private.h>

#define UI_MDI_CHILD_CALCULATE_LAYOUT() \
	int titleSize = UI_SIZE_MDI_CHILD_TITLE * element->window->scale; \
	int borderSize = UI_SIZE_MDI_CHILD_BORDER * element->window->scale; \
	UIRectangle title = UIRectangleAdd(element->bounds, UI_RECT_4(borderSize, -borderSize, 0, 0)); \
	title.b = title.t + titleSize; \
	UIRectangle content = UIRectangleAdd(element->bounds, UI_RECT_4(borderSize, -borderSize, titleSize, -borderSize));

static int uiMDIChildHitTest(UIMDIChild *mdiChild, int x, int y) {
	UIElement *element = &mdiChild->e;
	UI_MDI_CHILD_CALCULATE_LAYOUT();
	int cornerSize = UI_SIZE_MDI_CHILD_CORNER * element->window->scale;
	if (!UIRectangleContains(element->bounds, x, y) || UIRectangleContains(content, x, y)) return -1;
	else if (x < element->bounds.l + cornerSize && y < element->bounds.t + cornerSize) return 0b1010;
	else if (x > element->bounds.r - cornerSize && y < element->bounds.t + cornerSize) return 0b0110;
	else if (x < element->bounds.l + cornerSize && y > element->bounds.b - cornerSize) return 0b1001;
	else if (x > element->bounds.r - cornerSize && y > element->bounds.b - cornerSize) return 0b0101;
	else if (x < element->bounds.l + borderSize) return 0b1000;
	else if (x > element->bounds.r - borderSize) return 0b0100;
	else if (y < element->bounds.t + borderSize) return 0b0010;
	else if (y > element->bounds.b - borderSize) return 0b0001;
	else if (UIRectangleContains(title, x, y)) return 0b1111;
	else return -1;
}

static void uiMDIChildCloseButton(void *_child) {
	UIElement *child = (UIElement *) _child;
	
	if (!UIElementMessage(child, UI_MSG_WINDOW_CLOSE, 0, 0)) {
		UIElementDestroy(child);
		UIElementRefresh(child->parent);
	}
}

static int uiMDIChildMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIMDIChild *mdiChild = (UIMDIChild *) element;

	if (message == UI_MSG_PAINT) {
		UI_MDI_CHILD_CALCULATE_LAYOUT();
		UIPainter *painter = (UIPainter *) dp;
		UIRectangle borders = UI_RECT_4(borderSize, borderSize, titleSize, borderSize);
		UIDrawBorder(painter, element->bounds, ui.theme.buttonNormal, borders);
		UIDrawBorder(painter, element->bounds, ui.theme.border, UI_RECT_1((int) element->window->scale));
		UIDrawBorder(painter, UIRectangleAdd(content, UI_RECT_1I(-1)), ui.theme.border, UI_RECT_1((int) element->window->scale));
		UIDrawString(painter, title, mdiChild->title, mdiChild->titleBytes, ui.theme.text, UI_ALIGN_LEFT, NULL);
	} else if (message == UI_MSG_GET_WIDTH) {
		UIElement *child = element->children;
		while (child && child->next) child = child->next;
		int width = 2 * UI_SIZE_MDI_CHILD_BORDER;
		width += (child ? UIElementMessage(child, message, di ? (di - UI_SIZE_MDI_CHILD_TITLE + UI_SIZE_MDI_CHILD_BORDER) : 0, dp) : 0);
		if (width < UI_SIZE_MDI_CHILD_MINIMUM_WIDTH) width = UI_SIZE_MDI_CHILD_MINIMUM_WIDTH;
		return width;
	} else if (message == UI_MSG_GET_HEIGHT) {
		UIElement *child = element->children;
		while (child && child->next) child = child->next;
		int height = UI_SIZE_MDI_CHILD_TITLE + UI_SIZE_MDI_CHILD_BORDER;
		height += (child ? UIElementMessage(child, message, di ? (di - 2 * UI_SIZE_MDI_CHILD_BORDER) : 0, dp) : 0);
		if (height < UI_SIZE_MDI_CHILD_MINIMUM_HEIGHT) height = UI_SIZE_MDI_CHILD_MINIMUM_HEIGHT;
		return height;
	} else if (message == UI_MSG_LAYOUT) {
		UI_MDI_CHILD_CALCULATE_LAYOUT();

		UIElement *child = element->children;
		int position = title.r;

		while (child && child->next) {
			int width = UIElementMessage(child, UI_MSG_GET_WIDTH, 0, 0);
			UIElementMove(child, UI_RECT_4(position - width, position, title.t, title.b), false);
			position -= width, child = child->next;
		}

		if (child) {
			UIElementMove(child, content, false);
		}
	} else if (message == UI_MSG_GET_CURSOR) {
		int hitTest = uiMDIChildHitTest(mdiChild, element->window->cursorX, element->window->cursorY);
		if (hitTest == 0b1000) return UI_CURSOR_RESIZE_LEFT;
		if (hitTest == 0b0010) return UI_CURSOR_RESIZE_UP;
		if (hitTest == 0b0110) return UI_CURSOR_RESIZE_UP_RIGHT;
		if (hitTest == 0b1010) return UI_CURSOR_RESIZE_UP_LEFT;
		if (hitTest == 0b0100) return UI_CURSOR_RESIZE_RIGHT;
		if (hitTest == 0b0001) return UI_CURSOR_RESIZE_DOWN;
		if (hitTest == 0b1001) return UI_CURSOR_RESIZE_DOWN_LEFT;
		if (hitTest == 0b0101) return UI_CURSOR_RESIZE_DOWN_RIGHT;
		return UI_CURSOR_ARROW;
	} else if (message == UI_MSG_LEFT_DOWN) {
		mdiChild->dragHitTest = uiMDIChildHitTest(mdiChild, element->window->cursorX, element->window->cursorY);
		mdiChild->dragOffset = UIRectangleAdd(element->bounds, UI_RECT_2(-element->window->cursorX, -element->window->cursorY));
	} else if (message == UI_MSG_LEFT_UP) {
		if (mdiChild->bounds.l < 0) mdiChild->bounds.r -= mdiChild->bounds.l, mdiChild->bounds.l = 0;
		if (mdiChild->bounds.t < 0) mdiChild->bounds.b -= mdiChild->bounds.t, mdiChild->bounds.t = 0;
		UIElementRefresh(element->parent);
	} else if (message == UI_MSG_MOUSE_DRAG) {
		if (mdiChild->dragHitTest > 0) {
#define _UI_MDI_CHILD_MOVE_EDGE(bit, edge, cursor, size, opposite, negate, minimum, offset) \
	if (mdiChild->dragHitTest & bit) mdiChild->bounds.edge = mdiChild->dragOffset.edge + element->window->cursor - element->parent->bounds.offset; \
	if ((mdiChild->dragHitTest & bit) && size(mdiChild->bounds) < minimum) mdiChild->bounds.edge = mdiChild->bounds.opposite negate minimum;
			_UI_MDI_CHILD_MOVE_EDGE(0b1000, l, cursorX, UI_RECT_WIDTH, r, -, UI_SIZE_MDI_CHILD_MINIMUM_WIDTH, l);
			_UI_MDI_CHILD_MOVE_EDGE(0b0100, r, cursorX, UI_RECT_WIDTH, l, +, UI_SIZE_MDI_CHILD_MINIMUM_WIDTH, l);
			_UI_MDI_CHILD_MOVE_EDGE(0b0010, t, cursorY, UI_RECT_HEIGHT, b, -, UI_SIZE_MDI_CHILD_MINIMUM_HEIGHT, t);
			_UI_MDI_CHILD_MOVE_EDGE(0b0001, b, cursorY, UI_RECT_HEIGHT, t, +, UI_SIZE_MDI_CHILD_MINIMUM_HEIGHT, t);
			UIElementRefresh(element->parent);
		}
	} else if (message == UI_MSG_DESTROY) {
		UI_FREE(mdiChild->title);
		UIMDIClient *client = (UIMDIClient *) element->parent;
		if (client->e.children == element) client->e.children = element->next;
		if (mdiChild->previous) mdiChild->previous->e.next = element->next;
		if (element->next) ((UIMDIChild *) element->next)->previous = mdiChild->previous;
		if (client->active == mdiChild) client->active = mdiChild->previous;
	}

	return 0;
}

static int uiMDIClientMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIMDIClient *client = (UIMDIClient *) element;

	if (message == UI_MSG_PAINT) {
		UIDrawBlock((UIPainter *) dp, element->bounds, (element->flags & UI_MDI_CLIENT_TRANSPARENT) ? 0 : ui.theme.panel2);
	} else if (message == UI_MSG_LAYOUT) {
		UIElement *child = element->children;

		while (child) {
			UI_ASSERT(child->messageClass == uiMDIChildMessage);

			UIMDIChild *mdiChild = (UIMDIChild *) child;

			if (UIRectangleEquals(mdiChild->bounds, UI_RECT_1(0))) {
				int width = UIElementMessage(&mdiChild->e, UI_MSG_GET_WIDTH, 0, 0);
				int height = UIElementMessage(&mdiChild->e, UI_MSG_GET_HEIGHT, width, 0);
				if (client->cascade + width > element->bounds.r || client->cascade + height > element->bounds.b) client->cascade = 0;
				mdiChild->bounds = UI_RECT_4(client->cascade, client->cascade + width, client->cascade, client->cascade + height);
				client->cascade += UI_SIZE_MDI_CASCADE * element->window->scale;
			}

			UIRectangle bounds = UIRectangleAdd(mdiChild->bounds, UI_RECT_2(element->bounds.l, element->bounds.t));
			UIElementMove(child, bounds, false);
			child = child->next;
		}
	} else if (message == UI_MSG_FIND_BY_POINT) {
		UIFindByPoint *m = (UIFindByPoint *) dp;
		UIMDIChild *child = client->active;

		while (child) {
			if (UIRectangleContains(child->e.bounds, m->x, m->y)) {
				m->result = UIElementFindByPoint(&child->e, m->x, m->y);
				return 1;
			}

			child = child->previous;
		}

		return 1;
	} else if (message == UI_MSG_PRESSED_DESCENDENT) {
		UIMDIChild *child = (UIMDIChild *) dp;

		if (child && child != client->active) {
			if (client->e.children == &child->e) client->e.children = child->e.next;
			if (child->previous) child->previous->e.next = child->e.next;
			if (child->e.next) ((UIMDIChild *) child->e.next)->previous = child->previous;
			if (client->active) client->active->e.next = &child->e;
			child->previous = client->active;
			child->e.next = NULL;
			client->active = child;
			((UIMDIChild *) client->e.children)->previous = NULL;
			UIElementRefresh(element);
		}
	}

	return 0;
}

UIMDIChild *UIMDIChildCreate(UIElement *parent, uint32_t flags, UIRectangle initialBounds, const char *title, ptrdiff_t titleBytes) {
	UI_ASSERT(parent->messageClass == uiMDIClientMessage);

	UIMDIChild *mdiChild = (UIMDIChild *) UIElementCreate(sizeof(UIMDIChild), parent, flags, uiMDIChildMessage, "MDIChild");
	UIMDIClient *mdiClient = (UIMDIClient *) parent;

	mdiChild->bounds = initialBounds;
	mdiChild->title = UIStringCopy(title, (mdiChild->titleBytes = titleBytes));
	mdiChild->previous = mdiClient->active;
	mdiClient->active = mdiChild;

	if (flags & UI_MDI_CHILD_CLOSE_BUTTON) {
		UIButton *closeButton = UIButtonCreate(&mdiChild->e, UI_BUTTON_SMALL | UI_ELEMENT_NON_CLIENT, "X", 1);
		closeButton->invoke = uiMDIChildCloseButton;
		closeButton->e.cp = mdiChild;
	}

	return mdiChild;
}

bool UIIsMDIChild(const UIElement *element)
{
  return element->messageClass == uiMDIChildMessage;
}

UIMDIClient *UIMDIClientCreate(UIElement *parent, uint32_t flags) {
	return (UIMDIClient *) UIElementCreate(sizeof(UIMDIClient), parent, flags, uiMDIClientMessage, "MDIClient");
}
