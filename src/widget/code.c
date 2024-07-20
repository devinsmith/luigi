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

int UICodeHitTest(UICode *code, int x, int y) {
	x -= code->e.bounds.l;

	if (x < 0 || x >= UI_RECT_WIDTH(code->e.bounds) - UI_SIZE_SCROLL_BAR * code->e.window->scale) {
		return 0;
	}

	y -= code->e.bounds.t - code->vScroll->position;

	UIFont *previousFont = UIFontActivate(code->font);
	int lineHeight = UIMeasureStringHeight();
	bool inMargin = x < UI_SIZE_CODE_MARGIN + UI_SIZE_CODE_MARGIN_GAP / 2 && (~code->e.flags & UI_CODE_NO_MARGIN);
	UIFontActivate(previousFont);

	if (y < 0 || y >= lineHeight * code->lineCount) {
		return 0;
	}

	int line = y / lineHeight + 1;
	return inMargin ? -line : line;
}

int UIDrawStringHighlighted(UIPainter *painter, UIRectangle lineBounds, const char *string, ptrdiff_t bytes, int tabSize) {
	if (bytes == -1) bytes = UIStringLength(string);
	if (bytes > 10000) bytes = 10000;

	uint32_t colors[] = {
		ui.theme->codeDefault,
		ui.theme->codeComment,
		ui.theme->codeString,
		ui.theme->codeNumber,
		ui.theme->codeOperator,
		ui.theme->codePreprocessor,
	};

	int x = lineBounds.l;
	int y = (lineBounds.t + lineBounds.b - UIMeasureStringHeight()) / 2;
	int ti = 0;
	int lexState = 0;
	bool inComment = false, inIdentifier = false, inChar = false, startedString = false;
	uint32_t last = 0;

	while (bytes--) {
		char c = *string++;

		last <<= 8;
		last |= c;

		if (lexState == 4) {
			lexState = 0;
		} else if (lexState == 1) {
			if ((last & 0xFF0000) == ('*' << 16) && (last & 0xFF00) == ('/' << 8) && inComment) {
				lexState = 0, inComment = false;
			}
		} else if (lexState == 3) {
			if (!UICharIsAlpha(c) && !UICharIsDigit(c)) {
				lexState = 0;
			}
		} else if (lexState == 2) {
			if (!startedString) {
				if (!inChar && ((last >> 8) & 0xFF) == '"' && ((last >> 16) & 0xFF) != '\\') {
					lexState = 0;
				} else if (inChar && ((last >> 8) & 0xFF) == '\'' && ((last >> 16) & 0xFF) != '\\') {
					lexState = 0;
				}
			}

			startedString = false;
		}

		if (lexState == 0) {
			if (c == '#') {
				lexState = 5;
			} else if (c == '/' && *string == '/') {
				lexState = 1;
			} else if (c == '/' && *string == '*') {
				lexState = 1, inComment = true;
			} else if (c == '"') {
				lexState = 2;
				inChar = false;
				startedString = true;
			} else if (c == '\'') {
				lexState = 2;
				inChar = true;
				startedString = true;
			} else if (UICharIsDigit(c) && !inIdentifier) {
				lexState = 3;
			} else if (!UICharIsAlpha(c) && !UICharIsDigit(c)) {
				lexState = 4;
				inIdentifier = false;
			} else {
				inIdentifier = true;
			}
		}

		if (c == '\t') {
			x += ui.activeFont->glyphWidth, ti++;
			while (ti % tabSize) x += ui.activeFont->glyphWidth, ti++;
		} else {
			UIDrawGlyph(painter, x, y, c, colors[lexState]);
			x += ui.activeFont->glyphWidth, ti++;
		}
	}

	return x;
}

int _UICodeMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UICode *code = (UICode *) element;
	
	if (message == UI_MSG_LAYOUT) {
		UIFont *previousFont = UIFontActivate(code->font);

		if (code->moveScrollToFocusNextLayout) {
			code->vScroll->position = (code->focused + 0.5) * UIMeasureStringHeight() - UI_RECT_HEIGHT(code->e.bounds) / 2;
		}

		UIRectangle scrollBarBounds = element->bounds;
		scrollBarBounds.l = scrollBarBounds.r - UI_SIZE_SCROLL_BAR * code->e.window->scale;
		code->vScroll->maximum = code->lineCount * UIMeasureStringHeight();
		code->vScroll->page = UI_RECT_HEIGHT(element->bounds);
		UIFontActivate(previousFont);
		UIElementMove(&code->vScroll->e, scrollBarBounds, true);
	} else if (message == UI_MSG_PAINT) {
		UIFont *previousFont = UIFontActivate(code->font);

		UIPainter *painter = (UIPainter *) dp;
		UIRectangle lineBounds = element->bounds;
		lineBounds.r -= UI_SIZE_SCROLL_BAR * code->e.window->scale;

		if (~code->e.flags & UI_CODE_NO_MARGIN) {
			lineBounds.l += UI_SIZE_CODE_MARGIN + UI_SIZE_CODE_MARGIN_GAP;
		}

		int lineHeight = UIMeasureStringHeight();
		lineBounds.t -= (int64_t) code->vScroll->position % lineHeight;

		UIDrawBlock(painter, element->bounds, ui.theme->codeBackground);

		for (int i = code->vScroll->position / lineHeight; i < code->lineCount; i++) {
			if (lineBounds.t > element->clip.b) {
				break;
			}

			lineBounds.b = lineBounds.t + lineHeight;

			if (~code->e.flags & UI_CODE_NO_MARGIN) {
				char string[16];
				int p = 16;
				int lineNumber = i + 1;

				while (lineNumber) {
					string[--p] = (lineNumber % 10) + '0';
					lineNumber /= 10;
				}

				UIRectangle marginBounds = lineBounds;
				marginBounds.r = marginBounds.l - UI_SIZE_CODE_MARGIN_GAP;
				marginBounds.l -= UI_SIZE_CODE_MARGIN + UI_SIZE_CODE_MARGIN_GAP;

				uint32_t marginColor = UIElementMessage(element, UI_MSG_CODE_GET_MARGIN_COLOR, i + 1, 0);

				if (marginColor) {
					UIDrawBlock(painter, marginBounds, marginColor);
				}

				UIDrawString(painter, marginBounds, string + p, 16 - p, ui.theme->codeDefault, UI_ALIGN_RIGHT, NULL);
			}

			if (code->focused == i) {
				UIDrawBlock(painter, lineBounds, ui.theme->codeFocused);
			}

			int x = UIDrawStringHighlighted(painter, lineBounds, code->content + code->lines[i].offset, code->lines[i].bytes, code->tabSize);
			int y = (lineBounds.t + lineBounds.b - UIMeasureStringHeight()) / 2;

			UICodeDecorateLine m = { 0 };
			m.x = x, m.y = y, m.bounds = lineBounds, m.index = i + 1, m.painter = painter;
			UIElementMessage(element, UI_MSG_CODE_DECORATE_LINE, 0, &m);

			lineBounds.t += lineHeight;
		}

		UIFontActivate(previousFont);
	} else if (message == UI_MSG_SCROLLED) {
		code->moveScrollToFocusNextLayout = false;
		UIElementRefresh(element);
	} else if (message == UI_MSG_MOUSE_WHEEL) {
		return UIElementMessage(&code->vScroll->e, message, di, dp);
	} else if (message == UI_MSG_GET_CURSOR) {
		if (UICodeHitTest(code, element->window->cursorX, element->window->cursorY) < 0) {
			return UI_CURSOR_FLIPPED_ARROW;
		}
	} else if (message == UI_MSG_DESTROY) {
		UI_FREE(code->content);
		UI_FREE(code->lines);
	}

	return 0;
}

void UICodeFocusLine(UICode *code, int index) {
	code->focused = index - 1;
	code->moveScrollToFocusNextLayout = true;
}

void UICodeInsertContent(UICode *code, const char *content, ptrdiff_t byteCount, bool replace) {
	UIFont *previousFont = UIFontActivate(code->font);

	if (byteCount == -1) {
		byteCount = UIStringLength(content);
	}

	if (byteCount > 1000000000) {
		byteCount = 1000000000;
	}

	if (replace) {
		UI_FREE(code->content);
		UI_FREE(code->lines);
		code->content = NULL;
		code->lines = NULL;
		code->contentBytes = 0;
		code->lineCount = 0;
	}

	code->content = (char *) UI_REALLOC(code->content, code->contentBytes + byteCount);

	if (!byteCount) {
		return;
	}

	int lineCount = content[byteCount - 1] != '\n';

	for (int i = 0; i < byteCount; i++) {
		code->content[i + code->contentBytes] = content[i];

		if (content[i] == '\n') {
			lineCount++;
		}
	}

	code->lines = (UICodeLine *) UI_REALLOC(code->lines, sizeof(UICodeLine) * (code->lineCount + lineCount));
	int offset = 0, lineIndex = 0;

	for (intptr_t i = 0; i <= byteCount && lineIndex < lineCount; i++) {
		if (content[i] == '\n' || i == byteCount) {
			UICodeLine line = { 0 };
			line.offset = offset + code->contentBytes;
			line.bytes = i - offset;
			code->lines[code->lineCount + lineIndex] = line;
			lineIndex++;
			offset = i + 1;
		}
	}

	code->lineCount += lineCount;
	code->contentBytes += byteCount;

	if (!replace) {
		code->vScroll->position = code->lineCount * UIMeasureStringHeight();
	}

	UIFontActivate(previousFont);
}

UICode *UICodeCreate(UIElement *parent, uint32_t flags) {
	UICode *code = (UICode *) UIElementCreate(sizeof(UICode), parent, flags, _UICodeMessage, "Code");
	code->font = ui.activeFont;
	code->vScroll = UIScrollBarCreate(&code->e, 0);
	code->focused = -1;
	code->tabSize = 4;
	return code;
}
