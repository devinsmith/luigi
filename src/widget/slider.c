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

static int uiSliderMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UISlider *slider = (UISlider *) element;

	if (message == UI_MSG_GET_HEIGHT) {
		return UI_SIZE_SLIDER_HEIGHT * element->window->scale;
	} else if (message == UI_MSG_GET_WIDTH) {
		return UI_SIZE_SLIDER_WIDTH * element->window->scale;
	} else if (message == UI_MSG_PAINT) {
		UIPainter *painter = (UIPainter *) dp;
		UIRectangle bounds = element->bounds;
		int centerY = (bounds.t + bounds.b) / 2;
		int trackSize = UI_SIZE_SLIDER_TRACK * element->window->scale;
		int thumbSize = UI_SIZE_SLIDER_THUMB * element->window->scale;
		int thumbPosition = (UI_RECT_WIDTH(bounds) - thumbSize) * slider->position;
		UIRectangle track = UI_RECT_4(bounds.l, bounds.r, centerY - (trackSize + 1) / 2, centerY + trackSize / 2);
		UIDrawRectangle(painter, track, ui.theme.buttonNormal, ui.theme.border, UI_RECT_1(1));
		bool pressed = element == element->window->pressed;
		bool hovered = element == element->window->hovered;
		bool disabled = element->flags & UI_ELEMENT_DISABLED;
		uint32_t color = disabled ? ui.theme.buttonDisabled : pressed ? ui.theme.buttonPressed : hovered ? ui.theme.buttonHovered : ui.theme.buttonNormal;
		UIRectangle thumb = UI_RECT_4(bounds.l + thumbPosition, bounds.l + thumbPosition + thumbSize, centerY - (thumbSize + 1) / 2, centerY + thumbSize / 2);
		UIDrawRectangle(painter, thumb, color, ui.theme.border, UI_RECT_1(1));
	} else if (message == UI_MSG_LEFT_DOWN || (message == UI_MSG_MOUSE_DRAG && element->window->pressedButton == 1)) {
		UIRectangle bounds = element->bounds;
		int thumbSize = UI_SIZE_SLIDER_THUMB * element->window->scale;
		slider->position = (float) (element->window->cursorX - thumbSize / 2 - bounds.l) / (UI_RECT_WIDTH(bounds) - thumbSize);
		if (slider->steps > 1) slider->position = (int) (slider->position * (slider->steps - 1) + 0.5f) / (float) (slider->steps - 1);
		if (slider->position < 0) slider->position = 0;
		if (slider->position > 1) slider->position = 1;
		UIElementMessage(element, UI_MSG_VALUE_CHANGED, 0, 0);
		UIElementRepaint(element, NULL);
	} else if (message == UI_MSG_UPDATE) {
		UIElementRepaint(element, NULL);
	}

	return 0;
}

UISlider *UISliderCreate(UIElement *parent, uint32_t flags) {
	return (UISlider *) UIElementCreate(sizeof(UISlider), parent, flags, uiSliderMessage, "Slider");
}

