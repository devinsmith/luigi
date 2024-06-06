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

static int uiGaugeMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIGauge *gauge = (UIGauge *) element;

	if (message == UI_MSG_GET_HEIGHT) {
		return UI_SIZE_GAUGE_HEIGHT * element->window->scale;
	} else if (message == UI_MSG_GET_WIDTH) {
		return UI_SIZE_GAUGE_WIDTH * element->window->scale;
	} else if (message == UI_MSG_PAINT) {
		UIPainter *painter = (UIPainter *) dp;
		UIDrawRectangle(painter, element->bounds, ui.theme.buttonNormal, ui.theme.border, UI_RECT_1(1));
		UIRectangle filled = UIRectangleAdd(element->bounds, UI_RECT_1I(1));
		filled.r = filled.l + UI_RECT_WIDTH(filled) * gauge->position;
		UIDrawBlock(painter, filled, ui.theme.selected);
	}

	return 0;
}

UIGauge *UIGaugeCreate(UIElement *parent, uint32_t flags) {
	return (UIGauge *) UIElementCreate(sizeof(UIGauge), parent, flags, uiGaugeMessage, "Gauge");
}
