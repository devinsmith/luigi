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

static int uiPanelMeasure(UIPanel *panel) {
	bool horizontal = panel->e.flags & UI_PANEL_HORIZONTAL;
	int size = 0;
	UIElement *child = panel->e.children;

	while (child) {
		if (~child->flags & UI_ELEMENT_HIDE) {
			if (horizontal) {
				int height = UIElementMessage(child, UI_MSG_GET_HEIGHT, 0, 0);

				if (height > size) {
					size = height;
				}
			} else {
				int width = UIElementMessage(child, UI_MSG_GET_WIDTH, 0, 0);

				if (width > size) {
					size = width;
				}
			}
		}

		child = child->next;
	}

	int border = 0;

	if (horizontal) {
		border = panel->border.t + panel->border.b;
	} else {
		border = panel->border.l + panel->border.r;
	}

	return size + border * panel->e.window->scale;
}

static int uiPanelLayout(UIPanel *panel, UIRectangle bounds, bool measure) {
	bool horizontal = panel->e.flags & UI_PANEL_HORIZONTAL;
	float scale = panel->e.window->scale;
	int position = (horizontal ? panel->border.l : panel->border.t) * scale;
	if (panel->scrollBar && !measure) position -= panel->scrollBar->position;
	int hSpace = UI_RECT_WIDTH(bounds) - UI_RECT_TOTAL_H(panel->border) * scale;
	int vSpace = UI_RECT_HEIGHT(bounds) - UI_RECT_TOTAL_V(panel->border) * scale;

	int available = horizontal ? hSpace : vSpace;
	int fill = 0, count = 0, perFill = 0;

	for (UIElement *child = panel->e.children; child; child = child->next) {
		if (child->flags & (UI_ELEMENT_HIDE | UI_ELEMENT_NON_CLIENT)) {
			continue;
		}

		count++;

		if (horizontal) {
			if (child->flags & UI_ELEMENT_H_FILL) {
				fill++;
			} else if (available > 0) {
				available -= UIElementMessage(child, UI_MSG_GET_WIDTH, vSpace, 0);
			}
		} else {
			if (child->flags & UI_ELEMENT_V_FILL) {
				fill++;
			} else if (available > 0) {
				available -= UIElementMessage(child, UI_MSG_GET_HEIGHT, hSpace, 0);
			}
		}
	}

	if (count) {
		available -= (count - 1) * (int) (panel->gap * scale);
	}

	if (available > 0 && fill) {
		perFill = available / fill;
	}

	bool expand = panel->e.flags & UI_PANEL_EXPAND;
	int scaledBorder2 = (horizontal ? panel->border.t : panel->border.l) * panel->e.window->scale;

	for (UIElement *child = panel->e.children; child; child = child->next) {
		if (child->flags & (UI_ELEMENT_HIDE | UI_ELEMENT_NON_CLIENT)) {
			continue;
		}

		if (horizontal) {
			int height = ((child->flags & UI_ELEMENT_V_FILL) || expand) ? vSpace : UIElementMessage(child, UI_MSG_GET_HEIGHT, 0, 0);
			int width = (child->flags & UI_ELEMENT_H_FILL) ? perFill : UIElementMessage(child, UI_MSG_GET_WIDTH, height, 0);
			UIRectangle relative = UI_RECT_4(position, position + width, 
					scaledBorder2 + (vSpace - height) / 2, 
					scaledBorder2 + (vSpace + height) / 2);
			if (!measure) UIElementMove(child, UIRectangleTranslate(relative, bounds), false);
			position += width + panel->gap * scale;
		} else {
			int width = ((child->flags & UI_ELEMENT_H_FILL) || expand) ? hSpace : UIElementMessage(child, UI_MSG_GET_WIDTH, 0, 0);
			int height = (child->flags & UI_ELEMENT_V_FILL) ? perFill : UIElementMessage(child, UI_MSG_GET_HEIGHT, width, 0);
			UIRectangle relative = UI_RECT_4(scaledBorder2 + (hSpace - width) / 2, 
					scaledBorder2 + (hSpace + width) / 2, position, position + height);
			if (!measure) UIElementMove(child, UIRectangleTranslate(relative, bounds), false);
			position += height + panel->gap * scale;
		}
	}

	return position - panel->gap * scale + (horizontal ? panel->border.r : panel->border.b) * scale;
}

static int uiPanelMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIPanel *panel = (UIPanel *) element;
	bool horizontal = element->flags & UI_PANEL_HORIZONTAL;

	if (message == UI_MSG_LAYOUT) {
		int scrollBarWidth = panel->scrollBar ? (UI_SIZE_SCROLL_BAR * element->window->scale) : 0;
		UIRectangle bounds = element->bounds;
		bounds.r -= scrollBarWidth;

		if (panel->scrollBar) {
			UIRectangle scrollBarBounds = element->bounds;
			scrollBarBounds.l = scrollBarBounds.r - scrollBarWidth;
			panel->scrollBar->maximum = uiPanelLayout(panel, bounds, true);
			panel->scrollBar->page = UI_RECT_HEIGHT(element->bounds);
			UIElementMove(&panel->scrollBar->e, scrollBarBounds, true);
		}

    uiPanelLayout(panel, bounds, false);
	} else if (message == UI_MSG_GET_WIDTH) {
		if (horizontal) {
			return uiPanelLayout(panel, UI_RECT_4(0, 0, 0, di), true);
		} else {
			return uiPanelMeasure(panel);
		}
	} else if (message == UI_MSG_GET_HEIGHT) {
		if (horizontal) {
			return uiPanelMeasure(panel);
		} else {
			int width = di && panel->scrollBar ? (di - UI_SIZE_SCROLL_BAR * element->window->scale) : di;
			return uiPanelLayout(panel, UI_RECT_4(0, width, 0, 0), true);
		}
	} else if (message == UI_MSG_PAINT) {
		if (element->flags & UI_PANEL_GRAY) {
			UIDrawBlock((UIPainter *) dp, element->bounds, ui.theme.panel1);
		} else if (element->flags & UI_PANEL_WHITE) {
			UIDrawBlock((UIPainter *) dp, element->bounds, ui.theme.panel2);
		} 
		
		if (element->flags & UI_PANEL_BORDER) {
			UIDrawBorder((UIPainter *) dp, element->bounds, ui.theme.border, UI_RECT_1((int) element->window->scale));
		}
	} else if (message == UI_MSG_MOUSE_WHEEL && panel->scrollBar) {
		return UIElementMessage(&panel->scrollBar->e, message, di, dp);
	} else if (message == UI_MSG_SCROLLED) {
		UIElementRefresh(element);
	}

	return 0;
}

bool UIIsPanel(const UIElement *element)
{
  return element->messageClass == uiPanelMessage;
}

UIPanel *UIPanelCreate(UIElement *parent, uint32_t flags) {
	UIPanel *panel = (UIPanel *) UIElementCreate(sizeof(UIPanel), parent, flags, uiPanelMessage, "Panel");

	if (flags & UI_PANEL_MEDIUM_SPACING) {
		panel->border = UI_RECT_1(UI_SIZE_PANE_MEDIUM_BORDER);
		panel->gap = UI_SIZE_PANE_MEDIUM_GAP;
	} else if (flags & UI_PANEL_SMALL_SPACING) {
		panel->border = UI_RECT_1(UI_SIZE_PANE_SMALL_BORDER);
		panel->gap = UI_SIZE_PANE_SMALL_GAP;
	}

	if (flags & UI_PANEL_SCROLL) {
		panel->scrollBar = UIScrollBarCreate(&panel->e, UI_ELEMENT_NON_CLIENT);
	}

	return panel;
}
