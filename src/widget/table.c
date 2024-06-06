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

int UITableHitTest(UITable *table, int x, int y) {
	x -= table->e.bounds.l;

	if (x < 0 || x >= UI_RECT_WIDTH(table->e.bounds) - UI_SIZE_SCROLL_BAR * table->e.window->scale) {
		return -1;
	}

	y -= (table->e.bounds.t + UI_SIZE_TABLE_HEADER * table->e.window->scale) - table->vScroll->position;

	int rowHeight = UI_SIZE_TABLE_ROW * table->e.window->scale;

	if (y < 0 || y >= rowHeight * table->itemCount) {
		return -1;
	}

	return y / rowHeight;
}

int UITableHeaderHitTest(UITable *table, int x, int y) {
	if (!table->columnCount) return -1;
	UIRectangle header = table->e.bounds;
	header.b = header.t + UI_SIZE_TABLE_HEADER * table->e.window->scale;
	header.l += UI_SIZE_TABLE_COLUMN_GAP * table->e.window->scale;
	int position = 0, index = 0;

	while (true) {
		int end = position;
		for (; table->columns[end] != '\t' && table->columns[end]; end++);
		header.r = header.l + table->columnWidths[index];
		if (UIRectangleContains(header, x, y)) return index;
		header.l += table->columnWidths[index] + UI_SIZE_TABLE_COLUMN_GAP * table->e.window->scale;
		if (table->columns[end] != '\t') break;
		position = end + 1, index++;
	}

	return -1;
}

bool UITableEnsureVisible(UITable *table, int index) {
	int rowHeight = UI_SIZE_TABLE_ROW * table->e.window->scale;
	int y = index * rowHeight;
	y -= table->vScroll->position;
	int height = UI_RECT_HEIGHT(table->e.bounds) - UI_SIZE_TABLE_HEADER * table->e.window->scale - rowHeight;

	if (y < 0) {
		table->vScroll->position += y;
		UIElementRefresh(&table->e);
		return true;
	} else if (y > height) {
		table->vScroll->position -= height - y;
		UIElementRefresh(&table->e);
		return true;
	} else {
		return false;
	}
}

void UITableResizeColumns(UITable *table) {
	int position = 0;
	int count = 0;

	while (true) {
		int end = position;
		for (; table->columns[end] != '\t' && table->columns[end]; end++);
		count++;
		if (table->columns[end] == '\t') position = end + 1;
		else break;
	}

	UI_FREE(table->columnWidths);
	table->columnWidths = (int *) UI_MALLOC(count * sizeof(int));
	table->columnCount = count;

	position = 0;

	char buffer[256];
	UITableGetItem m = { 0 };
	m.buffer = buffer;
	m.bufferBytes = sizeof(buffer);

	while (true) {
		int end = position;
		for (; table->columns[end] != '\t' && table->columns[end]; end++);

		int longest = UIMeasureStringWidth(table->columns + position, end - position);

		for (int i = 0; i < table->itemCount; i++) {
			m.index = i;
			int bytes = UIElementMessage(&table->e, UI_MSG_TABLE_GET_ITEM, 0, &m);
			int width = UIMeasureStringWidth(buffer, bytes);

			if (width > longest) {
				longest = width;
			}
		}

		table->columnWidths[m.column] = longest;
		m.column++;
		if (table->columns[end] == '\t') position = end + 1;
		else break;
	}
}

static int uiTableMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UITable *table = (UITable *) element;

	if (message == UI_MSG_PAINT) {
		UIPainter *painter = (UIPainter *) dp;
		UIRectangle bounds = element->bounds;
		bounds.r -= UI_SIZE_SCROLL_BAR * element->window->scale;
		UIDrawBlock(painter, bounds, ui.theme.panel2);
		char buffer[256];
		UIRectangle row = bounds;
		int rowHeight = UI_SIZE_TABLE_ROW * element->window->scale;
		UITableGetItem m = { 0 };
		m.buffer = buffer;
		m.bufferBytes = sizeof(buffer);
		row.t += UI_SIZE_TABLE_HEADER * table->e.window->scale;
		row.t -= (int64_t) table->vScroll->position % rowHeight;
		int hovered = UITableHitTest(table, element->window->cursorX, element->window->cursorY);

		for (int i = table->vScroll->position / rowHeight; i < table->itemCount; i++) {
			if (row.t > element->clip.b) {
				break;
			}
			
			row.b = row.t + rowHeight;
			m.index = i;
			m.isSelected = false;
			m.column = 0;
			int bytes = UIElementMessage(element, UI_MSG_TABLE_GET_ITEM, 0, &m);
			uint32_t textColor = ui.theme.text;

			if (m.isSelected) {
				UIDrawBlock(painter, row, ui.theme.selected);
				textColor = ui.theme.textSelected;
			} else if (hovered == i) {
				UIDrawBlock(painter, row, ui.theme.buttonHovered);
			}

			UIRectangle cell = row;
			cell.l += UI_SIZE_TABLE_COLUMN_GAP * table->e.window->scale;

			for (int j = 0; j < table->columnCount; j++) {
				if (j) {
					m.column = j;
					bytes = UIElementMessage(element, UI_MSG_TABLE_GET_ITEM, 0, &m);
				}

				cell.r = cell.l + table->columnWidths[j];
				if ((size_t) bytes > m.bufferBytes && bytes > 0) bytes = m.bufferBytes;
				UIDrawString(painter, cell, buffer, bytes, textColor, UI_ALIGN_LEFT, NULL);
				cell.l += table->columnWidths[j] + UI_SIZE_TABLE_COLUMN_GAP * table->e.window->scale;
			}

			row.t += rowHeight;
		}

		UIRectangle header = bounds;
		header.b = header.t + UI_SIZE_TABLE_HEADER * table->e.window->scale;
		UIDrawRectangle(painter, header, ui.theme.panel1, ui.theme.border, UI_RECT_4(0, 0, 0, 1));
		header.l += UI_SIZE_TABLE_COLUMN_GAP * table->e.window->scale;

		int position = 0;
		int index = 0;

		if (table->columnCount) {
			while (true) {
				int end = position;
				for (; table->columns[end] != '\t' && table->columns[end]; end++);

				header.r = header.l + table->columnWidths[index];
				UIDrawString(painter, header, table->columns + position, end - position, ui.theme.text, UI_ALIGN_LEFT, NULL);
				if (index == table->columnHighlight) UIDrawInvert(painter, header);
				header.l += table->columnWidths[index] + UI_SIZE_TABLE_COLUMN_GAP * table->e.window->scale;

				if (table->columns[end] == '\t') {
					position = end + 1;
					index++;
				} else {
					break;
				}
			}
		}
	} else if (message == UI_MSG_LAYOUT) {
		UIRectangle scrollBarBounds = element->bounds;
		scrollBarBounds.l = scrollBarBounds.r - UI_SIZE_SCROLL_BAR * element->window->scale;
		table->vScroll->maximum = table->itemCount * UI_SIZE_TABLE_ROW * element->window->scale;
		table->vScroll->page = UI_RECT_HEIGHT(element->bounds) - UI_SIZE_TABLE_HEADER * table->e.window->scale;
		UIElementMove(&table->vScroll->e, scrollBarBounds, true);
	} else if (message == UI_MSG_MOUSE_MOVE || message == UI_MSG_UPDATE) {
		UIElementRepaint(element, NULL);
	} else if (message == UI_MSG_SCROLLED) {
		UIElementRefresh(element);
	} else if (message == UI_MSG_MOUSE_WHEEL) {
		return UIElementMessage(&table->vScroll->e, message, di, dp);
	} else if (message == UI_MSG_DESTROY) {
		UI_FREE(table->columns);
		UI_FREE(table->columnWidths);
	}

	return 0;
}

UITable *UITableCreate(UIElement *parent, uint32_t flags, const char *columns) {
	UITable *table = (UITable *) UIElementCreate(sizeof(UITable), parent, flags, uiTableMessage, "Table");
	table->vScroll = UIScrollBarCreate(&table->e, 0);
	table->columns = UIStringCopy(columns, -1);
	table->columnHighlight = -1;
	return table;
}
