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

static void _UIElementDestroyDescendents(UIElement *element, bool topLevel) {
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

UIRectangle UIElementScreenBounds(UIElement *element) {
	int x = 0, y = 0;
	_UIWindowGetScreenPosition(element->window, &x, &y);
	return UIRectangleAdd(element->bounds, UI_RECT_2(x, y));
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
