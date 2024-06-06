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

static void uiWrapPanelLayoutRow(UIWrapPanel *panel, UIElement *child, UIElement *rowEnd, int rowY, int rowHeight) {
	int rowPosition = 0;

	while (child != rowEnd) {
		int height = UIElementMessage(child, UI_MSG_GET_HEIGHT, 0, 0);
		int width = UIElementMessage(child, UI_MSG_GET_WIDTH, 0, 0);
		UIRectangle relative = UI_RECT_4(rowPosition, rowPosition + width, rowY + rowHeight / 2 - height / 2, rowY + rowHeight / 2 + height / 2);
		UIElementMove(child, UIRectangleTranslate(relative, panel->e.bounds), false);
		child = child->next;
		rowPosition += width;
	}
}

static int uiWrapPanelMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIWrapPanel *panel = (UIWrapPanel *) element;
//	bool horizontal = element->flags & UI_PANEL_HORIZONTAL;

	if (message == UI_MSG_LAYOUT || message == UI_MSG_GET_HEIGHT) {
		int totalHeight = 0;
		int rowPosition = 0;
		int rowHeight = 0;
		int rowLimit = message == UI_MSG_LAYOUT ? UI_RECT_WIDTH(element->bounds) : di;

		UIElement *child = panel->e.children;
		UIElement *rowStart = child;

		while (child) {
			if (~child->flags & UI_ELEMENT_HIDE) {
				int height = UIElementMessage(child, UI_MSG_GET_HEIGHT, 0, 0);
				int width = UIElementMessage(child, UI_MSG_GET_WIDTH, 0, 0);

				if (rowLimit && rowPosition + width > rowLimit) {
					uiWrapPanelLayoutRow(panel, rowStart, child, totalHeight, rowHeight);
					totalHeight += rowHeight;
					rowPosition = rowHeight = 0;
					rowStart = child;
				}

				if (height > rowHeight) {
					rowHeight = height;
				}

				rowPosition += width;
			}

			child = child->next;
		}

		if (message == UI_MSG_GET_HEIGHT) {
			return totalHeight + rowHeight;
		} else {
			uiWrapPanelLayoutRow(panel, rowStart, child, totalHeight, rowHeight);
		}
	}

	return 0;
}

UIWrapPanel *UIWrapPanelCreate(UIElement *parent, uint32_t flags) {
	return (UIWrapPanel *) UIElementCreate(sizeof(UIWrapPanel), parent, flags, uiWrapPanelMessage, "Wrap Panel");
}
