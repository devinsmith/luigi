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

#ifndef LUIGI_PRIVATE_H
#define LUIGI_PRIVATE_H

#ifdef WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#endif

struct ui_ctx {
	UIWindow *windows;
	UIElement *animating;
	UITheme *theme;

	UIElement *parentStack[16];
	int parentStackCount;

	bool quit;
	const char *dialogResult;
	UIElement *dialogOldFocus;

	UIFont *activeFont;

#ifdef UI_DEBUG
	UIWindow *inspector;
	UITable *inspectorTable;
	UIWindow *inspectorTarget;
	UICode *inspectorLog;
#endif

#ifdef WIN32
	HCURSOR cursors[UI_CURSOR_COUNT];
	HANDLE heap;
	bool assertionFailure;
#else
  Display *display;
  Visual *visual;
  XIM xim;
  Atom windowClosedID, primaryID, uriListID, plainTextID;
  Atom dndEnterID, dndPositionID, dndStatusID, dndActionCopyID, dndDropID, dndSelectionID, dndFinishedID, dndAwareID;
  Atom clipboardID, xSelectionDataID, textID, targetID, incrID;
  Cursor cursors[UI_CURSOR_COUNT];
  char *pasteText;
  XEvent copyEvent;
#endif

#ifdef UI_FREETYPE
	FT_Library ft;
#endif
};

extern struct ui_ctx ui;

#ifndef WIN32
// Ehhhh...
UIWindow *X11FindWindow(Window window);
#endif

#ifdef UI_WINDOWS
void *_UIHeapReAlloc(void *pointer, size_t size);
#endif

#define UI_FREE UIFree

void _UIWindowEndPaint(UIWindow *window, UIPainter *painter);
void _UIWindowSetCursor(UIWindow *window, int cursor);
void _UIWindowGetScreenPosition(UIWindow *window, int *x, int *y);
void _UIWindowSetPressed(UIWindow *window, UIElement *element, int button);
bool _UIMessageLoopSingle(int *result);
void _UIInspectorRefresh();
void _UIUpdate();

#endif // LUIGI_PRIVATE_H
