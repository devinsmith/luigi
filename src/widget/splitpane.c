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

static int uiSplitPaneMessage(UIElement *element, UIMessage message, int di, void *dp);

static int uiSplitterMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UISplitPane *splitPane = (UISplitPane *) element->parent;
	bool vertical = splitPane->e.flags & UI_SPLIT_PANE_VERTICAL;

	if (message == UI_MSG_PAINT) {
		UIRectangle borders = vertical ? UI_RECT_2(0, 1) : UI_RECT_2(1, 0);
		UIDrawRectangle((UIPainter *) dp, element->bounds, ui.theme.buttonNormal, ui.theme.border, borders);
	} else if (message == UI_MSG_GET_CURSOR) {
		return vertical ? UI_CURSOR_SPLIT_V : UI_CURSOR_SPLIT_H;
	} else if (message == UI_MSG_MOUSE_DRAG) {
		int cursor = vertical ? element->window->cursorY : element->window->cursorX;
		int splitterSize = UI_SIZE_SPLITTER * element->window->scale;
		int space = (vertical ? UI_RECT_HEIGHT(splitPane->e.bounds) : UI_RECT_WIDTH(splitPane->e.bounds)) - splitterSize;
		float oldWeight = splitPane->weight;
		splitPane->weight = (float) (cursor - splitterSize / 2 - (vertical ? splitPane->e.bounds.t : splitPane->e.bounds.l)) / space;
		if (splitPane->weight < 0.05f) splitPane->weight = 0.05f;
		if (splitPane->weight > 0.95f) splitPane->weight = 0.95f;

		if (element->next->next->messageClass == uiSplitPaneMessage
				&& (element->next->next->flags & UI_SPLIT_PANE_VERTICAL) == (splitPane->e.flags & UI_SPLIT_PANE_VERTICAL)) {
			UISplitPane *subSplitPane = (UISplitPane *) element->next->next;
			subSplitPane->weight = (splitPane->weight - oldWeight - subSplitPane->weight + oldWeight * subSplitPane->weight) / (-1 + splitPane->weight);
			if (subSplitPane->weight < 0.05f) subSplitPane->weight = 0.05f;
			if (subSplitPane->weight > 0.95f) subSplitPane->weight = 0.95f;
		}

		UIElementRefresh(&splitPane->e);
	}

	return 0;
}

static int uiSplitPaneMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UISplitPane *splitPane = (UISplitPane *) element;
	bool vertical = splitPane->e.flags & UI_SPLIT_PANE_VERTICAL;

	if (message == UI_MSG_LAYOUT) {
		UIElement *splitter = element->children;
		UI_ASSERT(splitter);
		UIElement *left = splitter->next;
		UI_ASSERT(left);
		UIElement *right = left->next;
		UI_ASSERT(right);
		UI_ASSERT(!right->next);

		int splitterSize = UI_SIZE_SPLITTER * element->window->scale;
		int space = (vertical ? UI_RECT_HEIGHT(element->bounds) : UI_RECT_WIDTH(element->bounds)) - splitterSize;
		int leftSize = space * splitPane->weight;
		int rightSize = space - leftSize;

		if (vertical) {
			UIElementMove(left, UI_RECT_4(element->bounds.l, element->bounds.r, element->bounds.t, element->bounds.t + leftSize), false);
			UIElementMove(splitter, UI_RECT_4(element->bounds.l, element->bounds.r, element->bounds.t + leftSize, element->bounds.t + leftSize + splitterSize), false);
			UIElementMove(right, UI_RECT_4(element->bounds.l, element->bounds.r, element->bounds.b - rightSize, element->bounds.b), false);
		} else {
			UIElementMove(left, UI_RECT_4(element->bounds.l, element->bounds.l + leftSize, element->bounds.t, element->bounds.b), false);
			UIElementMove(splitter, UI_RECT_4(element->bounds.l + leftSize, element->bounds.l + leftSize + splitterSize, element->bounds.t, element->bounds.b), false);
			UIElementMove(right, UI_RECT_4(element->bounds.r - rightSize, element->bounds.r, element->bounds.t, element->bounds.b), false);
		}
	}

	return 0;
}

UISplitPane *UISplitPaneCreate(UIElement *parent, uint32_t flags, float weight) {
	UISplitPane *splitPane = (UISplitPane *) UIElementCreate(sizeof(UISplitPane), parent, flags, uiSplitPaneMessage, "Split Pane");
	splitPane->weight = weight;
	UIElementCreate(sizeof(UIElement), &splitPane->e, 0, uiSplitterMessage, "Splitter");
	return splitPane;
}
