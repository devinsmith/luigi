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

void UITextboxReplace(UITextbox *textbox, const char *text, ptrdiff_t bytes, bool sendChangedMessage) {
	if (bytes == -1) {
		bytes = UIStringLength(text);
	}

	int deleteFrom = textbox->carets[0], deleteTo = textbox->carets[1];

	if (deleteFrom > deleteTo) {
		UI_SWAP(int, deleteFrom, deleteTo);
	}

	for (int i = deleteTo; i < textbox->bytes; i++) {
		textbox->string[i - deleteTo + deleteFrom] = textbox->string[i];
	}

	textbox->bytes -= deleteTo - deleteFrom;
	textbox->carets[0] = textbox->carets[1] = deleteFrom;

	textbox->string = (char *) UI_REALLOC(textbox->string, textbox->bytes + bytes);

	for (int i = textbox->bytes + bytes - 1; i >= textbox->carets[0] + bytes; i--) {
		textbox->string[i] = textbox->string[i - bytes];
	}

	for (int i = textbox->carets[0]; i < textbox->carets[0] + bytes; i++) {
		textbox->string[i] = text[i - textbox->carets[0]];
	}

	textbox->bytes += bytes;
	textbox->carets[0] += bytes;
	textbox->carets[1] = textbox->carets[0];

	if (sendChangedMessage) {
		UIElementMessage(&textbox->e, UI_MSG_VALUE_CHANGED, 0, 0);
	}

	textbox->e.window->textboxModifiedFlag = true;
}

void UITextboxClear(UITextbox *textbox, bool sendChangedMessage) {
	textbox->carets[1] = 0;
	textbox->carets[0] = textbox->bytes;
	UITextboxReplace(textbox, "", 0, sendChangedMessage);
}

void UITextboxMoveCaret(UITextbox *textbox, bool backward, bool word) {
	while (true) {
		if (textbox->carets[0] > 0 && backward) {
			textbox->carets[0]--;
		} else if (textbox->carets[0] < textbox->bytes && !backward) {
			textbox->carets[0]++;
		} else {
			return;
		}

		if (!word) {
			return;
		} else if (textbox->carets[0] != textbox->bytes && textbox->carets[0] != 0) {
			char c1 = textbox->string[textbox->carets[0] - 1];
			char c2 = textbox->string[textbox->carets[0]];

			if (UICharIsAlphaOrDigitOrUnderscore(c1) != UICharIsAlphaOrDigitOrUnderscore(c2)) {
				return;
			}
		}
	}
}

static int uiTextboxMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UITextbox *textbox = (UITextbox *) element;

	if (message == UI_MSG_GET_HEIGHT) {
		return UI_SIZE_TEXTBOX_HEIGHT * element->window->scale;
	} else if (message == UI_MSG_GET_WIDTH) {
		return UI_SIZE_TEXTBOX_WIDTH * element->window->scale;
	} else if (message == UI_MSG_PAINT) {
		int scaledMargin = UI_SIZE_TEXTBOX_MARGIN * element->window->scale;
		int totalWidth = UIMeasureStringWidth(textbox->string, textbox->bytes) + scaledMargin * 2;
		UIRectangle textBounds = UIRectangleAdd(element->bounds, UI_RECT_1I(scaledMargin));

		if (textbox->scroll > totalWidth - UI_RECT_WIDTH(textBounds)) {
			textbox->scroll = totalWidth - UI_RECT_WIDTH(textBounds);
		}

		if (textbox->scroll < 0) {
			textbox->scroll = 0;
		}

		int caretX = UIMeasureStringWidth(textbox->string, textbox->carets[0]) - textbox->scroll;

		if (caretX < 0) {
			textbox->scroll = caretX + textbox->scroll;
		} else if (caretX > UI_RECT_WIDTH(textBounds)) {
			textbox->scroll = caretX - UI_RECT_WIDTH(textBounds) + textbox->scroll + 1;
		}

		UIPainter *painter = (UIPainter *) dp;
		bool focused = element->window->focused == element;
		bool disabled = element->flags & UI_ELEMENT_DISABLED;
		UIDrawRectangle(painter, element->bounds, 
			disabled ? ui.theme.buttonDisabled : focused ? ui.theme.textboxFocused : ui.theme.textboxNormal, 
			ui.theme.border, UI_RECT_1(1));
#ifdef __cplusplus
		UIStringSelection selection = {};
#else
		UIStringSelection selection = { 0 };
#endif
		selection.carets[0] = textbox->carets[0];
		selection.carets[1] = textbox->carets[1];
		selection.colorBackground = ui.theme.selected;
		selection.colorText = ui.theme.textSelected;
		textBounds.l -= textbox->scroll;
		UIDrawString(painter, textBounds, textbox->string, textbox->bytes, 
			disabled ? ui.theme.textDisabled : ui.theme.text, UI_ALIGN_LEFT, focused ? &selection : NULL);
	} else if (message == UI_MSG_GET_CURSOR) {
		return UI_CURSOR_TEXT;
	} else if (message == UI_MSG_LEFT_DOWN) {
		UIElementFocus(element);
	} else if (message == UI_MSG_UPDATE) {
		UIElementRepaint(element, NULL);
	} else if (message == UI_MSG_DESTROY) {
		UI_FREE(textbox->string);
	} else if (message == UI_MSG_KEY_TYPED) {
		UIKeyTyped *m = (UIKeyTyped *) dp;
		bool handled = true;

		if (textbox->rejectNextKey) {
			textbox->rejectNextKey = false;
			handled = false;
		} else if (m->code == UI_KEYCODE_BACKSPACE || m->code == UI_KEYCODE_DELETE) {
			if (textbox->carets[0] == textbox->carets[1]) {
				UITextboxMoveCaret(textbox, m->code == UI_KEYCODE_BACKSPACE, element->window->ctrl);
			}

			UITextboxReplace(textbox, NULL, 0, true);
		} else if (m->code == UI_KEYCODE_LEFT || m->code == UI_KEYCODE_RIGHT) {
			UITextboxMoveCaret(textbox, m->code == UI_KEYCODE_LEFT, element->window->ctrl);

			if (!element->window->shift) {
				textbox->carets[1] = textbox->carets[0];
			}
		} else if (m->code == UI_KEYCODE_HOME || m->code == UI_KEYCODE_END) {
			if (m->code == UI_KEYCODE_HOME) {
				textbox->carets[0] = 0;
			} else {
				textbox->carets[0] = textbox->bytes;
			}

			if (!element->window->shift) {
				textbox->carets[1] = textbox->carets[0];
			}
		} else if (m->code == UI_KEYCODE_LETTER('A') && element->window->ctrl) {
			textbox->carets[1] = 0;
			textbox->carets[0] = textbox->bytes;
		} else if (m->textBytes && !element->window->alt && !element->window->ctrl && m->text[0] >= 0x20) {
			UITextboxReplace(textbox, m->text, m->textBytes, true);
		} else if ((m->code == UI_KEYCODE_LETTER('C') || m->code == UI_KEYCODE_LETTER('X') || m->code == UI_KEYCODE_INSERT) 
				&& element->window->ctrl && !element->window->alt && !element->window->shift) {
			int   to = textbox->carets[0] > textbox->carets[1] ? textbox->carets[0] : textbox->carets[1];
			int from = textbox->carets[0] < textbox->carets[1] ? textbox->carets[0] : textbox->carets[1];

			if (from != to) {
				char *pasteText = (char *) UI_CALLOC(to - from + 1);
				for (int i = from; i < to; i++) pasteText[i - from] = textbox->string[i];
				UIClipboardWriteText(element->window, pasteText);
			}
			
			if (m->code == UI_KEYCODE_LETTER('X')) {
				UITextboxReplace(textbox, NULL, 0, true);
			}
		} else if ((m->code == UI_KEYCODE_LETTER('V') && element->window->ctrl && !element->window->alt && !element->window->shift)
				|| (m->code == UI_KEYCODE_INSERT && !element->window->ctrl && !element->window->alt && element->window->shift)) {
			size_t bytes;
			char *text = UIClipboardReadTextStart(element->window, &bytes);
			if (text) UITextboxReplace(textbox, text, bytes, true);
			UIClipboardReadTextEnd(element->window, text);
		} else {
			handled = false;
		}

		if (handled) {
			UIElementRepaint(element, NULL);
			return 1;
		}
	}

	return 0;
}

UITextbox *UITextboxCreate(UIElement *parent, uint32_t flags) {
	return (UITextbox *) UIElementCreate(sizeof(UITextbox), parent, flags | UI_ELEMENT_TAB_STOP, uiTextboxMessage, "Textbox");
}
