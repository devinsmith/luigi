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

uint64_t UIAnimateClock() {
	return (uint64_t) UI_CLOCK() * 1000 / UI_CLOCKS_PER_SECOND;
}

void UIWindowRegisterShortcut(UIWindow *window, UIShortcut shortcut) {
	if (window->shortcutCount + 1 > window->shortcutAllocated) {
		window->shortcutAllocated = (window->shortcutCount + 1) * 2;
		window->shortcuts = (UIShortcut *) UI_REALLOC(window->shortcuts, window->shortcutAllocated * sizeof(UIShortcut));
	}

	window->shortcuts[window->shortcutCount++] = shortcut;
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



