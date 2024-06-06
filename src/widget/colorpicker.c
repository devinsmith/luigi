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

#include <xmmintrin.h>

typedef union uiConvertFloatInteger {
  float f;
  uint32_t i;
} uiConvertFloatInteger;

static float uiFloorFloat(float x) {
  uiConvertFloatInteger convert = {x};
  uint32_t sign = convert.i & 0x80000000;
  int exponent = (int) ((convert.i >> 23) & 0xFF) - 0x7F;

  if (exponent >= 23) {
    // There aren't any bits representing a fractional part.
  } else if (exponent >= 0) {
    // Positive exponent.
    uint32_t mask = 0x7FFFFF >> exponent;
    if (!(mask & convert.i)) return x; // Already an integer.
    if (sign) convert.i += mask;
    convert.i &= ~mask; // Mask out the fractional bits.
  } else if (exponent < 0) {
    // Negative exponent.
    return sign ? -1.0 : 0.0;
  }

  return convert.f;
}

static float uiSquareRootFloat(float x) {
  float result[4];
  _mm_storeu_ps(result, _mm_sqrt_ps(_mm_set_ps(0, 0, 0, x)));
  return result[0];
}

#define _F(x) (((uiConvertFloatInteger) { .i = (x) }).f)

static float uiArcTanFloatI(float x) {
  float x2 = x * x;
  return x * (_F(0x3F7FFFF8) + x2 * (_F(0xBEAAA53C) + x2 * (_F(0x3E4BC990) + x2 * (_F(0xBE084A60) + x2 * _F(0x3D8864B0)))));
}

static float uiSinFloatI(float x) {
  float x2 = x * x;
  return x * (_F(0x3F800000) + x2 * (_F(0xBE2AAAA0) + x2 * (_F(0x3C0882C0) + x2 * _F(0xB94C6000))));
}

static float uiCosFloatI(float x) {
  float x2 = x * x;
  return _F(0x3F800000) + x2 * (_F(0xBEFFFFDA) + x2 * (_F(0x3D2A9F60) + x2 * _F(0xBAB22C00)));
}

#undef _F

static float uiSinFloat(float x) {
  bool negate = false;
  if (x < 0) { x = -x; negate = true; }
  x -= 2 * 3.141592654f * uiFloorFloat(x / (2 * 3.141592654f));
  if (x < 3.141592654f / 2) {}
  else if (x < 3.141592654f) { x = 3.141592654f - x; }
  else if (x < 3 * 3.141592654f / 2) { x = x - 3.141592654f; negate = !negate; }
  else { x = 3.141592654f * 2 - x; negate = !negate; }
  float y = x < 3.141592654f / 4 ? uiSinFloatI(x) : uiCosFloatI(3.141592654f / 2 - x);
  return negate ? -y : y;
}

static float uiCosFloat(float x) {
  return uiSinFloat(3.141592654f / 2 - x);
}

static float uiArcTanFloat(float x) {
  bool negate = false, reciprocalTaken = false;
  if (x < 0) { x = -x; negate = true; }
  if (x > 1) { x = 1 / x; reciprocalTaken = true; }
  float y = x < 0.5f ? uiArcTanFloatI(x) : (0.463647609f + uiArcTanFloatI((2 * x - 1) / (2 + x)));
  if (reciprocalTaken) { y = 3.141592654f / 2 - y; }
  return negate ? -y : y;
}

static float uiArcTan2Float(float y, float x) {
  if (x == 0) return y > 0 ? 3.141592654f / 2 : -3.141592654f / 2;
  else if (x > 0) return uiArcTanFloat(y / x);
  else if (y >= 0) return 3.141592654f + uiArcTanFloat(y / x);
  else return -3.141592654f + uiArcTanFloat(y / x);
}

static int uiColorCircleMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIColorPicker *colorPicker = (UIColorPicker *) element->parent;

	if (message == UI_MSG_PAINT) {
		UIPainter *painter = (UIPainter *) dp;

		int startY = element->bounds.t, endY = element->bounds.b;
		int startX = element->bounds.l, endX = element->bounds.r;
		int size = endY - startY;

		for (int i = startY; i < endY; i++) {
			uint32_t *out = painter->bits + i * painter->width + startX;
			int j = startX;
			float y0 = i - startY - size / 2, x0 = -size / 2;
			float angle = uiArcTan2Float((i - startY) * 2.0f / size - 1, -1);

			do {
				float distanceFromCenterSquared = x0 * x0 + y0 * y0;
				float hue = (angle + 3.14159f) * 0.954929658f;
				float saturation = uiSquareRootFloat(distanceFromCenterSquared * 4.0f / size / size);

				if (saturation <= 1 && UIRectangleContains(painter->clip, j, i)) {
					UIColorToRGB(hue, saturation, colorPicker->value, out);
					*out |= 0xFF000000;
				}

				out++, j++, x0++;

				if (distanceFromCenterSquared) {
					angle -= y0 / distanceFromCenterSquared;
				} else {
					angle = uiArcTan2Float((i - startY) * 2.0f / size - 1, 0.01f);
				}
			} while (j < endX);
		}

		float angle = colorPicker->hue / 0.954929658f - 3.14159f;
		float radius = colorPicker->saturation * size / 2;
		int cx = (startX + endX) / 2 + radius * uiCosFloat(angle);
		int cy = (startY + endY) / 2 + radius * uiSinFloat(angle);
		UIDrawInvert(painter, UI_RECT_4(cx - 1, cx + 1, startY, endY));
		UIDrawInvert(painter, UI_RECT_4(startX, endX, cy - 1, cy + 1));
	} else if (message == UI_MSG_GET_CURSOR) {
		return UI_CURSOR_CROSS_HAIR;
	} else if (message == UI_MSG_LEFT_DOWN || message == UI_MSG_MOUSE_DRAG) {
		int startY = element->bounds.t, endY = element->bounds.b, cursorY = element->window->cursorY;
		int startX = element->bounds.l, endX = element->bounds.r, cursorX = element->window->cursorX;
		int dx = (startX + endX) / 2, dy = (startY + endY) / 2;
		int size = endY - startY;

		float angle = uiArcTan2Float((cursorY - startY) * 2.0f / size - 1, (cursorX - startX) * 2.0f / size - 1);
		float distanceFromCenterSquared = (cursorX - dx) * (cursorX - dx) + (cursorY - dy) * (cursorY - dy);
		colorPicker->hue = (angle + 3.14159f) * 0.954929658f;
		colorPicker->saturation = uiSquareRootFloat(distanceFromCenterSquared * 4.0f / size / size);;
		if (colorPicker->saturation > 1) colorPicker->saturation = 1;

		UIElementMessage(&colorPicker->e, UI_MSG_VALUE_CHANGED, 0, 0);
		UIElementRepaint(&colorPicker->e, NULL);
	}

	return 0;
}

static int uiColorSliderMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIColorPicker *colorPicker = (UIColorPicker *) element->parent;
	float opacitySlider = element->flags & 1;

	if (message == UI_MSG_PAINT) {
		UIPainter *painter = (UIPainter *) dp;

		int startY = element->bounds.t, endY = element->bounds.b;
		int startX = element->bounds.l, endX = element->bounds.r;
		int size = endY - startY;

		for (int i = startY; i < endY; i++) {
			if (i < painter->clip.t || i >= painter->clip.b) continue;
			uint32_t *out = painter->bits + i * painter->width + startX;
			int j = element->clip.l;
			uint32_t color;
			float p = 1.0f - (float) (i - startY) / size;

			if (opacitySlider) {
				UIColorToRGB(colorPicker->hue, colorPicker->saturation, colorPicker->value, &color);
				color = UI_COLOR_FROM_FLOAT(p * (UI_COLOR_RED_F(color) - 0.5f) + 0.5f, 
					p * (UI_COLOR_GREEN_F(color) - 0.5f) + 0.5f, 
					p * (UI_COLOR_BLUE_F(color) - 0.5f) + 0.5f);
			} else {
				UIColorToRGB(colorPicker->hue, colorPicker->saturation, p, &color);
			}

			color |= 0xFF000000;

			do {
				*out = color;
				out++, j++;
			} while (j < element->clip.r);
		}

		int cy = (size - 1) * (1 - (opacitySlider ? colorPicker->opacity : colorPicker->value)) + startY;
		UIDrawInvert(painter, UI_RECT_4(startX, endX, cy - 1, cy + 1));
	} else if (message == UI_MSG_GET_CURSOR) {
		return UI_CURSOR_CROSS_HAIR;
	} else if (message == UI_MSG_LEFT_DOWN || message == UI_MSG_MOUSE_DRAG) {
		int startY = element->bounds.t, endY = element->bounds.b, cursorY = element->window->cursorY;
		float *value = opacitySlider ? &colorPicker->opacity : &colorPicker->value;
		*value = 1 - (float) (cursorY - startY) / (endY - startY);
		if (*value < 0) *value = 0;
		if (*value > 1) *value = 1;
		UIElementMessage(&colorPicker->e, UI_MSG_VALUE_CHANGED, 0, 0);
		UIElementRepaint(&colorPicker->e, NULL);
	}

	return 0;
}

static int uiColorPickerMessage(UIElement *element, UIMessage message, int di, void *dp) {
	bool hasOpacity = element->flags & UI_COLOR_PICKER_HAS_OPACITY;

	if (message == UI_MSG_GET_WIDTH) {
		return (hasOpacity ? 280 : 240) * element->window->scale;
	} else if (message == UI_MSG_GET_HEIGHT) {
		return 200 * element->window->scale;
	} else if (message == UI_MSG_LAYOUT) {
		UIRectangle bounds = element->bounds;

		int sliderSize = 35 * element->window->scale;
		int gap = 5 * element->window->scale;

		if (hasOpacity) {
			UIElementMove(element->children, UI_RECT_4(bounds.l, bounds.r - (sliderSize + gap) * 2, bounds.t, bounds.b), false);
			UIElementMove(element->children->next, UI_RECT_4(bounds.r - sliderSize * 2 - gap, bounds.r - sliderSize - gap, bounds.t, bounds.b), false);
			UIElementMove(element->children->next->next, UI_RECT_4(bounds.r - sliderSize, bounds.r, bounds.t, bounds.b), false);
		} else {
			UIElementMove(element->children, UI_RECT_4(bounds.l, bounds.r - sliderSize - gap, bounds.t, bounds.b), false);
			UIElementMove(element->children->next, UI_RECT_4(bounds.r - sliderSize, bounds.r, bounds.t, bounds.b), false);
		}
	}

	return 0;
}

bool UIColorToHSV(uint32_t rgb, float *hue, float *saturation, float *value) {
  float r = UI_COLOR_RED_F(rgb);
  float g = UI_COLOR_GREEN_F(rgb);
  float b = UI_COLOR_BLUE_F(rgb);

  float maximum = (r > g && r > b) ? r : (g > b ? g : b),
    minimum = (r < g && r < b) ? r : (g < b ? g : b),
    difference = maximum - minimum;
  *value = maximum;

  if (!difference) {
    *saturation = 0;
    return false;
  } else {
    if (r == maximum) *hue = (g - b) / difference + 0;
    if (g == maximum) *hue = (b - r) / difference + 2;
    if (b == maximum) *hue = (r - g) / difference + 4;
    if (*hue < 0) *hue += 6;
    *saturation = difference / maximum;
    return true;
  }
}

void UIColorToRGB(float h, float s, float v, uint32_t *rgb) {
  float r, g, b;

  if (!s) {
    r = g = b = v;
  } else {
    int h0 = ((int) h) % 6;
    float f = h - uiFloorFloat(h);
    float x = v * (1 - s), y = v * (1 - s * f), z = v * (1 - s * (1 - f));

    switch (h0) {
      case 0:  r = v, g = z, b = x; break;
      case 1:  r = y, g = v, b = x; break;
      case 2:  r = x, g = v, b = z; break;
      case 3:  r = x, g = y, b = v; break;
      case 4:  r = z, g = x, b = v; break;
      default: r = v, g = x, b = y; break;
    }
  }

  *rgb = UI_COLOR_FROM_FLOAT(r, g, b);
}


UIColorPicker *UIColorPickerCreate(UIElement *parent, uint32_t flags) {
	UIColorPicker *colorPicker = (UIColorPicker *) UIElementCreate(sizeof(UIColorPicker), parent, flags, uiColorPickerMessage, "ColorPicker");
	UIElementCreate(sizeof(UIElement), &colorPicker->e, 0, uiColorCircleMessage, "ColorCircle");
	UIElementCreate(sizeof(UIElement), &colorPicker->e, 0, uiColorSliderMessage, "ColorSlider");

	if (flags & UI_COLOR_PICKER_HAS_OPACITY) {
		UIElementCreate(sizeof(UIElement), &colorPicker->e, 1, uiColorSliderMessage, "ColorSlider");
	}

	return colorPicker;
}
