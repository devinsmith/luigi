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

static void uiButtonCalculateColors(UIElement *element, uint32_t *color, uint32_t *textColor) {
	bool disabled = element->flags & UI_ELEMENT_DISABLED;
	bool focused = element == element->window->focused;
	bool pressed = element == element->window->pressed;
	bool hovered = element == element->window->hovered;
	*color = disabled ? ui.theme->buttonDisabled
		: (pressed && hovered) ? ui.theme->buttonPressed
		: (pressed || hovered) ? ui.theme->buttonHovered
		: focused ? ui.theme->selected : ui.theme->buttonNormal;
	*textColor = disabled ? ui.theme->textDisabled
		: *color == ui.theme->selected ? ui.theme->textSelected : ui.theme->text;
}


static int uiButtonMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIButton *button = (UIButton *) element;
	bool isMenuItem = element->flags & UI_BUTTON_MENU_ITEM;
	bool isDropDown = element->flags & UI_BUTTON_DROP_DOWN;
	
	if (message == UI_MSG_GET_HEIGHT) {
		if (isMenuItem) {
			return UI_SIZE_MENU_ITEM_HEIGHT * element->window->scale;
		} else {
			return UI_SIZE_BUTTON_HEIGHT * element->window->scale;
		}
	} else if (message == UI_MSG_GET_WIDTH) {
		int labelSize = UIMeasureStringWidth(button->label, button->labelBytes);
		int paddedSize = labelSize + UI_SIZE_BUTTON_PADDING * element->window->scale;
		if (isDropDown) paddedSize += ui.activeFont->glyphWidth * 2;
		int minimumSize = ((element->flags & UI_BUTTON_SMALL) ? 0 
				: isMenuItem ? UI_SIZE_MENU_ITEM_MINIMUM_WIDTH 
				: UI_SIZE_BUTTON_MINIMUM_WIDTH) 
			* element->window->scale;
		return paddedSize > minimumSize ? paddedSize : minimumSize;
	} else if (message == UI_MSG_PAINT) {
		UIPainter *painter = (UIPainter *) dp;

		uint32_t color, textColor;
    uiButtonCalculateColors(element, &color, &textColor);

		UIDrawRectangle(painter, element->bounds, color, ui.theme->border, UI_RECT_1(isMenuItem ? 0 : 1));

		if (element->flags & UI_BUTTON_CHECKED) {
			UIDrawBlock(painter, UIRectangleAdd(element->bounds, 
				UI_RECT_1I((int) (UI_SIZE_BUTTON_CHECKED_AREA * element->window->scale))), ui.theme->buttonPressed);
		}

		UIRectangle bounds = UIRectangleAdd(element->bounds, UI_RECT_2I((int) (UI_SIZE_MENU_ITEM_MARGIN * element->window->scale), 0));

		if (isMenuItem) {
			if (button->labelBytes == -1) {
				button->labelBytes = UIStringLength(button->label);
			}

			int tab = 0;
			for (; tab < button->labelBytes && button->label[tab] != '\t'; tab++);

			UIDrawString(painter, bounds, button->label, tab, textColor, UI_ALIGN_LEFT, NULL);

			if (button->labelBytes > tab) {
				UIDrawString(painter, bounds, button->label + tab + 1, button->labelBytes - tab - 1, textColor, UI_ALIGN_RIGHT, NULL);
			}
		} else if (isDropDown) {
			UIDrawString(painter, bounds, button->label, button->labelBytes, textColor, UI_ALIGN_LEFT, NULL);
			UIDrawString(painter, bounds, "\x19", 1, textColor, UI_ALIGN_RIGHT, NULL);
		} else {
			UIDrawString(painter, element->bounds, button->label, button->labelBytes, textColor, UI_ALIGN_CENTER, NULL);
		}
	} else if (message == UI_MSG_UPDATE) {
		UIElementRepaint(element, NULL);
	} else if (message == UI_MSG_DESTROY) {
		UI_FREE(button->label);
	} else if (message == UI_MSG_LEFT_DOWN) {
		if (element->flags & UI_BUTTON_CAN_FOCUS) {
			UIElementFocus(element);
		}
	} else if (message == UI_MSG_KEY_TYPED) {
		UIKeyTyped *m = (UIKeyTyped *) dp;
		
		if (m->textBytes == 1 && m->text[0] == ' ') {
			UIElementMessage(element, UI_MSG_CLICKED, 0, 0);
			UIElementRepaint(element, NULL);
		}
	} else if (message == UI_MSG_CLICKED) {
		if (button->invoke) {
			button->invoke(element->cp);
		}
	}

	return 0;
}

bool UIIsButton(const UIElement *element)
{
  return element->messageClass == uiButtonMessage;
}

UIButton *UIButtonCreate(UIElement *parent, uint32_t flags, const char *label, ptrdiff_t labelBytes) {
	UIButton *button = (UIButton *) UIElementCreate(sizeof(UIButton), parent, flags | UI_ELEMENT_TAB_STOP, uiButtonMessage, "Button");
	button->label = UIStringCopy(label, (button->labelBytes = labelBytes));
	return button;
}

static int uiCheckboxMessage(UIElement *element, UIMessage message, int di, void *dp) {
  UICheckbox *box = (UICheckbox *) element;

  if (message == UI_MSG_GET_HEIGHT) {
    return UI_SIZE_BUTTON_HEIGHT * element->window->scale;
  } else if (message == UI_MSG_GET_WIDTH) {
    int labelSize = UIMeasureStringWidth(box->label, box->labelBytes);
    return (labelSize + UI_SIZE_CHECKBOX_BOX + UI_SIZE_CHECKBOX_GAP) * element->window->scale;
  } else if (message == UI_MSG_PAINT) {
    UIPainter *painter = (UIPainter *) dp;
    uint32_t color, textColor;
    uiButtonCalculateColors(element, &color, &textColor);
    int midY = (element->bounds.t + element->bounds.b) / 2;
    UIRectangle boxBounds = UI_RECT_4(element->bounds.l, element->bounds.l + UI_SIZE_CHECKBOX_BOX,
                                      midY - UI_SIZE_CHECKBOX_BOX / 2, midY + UI_SIZE_CHECKBOX_BOX / 2);
    UIDrawRectangle(painter, boxBounds, color, ui.theme->border, UI_RECT_1(1));
    UIDrawString(painter, UIRectangleAdd(boxBounds, UI_RECT_4(1, 0, 0, 0)),
                 box->check == UI_CHECK_CHECKED ? "*" : box->check == UI_CHECK_INDETERMINATE ? "-" : " ", -1,
                 textColor, UI_ALIGN_CENTER, NULL);
    UIDrawString(painter, UIRectangleAdd(element->bounds, UI_RECT_4(UI_SIZE_CHECKBOX_BOX + UI_SIZE_CHECKBOX_GAP, 0, 0, 0)),
                 box->label, box->labelBytes, textColor, UI_ALIGN_LEFT, NULL);
  } else if (message == UI_MSG_UPDATE) {
    UIElementRepaint(element, NULL);
  } else if (message == UI_MSG_DESTROY) {
    UI_FREE(box->label);
  } else if (message == UI_MSG_KEY_TYPED) {
    UIKeyTyped *m = (UIKeyTyped *) dp;

    if (m->textBytes == 1 && m->text[0] == ' ') {
      UIElementMessage(element, UI_MSG_CLICKED, 0, 0);
      UIElementRepaint(element, NULL);
    }
  } else if (message == UI_MSG_CLICKED) {
    box->check = (box->check + 1) % ((element->flags & UI_CHECKBOX_ALLOW_INDETERMINATE) ? 3 : 2);
    UIElementRepaint(element, NULL);
    if (box->invoke) box->invoke(element->cp);
  }

  return 0;
}

UICheckbox *UICheckboxCreate(UIElement *parent, uint32_t flags, const char *label, ptrdiff_t labelBytes) {
  UICheckbox *box = (UICheckbox *) UIElementCreate(sizeof(UICheckbox), parent, flags | UI_ELEMENT_TAB_STOP, uiCheckboxMessage, "Checkbox");
  box->label = UIStringCopy(label, (box->labelBytes = labelBytes));
  return box;
}
