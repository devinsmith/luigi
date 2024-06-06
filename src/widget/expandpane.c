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

static int uiExpandPaneMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIExpandPane *pane = (UIExpandPane *) element;
	
	if (message == UI_MSG_GET_HEIGHT) {
		int height = UIElementMessage(&pane->button->e, message, di, dp);

		if (pane->expanded) {
			height += UIElementMessage(&pane->panel->e, message, di, dp);
		}

		return height;
	} else if (message == UI_MSG_LAYOUT) {
		UIRectangle bounds = pane->e.bounds;
		int buttonHeight = UIElementMessage(&pane->button->e, UI_MSG_GET_HEIGHT, UI_RECT_WIDTH(bounds), NULL);
		UIElementMove(&pane->button->e, UI_RECT_4(bounds.l, bounds.r, bounds.t, bounds.t + buttonHeight), false);

		if (pane->expanded) {
			pane->panel->e.flags &= ~UI_ELEMENT_HIDE;
			UIElementMove(&pane->panel->e, UI_RECT_4(bounds.l, bounds.r, bounds.t + buttonHeight, bounds.b), false);
		} else {
			pane->panel->e.flags |= UI_ELEMENT_HIDE;
		}
	} else if (message == UI_MSG_CLIENT_PARENT) {
		*(UIElement **) dp = &pane->panel->e;
	}

	return 0;
}

static void uiExpandPaneButtonInvoke(void *cp) {
	UIExpandPane *pane = (UIExpandPane *) cp;
	pane->expanded = !pane->expanded;
	if (pane->expanded) pane->button->e.flags |= UI_BUTTON_CHECKED;
	else pane->button->e.flags &= ~UI_BUTTON_CHECKED;

	UIElement *ancestor = &pane->e;

	while (ancestor) {
		UIElementRefresh(ancestor);

		if ((UIIsPanel(ancestor) && (ancestor->flags & UI_PANEL_SCROLL))
				|| (UIIsMDIChild(ancestor))
				|| (ancestor->flags & UI_ELEMENT_V_FILL)) {
			break;
		}

		ancestor = ancestor->parent;
	}
}

UIExpandPane *UIExpandPaneCreate(UIElement *parent, uint32_t flags, const char *label, ptrdiff_t labelBytes, uint32_t panelFlags) {
	UIExpandPane *pane = (UIExpandPane *) UIElementCreate(sizeof(UIExpandPane), parent, flags, uiExpandPaneMessage, "ExpandPane");
	pane->button = UIButtonCreate(parent, UI_ELEMENT_NON_CLIENT, label, labelBytes);
	pane->button->e.cp = pane;
	pane->button->invoke = uiExpandPaneButtonInvoke;
	pane->panel = UIPanelCreate(parent, UI_ELEMENT_NON_CLIENT | panelFlags);
	return pane;
}

