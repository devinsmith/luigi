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

static int uiDialogWrapperMessage(UIElement *element, UIMessage message, int di, void *dp) {
	if (message == UI_MSG_LAYOUT) {
		int width = UIElementMessage(element->children, UI_MSG_GET_WIDTH, 0, 0);
		int height = UIElementMessage(element->children, UI_MSG_GET_HEIGHT, width, 0);
		int cx = (element->bounds.l + element->bounds.r) / 2;
		int cy = (element->bounds.t + element->bounds.b) / 2;
		UIRectangle bounds = UI_RECT_4(cx - (width + 1) / 2, cx + width / 2, cy - (height + 1) / 2, cy + height / 2);
		UIElementMove(element->children, bounds, false);
		UIElementRepaint(element, NULL);
	} else if (message == UI_MSG_PAINT) {
		UIRectangle bounds = UIRectangleAdd(element->children->bounds, UI_RECT_1I(-1));
		UIDrawBorder((UIPainter *) dp, bounds, ui.theme->border, UI_RECT_1(1));
		UIDrawBorder((UIPainter *) dp, UIRectangleAdd(bounds, UI_RECT_1(1)), ui.theme->border, UI_RECT_1(1));
	} else if (message == UI_MSG_KEY_TYPED) {
		UIKeyTyped *typed = (UIKeyTyped *) dp;

		if (element->window->ctrl) return 0;
		if (element->window->shift) return 0;

		char c0 = 0, c1 = 0;

		if (typed->textBytes == 1 && typed->text[0] >= 'a' && typed->text[0] <= 'z') {
			c0 = typed->text[0], c1 = typed->text[0] - 'a' + 'A';
		} else {
			return 0;
		}

		UIElement *row = element->children->children;
		UIElement *target = NULL;
		bool duplicate = false;

		while (row) {
			UIElement *item = row->children;

			while (item) {
				if (UIIsButton(item)) {
					UIButton *button = (UIButton *) item;

					if (button->label && button->labelBytes && (button->label[0] == c0 || button->label[0] == c1)) {
						if (!target) {
							target = &button->e;
						} else {
							duplicate = true;
						}
					}
				}

				item = item->next;
			}

			row = row->next;
		}

		if (target) {
			if (duplicate) {
				UIElementFocus(target);
			} else {
				UIElementMessage(target, UI_MSG_CLICKED, 0, 0);
			}

			return 1;
		}
	}

	return 0;
}

static void uiDialogButtonInvoke(void *cp) {
	ui.dialogResult = (const char *) cp;
}

static int uiDialogTextboxMessage(UIElement *element, UIMessage message, int di, void *dp) {
	if (message == UI_MSG_VALUE_CHANGED) {
		UITextbox *textbox = (UITextbox *) element;
		char **buffer = (char **) element->cp;
		*buffer = (char *) UI_REALLOC(*buffer, textbox->bytes + 1);
		(*buffer)[textbox->bytes] = 0;

		for (ptrdiff_t i = 0; i < textbox->bytes; i++) {
			(*buffer)[i] = textbox->string[i];
		}
	}

	return 0;
}

const char *UIDialogShow(UIWindow *window, uint32_t flags, const char *format, ...) {
	// TODO Enter and escape.

	// Create the dialog wrapper and panel.

	UI_ASSERT(!window->dialog);
	window->dialog = UIElementCreate(sizeof(UIElement), &window->e, 0, uiDialogWrapperMessage, "DialogWrapper");
	UIPanel *panel = UIPanelCreate(window->dialog, UI_PANEL_MEDIUM_SPACING | UI_PANEL_GRAY | UI_PANEL_EXPAND);
	panel->border = UI_RECT_1(UI_SIZE_PANE_MEDIUM_BORDER * 2);
	window->e.children->flags |= UI_ELEMENT_DISABLED;

	// Create the dialog contents.

	va_list arguments;
	va_start(arguments, format);
	UIPanel *row = NULL;
	UIElement *focus = NULL;

	for (int i = 0; format[i]; i++) {
		if (i == 0 || format[i - 1] == '\n') {
			row = UIPanelCreate(&panel->e, UI_PANEL_HORIZONTAL);
			row->gap = UI_SIZE_PANE_SMALL_GAP;
		}

		if (format[i] == ' ' || format[i] == '\n') {
		} else if (format[i] == '%') {
			i++;

			if (format[i] == 'b' /* button */) {
				const char *label = va_arg(arguments, const char *);
				UIButton *button = UIButtonCreate(&row->e, 0, label, -1);
				if (!focus) focus = &button->e;
				button->invoke = uiDialogButtonInvoke;
				button->e.cp = (void *) label;
			} else if (format[i] == 's' /* label from string */) {
				const char *label = va_arg(arguments, const char *);
				UILabelCreate(&row->e, 0, label, -1);
			} else if (format[i] == 't' /* textbox */) {
				char **buffer = va_arg(arguments, char **);
				UITextbox *textbox = UITextboxCreate(&row->e, UI_ELEMENT_H_FILL);
				if (!focus) focus = &textbox->e;
				if (*buffer) UITextboxReplace(textbox, *buffer, UIStringLength(*buffer), false);
				textbox->e.cp = buffer;
				textbox->e.messageUser = uiDialogTextboxMessage;
			} else if (format[i] == 'f' /* horizontal fill */) {
				UISpacerCreate(&row->e, UI_ELEMENT_H_FILL, 0, 0);
			} else if (format[i] == 'l' /* horizontal line */) {
				UISpacerCreate(&row->e, UI_SPACER_LINE | UI_ELEMENT_H_FILL, 0, 1);
			} else if (format[i] == 'u' /* user */) {
				UIDialogUserCallback callback = va_arg(arguments, UIDialogUserCallback);
				callback(&row->e);
			}
		} else {
			int j = i;
			while (format[j] && format[j] != '%' && format[j] != '\n') j++;
			UILabelCreate(&row->e, 0, format + i, j - i);
			i = j - 1;
		}
	}

	va_end(arguments);

	window->dialogOldFocus = window->focused;
	UIElementFocus(focus ? focus : window->dialog);

	// Run the modal message loop.

	int result;
	ui.dialogResult = NULL;
	for (int i = 1; i <= 3; i++) _UIWindowSetPressed(window, NULL, i);
	UIElementRefresh(&window->e);
	_UIUpdate();
	while (!ui.dialogResult && _UIMessageLoopSingle(&result));
	ui.quit = !ui.dialogResult;

	// Destroy the dialog.

	window->e.children->flags &= ~UI_ELEMENT_DISABLED;
	UIElementDestroy(window->dialog);
	window->dialog = NULL;
	UIElementRefresh(&window->e);
	if (window->dialogOldFocus) UIElementFocus(window->dialogOldFocus);
	return ui.dialogResult ? ui.dialogResult : "";
}

