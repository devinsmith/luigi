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

static int uiTabPaneMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UITabPane *tabPane = (UITabPane *) element;
	
	if (message == UI_MSG_PAINT) {
		UIPainter *painter = (UIPainter *) dp;
		UIRectangle top = element->bounds;
		top.b = top.t + UI_SIZE_BUTTON_HEIGHT * element->window->scale;
		UIDrawRectangle(painter, top, ui.theme->panel1, ui.theme->border, UI_RECT_4(0, 0, 0, 1));

		UIRectangle tab = top;
		tab.l += UI_SIZE_TAB_PANE_SPACE_LEFT * element->window->scale;
		tab.t += UI_SIZE_TAB_PANE_SPACE_TOP * element->window->scale;

		int position = 0;
		int index = 0;

		while (true) {
			int end = position;
			for (; tabPane->tabs[end] != '\t' && tabPane->tabs[end]; end++);

			int width = UIMeasureStringWidth(tabPane->tabs, end - position);
			tab.r = tab.l + width + UI_SIZE_BUTTON_PADDING;

			uint32_t color = tabPane->active == index ? ui.theme->buttonPressed : ui.theme->buttonNormal;

			UIRectangle t = tab;

			if (tabPane->active == index) {
				t.b++;
				t.t--;
			} else {
				t.t++;
			}

			UIDrawRectangle(painter, t, color, ui.theme->border, UI_RECT_1(1));
			UIDrawString(painter, tab, tabPane->tabs + position, end - position, ui.theme->text, UI_ALIGN_CENTER, NULL);
			tab.l = tab.r - 1;

			if (tabPane->tabs[end] == '\t') {
				position = end + 1;
				index++;
			} else {
				break;
			}
		}
	} else if (message == UI_MSG_LEFT_DOWN) {
		UIRectangle tab = element->bounds;
		tab.b = tab.t + UI_SIZE_BUTTON_HEIGHT * element->window->scale;
		tab.l += UI_SIZE_TAB_PANE_SPACE_LEFT * element->window->scale;
		tab.t += UI_SIZE_TAB_PANE_SPACE_TOP * element->window->scale;

		int position = 0;
		int index = 0;

		while (true) {
			int end = position;
			for (; tabPane->tabs[end] != '\t' && tabPane->tabs[end]; end++);

			int width = UIMeasureStringWidth(tabPane->tabs, end - position);
			tab.r = tab.l + width + UI_SIZE_BUTTON_PADDING;

			if (UIRectangleContains(tab, element->window->cursorX, element->window->cursorY)) {
				tabPane->active = index;
				UIElementMessage(element, UI_MSG_LAYOUT, 0, 0);
				UIElementRepaint(element, NULL);
				break;
			}

			tab.l = tab.r - 1;

			if (tabPane->tabs[end] == '\t') {
				position = end + 1;
				index++;
			} else {
				break;
			}
		}
	} else if (message == UI_MSG_LAYOUT) {
		UIElement *child = element->children;
		int index = 0;

		UIRectangle content = element->bounds;
		content.t += UI_SIZE_BUTTON_HEIGHT * element->window->scale;

		while (child) {
			if (tabPane->active == index) {
				child->flags &= ~UI_ELEMENT_HIDE;
				UIElementMove(child, content, false);
				UIElementMessage(child, UI_MSG_TAB_SELECTED, 0, 0);
			} else {
				child->flags |= UI_ELEMENT_HIDE;
			}

			child = child->next;
			index++;
		}
	} else if (message == UI_MSG_GET_HEIGHT) {
		UIElement *child = element->children;
		int index = 0;
		int baseHeight = UI_SIZE_BUTTON_HEIGHT * element->window->scale;

		while (child) {
			if (tabPane->active == index) {
				return baseHeight + UIElementMessage(child, UI_MSG_GET_HEIGHT, di, dp);
			}

			child = child->next;
			index++;
		}
	} else if (message == UI_MSG_DESTROY) {
		UI_FREE(tabPane->tabs);
	}

	return 0;
}

UITabPane *UITabPaneCreate(UIElement *parent, uint32_t flags, const char *tabs) {
	UITabPane *tabPane = (UITabPane *) UIElementCreate(sizeof(UITabPane), parent, flags, uiTabPaneMessage, "Tab Pane");
	tabPane->tabs = UIStringCopy(tabs, -1);
	return tabPane;
}
