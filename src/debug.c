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

#ifdef UI_DEBUG

void UIInspectorLog(const char *cFormat, ...) {
	va_list arguments;
	va_start(arguments, cFormat);
	char buffer[4096];
	vsnprintf(buffer, sizeof(buffer), cFormat, arguments);
	UICodeInsertContent(ui.inspectorLog, buffer, -1, false);
	va_end(arguments);
	UIElementRefresh(&ui.inspectorLog->e);
}

UIElement *_UIInspectorFindNthElement(UIElement *element, int *index, int *depth) {
	if (*index == 0) {
		return element;
	}

	*index = *index - 1;
	
	UIElement *child = element->children;

	while (child) {
		if (!(child->flags & (UI_ELEMENT_DESTROY | UI_ELEMENT_HIDE))) {
			UIElement *result = _UIInspectorFindNthElement(child, index, depth);

			if (result) {
				if (depth) {
					*depth = *depth + 1;
				}

				return result;
			}
		}

		child = child->next;
	}

	return NULL;
}

int _UIInspectorTableMessage(UIElement *element, UIMessage message, int di, void *dp) {
	if (!ui.inspectorTarget) {
		return 0;
	}

	if (message == UI_MSG_TABLE_GET_ITEM) {
		UITableGetItem *m = (UITableGetItem *) dp;
		int index = m->index;
		int depth = 0;
		UIElement *element = _UIInspectorFindNthElement(&ui.inspectorTarget->e, &index, &depth);
		if (!element) return 0;

		if (m->column == 0) {
			return snprintf(m->buffer, m->bufferBytes, "%.*s%s", depth * 2, "                ", element->cClassName);
		} else if (m->column == 1) {
			return snprintf(m->buffer, m->bufferBytes, "%d:%d, %d:%d", UI_RECT_ALL(element->bounds));
		} else if (m->column == 2) {
			return snprintf(m->buffer, m->bufferBytes, "%d%c", element->id, element->window->focused == element ? '*' : ' ');
		}
	} else if (message == UI_MSG_MOUSE_MOVE) {
		int index = UITableHitTest(ui.inspectorTable, element->window->cursorX, element->window->cursorY);
		UIElement *element = NULL;
		if (index >= 0) element = _UIInspectorFindNthElement(&ui.inspectorTarget->e, &index, NULL);
		UIWindow *window = ui.inspectorTarget;
		UIPainter painter = { 0 };
		window->updateRegion = window->e.bounds;
		painter.bits = window->bits;
		painter.width = window->width;
		painter.height = window->height;
		painter.clip = UI_RECT_2S(window->width, window->height);

		for (int i = 0; i < window->width * window->height; i++) {
			window->bits[i] = 0xFF00FF;
		}

		_UIElementPaint(&window->e, &painter);
		painter.clip = UI_RECT_2S(window->width, window->height);

		if (element) {
			UIDrawInvert(&painter, element->bounds);
			UIDrawInvert(&painter, UIRectangleAdd(element->bounds, UI_RECT_1I(4)));
		}

		_UIWindowEndPaint(window, &painter);
	}

	return 0;
}

void _UIInspectorCreate() {
	ui.inspector = UIWindowCreate(0, UI_WINDOW_INSPECTOR, "Inspector", 0, 0);
	UISplitPane *splitPane = UISplitPaneCreate(&ui.inspector->e, 0, 0.5f);
	ui.inspectorTable = UITableCreate(&splitPane->e, 0, "Class\tBounds\tID");
	ui.inspectorTable->e.messageUser = _UIInspectorTableMessage;
	ui.inspectorLog = UICodeCreate(&splitPane->e, 0);
}

int _UIInspectorCountElements(UIElement *element) {
	UIElement *child = element->children;
	int count = 1;

	while (child) {
		if (!(child->flags & (UI_ELEMENT_DESTROY | UI_ELEMENT_HIDE))) {
			count += _UIInspectorCountElements(child);
		}

		child = child->next;
	}

	return count;
}

void _UIInspectorRefresh() {
	if (!ui.inspectorTarget || !ui.inspector || !ui.inspectorTable) return;
	ui.inspectorTable->itemCount = _UIInspectorCountElements(&ui.inspectorTarget->e);
	UITableResizeColumns(ui.inspectorTable);
	UIElementRefresh(&ui.inspectorTable->e);
}

void _UIInspectorSetFocusedWindow(UIWindow *window) {
	if (!ui.inspector || !ui.inspectorTable) return;

	if (window->e.flags & UI_WINDOW_INSPECTOR) {
		return;
	}

	if (ui.inspectorTarget != window) {
		ui.inspectorTarget = window;
		_UIInspectorRefresh();
	}
}

#else

void _UIInspectorCreate() {}
void _UIInspectorSetFocusedWindow(UIWindow *window) {}
void _UIInspectorRefresh() {}

#endif

#ifdef UI_AUTOMATION_TESTS

int UIAutomationRunTests();

void UIAutomationProcessMessage() {
	int result;
	_UIMessageLoopSingle(&result);
}

void UIAutomationKeyboardTypeSingle(intptr_t code, bool ctrl, bool shift, bool alt) {
	UIWindow *window = ui.windows; // TODO Get the focused window.
	UIKeyTyped m = { 0 };
	m.code = code;
	window->ctrl = ctrl;
	window->alt = alt;
	window->shift = shift;
	_UIWindowInputEvent(window, UI_MSG_KEY_TYPED, 0, &m);
	window->ctrl = false;
	window->alt = false;
	window->shift = false;
}

void UIAutomationKeyboardType(const char *string) {
	UIWindow *window = ui.windows; // TODO Get the focused window.

	UIKeyTyped m = { 0 };
	char c[2];
	m.text = c;
	m.textBytes = 1;
	c[1] = 0;

	for (int i = 0; string[i]; i++) {
		window->ctrl = false;
		window->alt = false;
		window->shift = (c[0] >= 'A' && c[0] <= 'Z');
		c[0] = string[i];
		m.code = (c[0] >= 'A' && c[0] <= 'Z') ? UI_KEYCODE_LETTER(c[0]) 
			: c[0] == '\n' ? UI_KEYCODE_ENTER 
			: c[0] == '\t' ? UI_KEYCODE_TAB 
			: c[0] == ' ' ? UI_KEYCODE_SPACE 
			: (c[0] >= '0' && c[0] <= '9') ? UI_KEYCODE_DIGIT(c[0]) : 0;
		_UIWindowInputEvent(window, UI_MSG_KEY_TYPED, 0, &m);
	}

	window->ctrl = false;
	window->alt = false;
	window->shift = false;
}

bool UIAutomationCheckCodeLineMatches(UICode *code, int lineIndex, const char *input) {
	if (lineIndex < 1 || lineIndex > code->lineCount) return false;
	int bytes = 0;
	for (int i = 0; input[i]; i++) bytes++;
	if (bytes != code->lines[lineIndex - 1].bytes) return false;
	for (int i = 0; input[i]; i++) if (code->content[code->lines[lineIndex - 1].offset + i] != input[i]) return false;
	return true;
}

bool UIAutomationCheckTableItemMatches(UITable *table, int row, int column, const char *input) {
	int bytes = 0;
	for (int i = 0; input[i]; i++) bytes++;
	if (row < 0 || row >= table->itemCount) return false;
	if (column < 0 || column >= table->columnCount) return false;
	char *buffer = (char *) UI_MALLOC(bytes + 1);
	UITableGetItem m = { 0 };
	m.buffer = buffer;
	m.bufferBytes = bytes + 1;
	m.column = column;
	m.index = row;
	int length = UIElementMessage(&table->e, UI_MSG_TABLE_GET_ITEM, 0, &m);
	if (length != bytes) return false;
	for (int i = 0; input[i]; i++) if (buffer[i] != input[i]) return false;
	return true;
}

#endif
