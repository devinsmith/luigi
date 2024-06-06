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

static int uiLabelMessage(UIElement *element, UIMessage message, int di, void *dp) {
  UILabel *label = (UILabel *) element;

  if (message == UI_MSG_GET_HEIGHT) {
    return UIMeasureStringHeight();
  } else if (message == UI_MSG_GET_WIDTH) {
    return UIMeasureStringWidth(label->label, label->labelBytes);
  } else if (message == UI_MSG_PAINT) {
    UIPainter *painter = (UIPainter *) dp;
    UIDrawString(painter, element->bounds, label->label, label->labelBytes, ui.theme.text, UI_ALIGN_LEFT, NULL);
  } else if (message == UI_MSG_DESTROY) {
    UIFree(label->label);
  }

  return 0;
}

void UILabelSetContent(UILabel *label, const char *string, ptrdiff_t stringBytes) {
  UIFree(label->label);
  label->label = UIStringCopy(string, (label->labelBytes = stringBytes));
}

UILabel *UILabelCreate(UIElement *parent, uint32_t flags, const char *string, ptrdiff_t stringBytes) {
  UILabel *label = (UILabel *) UIElementCreate(sizeof(UILabel), parent, flags, uiLabelMessage, "Label");
  label->label = UIStringCopy(string, (label->labelBytes = stringBytes));
  return label;
}
