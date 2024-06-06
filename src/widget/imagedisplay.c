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

static float uiLinearMap(float value, float inFrom, float inTo, float outFrom, float outTo) {
  float inRange = inTo - inFrom, outRange = outTo - outFrom;
  float normalisedValue = (value - inFrom) / inRange;
  return normalisedValue * outRange + outFrom;
}

static void uiImageDisplayUpdateViewport(UIImageDisplay *display) {
	UIRectangle bounds = display->e.bounds;
	bounds.r -= bounds.l, bounds.b -= bounds.t;
	
	float minimumZoomX = 1, minimumZoomY = 1;
	if (display->width  > bounds.r) minimumZoomX = (float) bounds.r / display->width;
	if (display->height > bounds.b) minimumZoomY = (float) bounds.b / display->height;
	float minimumZoom = minimumZoomX < minimumZoomY ? minimumZoomX : minimumZoomY;
	
	if (display->zoom < minimumZoom || (display->e.flags & _UI_IMAGE_DISPLAY_ZOOM_FIT)) {
		display->zoom = minimumZoom;
		display->e.flags |= _UI_IMAGE_DISPLAY_ZOOM_FIT;
	}
	
	if (display->panX < 0) display->panX = 0;
	if (display->panY < 0) display->panY = 0;
	if (display->panX > display->width  - bounds.r / display->zoom) display->panX = display->width  - bounds.r / display->zoom;
	if (display->panY > display->height - bounds.b / display->zoom) display->panY = display->height - bounds.b / display->zoom;
	
	if (bounds.r && display->width  * display->zoom <= bounds.r) display->panX = display->width  / 2 - bounds.r / display->zoom / 2;
	if (bounds.b && display->height * display->zoom <= bounds.b) display->panY = display->height / 2 - bounds.b / display->zoom / 2;
}

static int uiImageDisplayMessage(UIElement *element, UIMessage message, int di, void *dp) {
	UIImageDisplay *display = (UIImageDisplay *) element;
	
	if (message == UI_MSG_GET_HEIGHT) {
		return display->height;
	} else if (message == UI_MSG_GET_WIDTH) {
		return display->width;
	} else if (message == UI_MSG_DESTROY) {
		UI_FREE(display->bits);
	} else if (message == UI_MSG_PAINT) {
		UIPainter *painter = (UIPainter *) dp;
		
		int w = UI_RECT_WIDTH(element->bounds), h = UI_RECT_HEIGHT(element->bounds);
		int x = uiLinearMap(0, display->panX, display->panX + w / display->zoom, 0, w) + element->bounds.l;
		int y = uiLinearMap(0, display->panY, display->panY + h / display->zoom, 0, h) + element->bounds.t;
		
		UIRectangle image = UI_RECT_4(x, x + (int) (display->width * display->zoom), y, (int) (y + display->height * display->zoom));
		UIRectangle bounds = UIRectangleIntersection(painter->clip, UIRectangleIntersection(display->e.bounds, image));
		if (!UI_RECT_VALID(bounds)) return 0;
		
		if (display->zoom == 1) {
			uint32_t *lineStart = (uint32_t *) painter->bits + bounds.t * painter->width + bounds.l;
			uint32_t *sourceLineStart = display->bits + (bounds.l - image.l) + display->width * (bounds.t - image.t);
	
			for (int i = 0; i < bounds.b - bounds.t; i++, lineStart += painter->width, sourceLineStart += display->width) {
				uint32_t *destination = lineStart;
				uint32_t *source = sourceLineStart;
				int j = bounds.r - bounds.l;
	
				do {
					*destination = *source;
					destination++;
					source++;
				} while (--j);
			}
		} else {
			float zr = 1.0f / display->zoom;
			uint32_t *destination = (uint32_t *) painter->bits;
			
			for (int i = bounds.t; i < bounds.b; i++) {
				int ty = (i - image.t) * zr;
				
				for (int j = bounds.l; j < bounds.r; j++) {
					int tx = (j - image.l) * zr;
					destination[i * painter->width + j] = display->bits[ty * display->width + tx];
				}
			}
		}
	} else if (message == UI_MSG_MOUSE_WHEEL && (element->flags & UI_IMAGE_DISPLAY_INTERACTIVE)) {
		display->e.flags &= ~_UI_IMAGE_DISPLAY_ZOOM_FIT;
		int divisions = -di / 72;
		float factor = 1;
		float perDivision = element->window->ctrl ? 2.0f : element->window->alt ? 1.01f : 1.2f;
		while (divisions > 0) factor *= perDivision, divisions--;
		while (divisions < 0) factor /= perDivision, divisions++;
		if (display->zoom * factor > 64) factor = 64 / display->zoom;
		int mx = element->window->cursorX - element->bounds.l;
		int my = element->window->cursorY - element->bounds.t;
		display->zoom *= factor;
		display->panX -= mx / display->zoom * (1 - factor);
		display->panY -= my / display->zoom * (1 - factor);
		uiImageDisplayUpdateViewport(display);
		UIElementRepaint(&display->e, NULL);
	} else if (message == UI_MSG_LAYOUT && (element->flags & UI_IMAGE_DISPLAY_INTERACTIVE)) {
		UIRectangle bounds = display->e.bounds;
		bounds.r -= bounds.l, bounds.b -= bounds.t;
		display->panX -= (bounds.r - display->previousWidth ) / 2 / display->zoom;
		display->panY -= (bounds.b - display->previousHeight) / 2 / display->zoom;
		display->previousWidth = bounds.r, display->previousHeight = bounds.b;
		uiImageDisplayUpdateViewport(display);
	} else if (message == UI_MSG_GET_CURSOR && (element->flags & UI_IMAGE_DISPLAY_INTERACTIVE)
			&& (UI_RECT_WIDTH(element->bounds) < display->width * display->zoom 
				|| UI_RECT_HEIGHT(element->bounds) < display->height * display->zoom)) {
		return UI_CURSOR_HAND;
	} else if (message == UI_MSG_MOUSE_DRAG) {
		display->panX -= (element->window->cursorX - display->previousPanPointX) / display->zoom;
		display->panY -= (element->window->cursorY - display->previousPanPointY) / display->zoom;
		uiImageDisplayUpdateViewport(display);
		display->previousPanPointX = element->window->cursorX;
		display->previousPanPointY = element->window->cursorY;
		UIElementRepaint(element, NULL);
	} else if (message == UI_MSG_LEFT_DOWN) {           
		display->e.flags &= ~_UI_IMAGE_DISPLAY_ZOOM_FIT;  
		display->previousPanPointX = element->window->cursorX;
		display->previousPanPointY = element->window->cursorY;
	}

	return 0;
}

void UIImageDisplaySetContent(UIImageDisplay *display, uint32_t *bits, size_t width, size_t height, size_t stride) {
	UI_FREE(display->bits);

	display->bits = (uint32_t *) UI_MALLOC(width * height * 4);
	display->width = width;
	display->height = height;

	uint32_t *destination = display->bits;
	uint32_t *source = bits;

	for (uintptr_t row = 0; row < height; row++, source += stride / 4) {
		for (uintptr_t i = 0; i < width; i++) {
			*destination++ = source[i];
		}
	}
}

UIImageDisplay *UIImageDisplayCreate(UIElement *parent, uint32_t flags, uint32_t *bits, size_t width, size_t height, size_t stride) {
	UIImageDisplay *display = (UIImageDisplay *) UIElementCreate(sizeof(UIImageDisplay), parent, flags, uiImageDisplayMessage, "ImageDisplay");
	display->zoom = 1.0f;
	UIImageDisplaySetContent(display, bits, width, height, stride);
	return display;
}
