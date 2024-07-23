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

#include "luigi.h"
#include "luigi_private.h"

#ifdef UI_WINDOWS

const int UI_KEYCODE_A = 'A';
const int UI_KEYCODE_0 = '0';
const int UI_KEYCODE_BACKSPACE = VK_BACK;
const int UI_KEYCODE_DELETE = VK_DELETE;
const int UI_KEYCODE_DOWN = VK_DOWN;
const int UI_KEYCODE_END = VK_END;
const int UI_KEYCODE_ENTER = VK_RETURN;
const int UI_KEYCODE_ESCAPE = VK_ESCAPE;
const int UI_KEYCODE_F1 = VK_F1;
const int UI_KEYCODE_HOME = VK_HOME;
const int UI_KEYCODE_LEFT = VK_LEFT;
const int UI_KEYCODE_RIGHT = VK_RIGHT;
const int UI_KEYCODE_SPACE = VK_SPACE;
const int UI_KEYCODE_TAB = VK_TAB;
const int UI_KEYCODE_UP = VK_UP;
const int UI_KEYCODE_INSERT = VK_INSERT;

int _UIWindowMessage(UIElement *element, UIMessage message, int di, void *dp) {
	if (message == UI_MSG_DESTROY) {
		UIWindow *window = (UIWindow *) element;
		_UIWindowDestroyCommon(window);
		SetWindowLongPtr(window->hwnd, GWLP_USERDATA, 0);
		DestroyWindow(window->hwnd);
	}

	return _UIWindowMessageCommon(element, message, di, dp);
}

LRESULT CALLBACK _UIWindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	UIWindow *window = (UIWindow *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (!window || ui.assertionFailure) {
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	if (message == WM_CLOSE) {
		if (UIElementMessage(&window->e, UI_MSG_WINDOW_CLOSE, 0, 0)) {
			_UIUpdate();
			return 0;
		} else {
			PostQuitMessage(0);
		}
	} else if (message == WM_SIZE) {
		RECT client;
		GetClientRect(hwnd, &client);
		window->width = client.right;
		window->height = client.bottom;
		window->bits = (uint32_t *) UI_REALLOC(window->bits, window->width * window->height * 4);
		window->e.bounds = UI_RECT_2S(window->width, window->height);
		window->e.clip = UI_RECT_2S(window->width, window->height);
		UIElementMessage(&window->e, UI_MSG_LAYOUT, 0, 0);
		_UIUpdate();
	} else if (message == WM_MOUSEMOVE) {
		if (!window->trackingLeave) {
			window->trackingLeave = true;
			TRACKMOUSEEVENT leave = { 0 };
			leave.cbSize = sizeof(TRACKMOUSEEVENT);
			leave.dwFlags = TME_LEAVE;
			leave.hwndTrack = hwnd;
			TrackMouseEvent(&leave);
		}

		POINT cursor;
		GetCursorPos(&cursor);
		ScreenToClient(hwnd, &cursor);
		window->cursorX = cursor.x;
		window->cursorY = cursor.y;
		_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
	} else if (message == WM_MOUSELEAVE) {
		window->trackingLeave = false;

		if (!window->pressed) {
			window->cursorX = -1;
			window->cursorY = -1;
		}

		_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
	} else if (message == WM_LBUTTONDOWN) {
		SetCapture(hwnd);
		_UIWindowInputEvent(window, UI_MSG_LEFT_DOWN, 0, 0);
	} else if (message == WM_LBUTTONUP) {
		if (window->pressedButton == 1) ReleaseCapture();
		_UIWindowInputEvent(window, UI_MSG_LEFT_UP, 0, 0);
	} else if (message == WM_MBUTTONDOWN) {
		SetCapture(hwnd);
		_UIWindowInputEvent(window, UI_MSG_MIDDLE_DOWN, 0, 0);
	} else if (message == WM_MBUTTONUP) {
		if (window->pressedButton == 2) ReleaseCapture();
		_UIWindowInputEvent(window, UI_MSG_MIDDLE_UP, 0, 0);
	} else if (message == WM_RBUTTONDOWN) {
		SetCapture(hwnd);
		_UIWindowInputEvent(window, UI_MSG_RIGHT_DOWN, 0, 0);
	} else if (message == WM_RBUTTONUP) {
		if (window->pressedButton == 3) ReleaseCapture();
		_UIWindowInputEvent(window, UI_MSG_RIGHT_UP, 0, 0);
	} else if (message == WM_MOUSEWHEEL) {
		int delta = (int) wParam >> 16;
		_UIWindowInputEvent(window, UI_MSG_MOUSE_WHEEL, -delta, 0);
	} else if (message == WM_KEYDOWN) {
		window->ctrl = GetKeyState(VK_CONTROL) & 0x8000;
		window->shift = GetKeyState(VK_SHIFT) & 0x8000;
		window->alt = GetKeyState(VK_MENU) & 0x8000;

		UIKeyTyped m = { 0 };
		m.code = wParam;
		_UIWindowInputEvent(window, UI_MSG_KEY_TYPED, 0, &m);
	} else if (message == WM_CHAR) {
		UIKeyTyped m = { 0 };
		char c = wParam;
		m.text = &c;
		m.textBytes = 1;
		_UIWindowInputEvent(window, UI_MSG_KEY_TYPED, 0, &m);
	} else if (message == WM_PAINT) {
		PAINTSTRUCT paint;
		HDC dc = BeginPaint(hwnd, &paint);
		BITMAPINFOHEADER info = { 0 };
		info.biSize = sizeof(info);
		info.biWidth = window->width, info.biHeight = -window->height;
		info.biPlanes = 1, info.biBitCount = 32;
		StretchDIBits(dc, 0, 0, UI_RECT_SIZE(window->e.bounds), 0, 0, UI_RECT_SIZE(window->e.bounds),
			window->bits, (BITMAPINFO *) &info, DIB_RGB_COLORS, SRCCOPY);
		EndPaint(hwnd, &paint);
	} else if (message == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT) {
		SetCursor(ui.cursors[window->cursorStyle]);
		return 1;
	} else if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
		_UIMenusClose();

		if (message == WM_SETFOCUS) {
			_UIInspectorSetFocusedWindow(window);
			UIElementMessage(&window->e, UI_MSG_WINDOW_ACTIVATE, 0, 0);
		}
	} else if (message == WM_MOUSEACTIVATE && (window->e.flags & UI_WINDOW_MENU)) {
		return MA_NOACTIVATE;
	} else if (message == WM_DROPFILES) {
		HDROP drop = (HDROP) wParam;
		int count = DragQueryFile(drop, 0xFFFFFFFF, NULL, 0);
		char **files = (char **) UI_MALLOC(sizeof(char *) * count);
		
		for (int i = 0; i < count; i++) {
			int length = DragQueryFile(drop, i, NULL, 0);
			files[i] = (char *) UI_MALLOC(length + 1);
			files[i][length] = 0;
			DragQueryFile(drop, i, files[i], length + 1);
		}
		
		UIElementMessage(&window->e, UI_MSG_WINDOW_DROP_FILES, count, files);
		for (int i = 0; i < count; i++) UI_FREE(files[i]);		
		UI_FREE(files);
		DragFinish(drop);
		_UIUpdate();
	} else if (message == WM_APP + 1) {
		UIElementMessage(&window->e, (UIMessage) wParam, 0, (void *) lParam);
		_UIUpdate();
	} else {
		if (message == WM_NCLBUTTONDOWN || message == WM_NCMBUTTONDOWN || message == WM_NCRBUTTONDOWN) {
			if (~window->e.flags & UI_WINDOW_MENU) {
				_UIMenusClose();
				_UIUpdate();
			}
		}

		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}

void UIInitialise() {
	ui.heap = GetProcessHeap();
	
	_UIInitialiseCommon();

	ui.cursors[UI_CURSOR_ARROW] = LoadCursor(NULL, IDC_ARROW);
	ui.cursors[UI_CURSOR_TEXT] = LoadCursor(NULL, IDC_IBEAM);
	ui.cursors[UI_CURSOR_SPLIT_V] = LoadCursor(NULL, IDC_SIZENS);
	ui.cursors[UI_CURSOR_SPLIT_H] = LoadCursor(NULL, IDC_SIZEWE);
	ui.cursors[UI_CURSOR_FLIPPED_ARROW] = LoadCursor(NULL, IDC_ARROW);
	ui.cursors[UI_CURSOR_CROSS_HAIR] = LoadCursor(NULL, IDC_CROSS);
	ui.cursors[UI_CURSOR_HAND] = LoadCursor(NULL, IDC_HAND);
	ui.cursors[UI_CURSOR_RESIZE_UP] = LoadCursor(NULL, IDC_SIZENS);
	ui.cursors[UI_CURSOR_RESIZE_LEFT] = LoadCursor(NULL, IDC_SIZEWE);
	ui.cursors[UI_CURSOR_RESIZE_UP_RIGHT] = LoadCursor(NULL, IDC_SIZENESW);
	ui.cursors[UI_CURSOR_RESIZE_UP_LEFT] = LoadCursor(NULL, IDC_SIZENWSE);
	ui.cursors[UI_CURSOR_RESIZE_DOWN] = LoadCursor(NULL, IDC_SIZENS);
	ui.cursors[UI_CURSOR_RESIZE_RIGHT] = LoadCursor(NULL, IDC_SIZEWE);
	ui.cursors[UI_CURSOR_RESIZE_DOWN_LEFT] = LoadCursor(NULL, IDC_SIZENESW);
	ui.cursors[UI_CURSOR_RESIZE_DOWN_RIGHT] = LoadCursor(NULL, IDC_SIZENWSE);

	WNDCLASS windowClass = { 0 };
	windowClass.lpfnWndProc = _UIWindowProcedure;
	windowClass.lpszClassName = "normal";
	RegisterClass(&windowClass);
	windowClass.style |= CS_DROPSHADOW;
	windowClass.lpszClassName = "shadow";
	RegisterClass(&windowClass);
}

bool _UIMessageLoopSingle(int *result) {
	MSG message = { 0 };

	if (ui.animating) {
		if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT) {
				*result = message.wParam;
				return false;
			}

			TranslateMessage(&message);
			DispatchMessage(&message);
		} else {
			_UIProcessAnimations();
		}
	} else {
		if (!GetMessage(&message, NULL, 0, 0)) {
			*result = message.wParam;
			return false;
		}

		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return true;
}

void UIMenuShow(UIMenu *menu) {
	int width, height;
	_UIMenuPrepare(menu, &width, &height);
	MoveWindow(menu->e.window->hwnd, menu->pointX, menu->pointY, width, height, FALSE);
	ShowWindow(menu->e.window->hwnd, SW_SHOWNOACTIVATE);
}

UIWindow *UIWindowCreate(UIWindow *owner, uint32_t flags, const char *cTitle, int width, int height) {
	_UIMenusClose();

	UIWindow *window = (UIWindow *) UIElementCreate(sizeof(UIWindow), NULL, flags | UI_ELEMENT_WINDOW, _UIWindowMessage, "Window");
	_UIWindowAdd(window);
	if (owner) window->scale = owner->scale;

	if (flags & UI_WINDOW_MENU) {
		UI_ASSERT(owner);

		window->hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_NOACTIVATE, "shadow", 0, WS_POPUP, 
			0, 0, 0, 0, owner->hwnd, NULL, NULL, NULL);
	} else {
		window->hwnd = CreateWindowEx(WS_EX_ACCEPTFILES, "normal", cTitle, WS_OVERLAPPEDWINDOW, 
			CW_USEDEFAULT, CW_USEDEFAULT, width ? width : CW_USEDEFAULT, height ? height : CW_USEDEFAULT,
			owner ? owner->hwnd : NULL, NULL, NULL, NULL);
	}

	SetWindowLongPtr(window->hwnd, GWLP_USERDATA, (LONG_PTR) window);

	if (~flags & UI_WINDOW_MENU) {
		ShowWindow(window->hwnd, SW_SHOW);
		PostMessage(window->hwnd, WM_SIZE, 0, 0);
	}

	return window;
}

void _UIWindowEndPaint(UIWindow *window, UIPainter *painter) {
	HDC dc = GetDC(window->hwnd);
	BITMAPINFOHEADER info = { 0 };
	info.biSize = sizeof(info);
	info.biWidth = window->width, info.biHeight = window->height;
	info.biPlanes = 1, info.biBitCount = 32;
	StretchDIBits(dc, 
		UI_RECT_TOP_LEFT(window->updateRegion), UI_RECT_SIZE(window->updateRegion), 
		window->updateRegion.l, window->updateRegion.b + 1, 
		UI_RECT_WIDTH(window->updateRegion), -UI_RECT_HEIGHT(window->updateRegion),
		window->bits, (BITMAPINFO *) &info, DIB_RGB_COLORS, SRCCOPY);
	ReleaseDC(window->hwnd, dc);
}

void _UIWindowSetCursor(UIWindow *window, int cursor) {
	SetCursor(ui.cursors[cursor]);
}

void _UIWindowGetScreenPosition(UIWindow *window, int *_x, int *_y) {
	POINT p;
	p.x = 0;
	p.y = 0;
	ClientToScreen(window->hwnd, &p);
	*_x = p.x;
	*_y = p.y;
}

void UIWindowPostMessage(UIWindow *window, UIMessage message, void *_dp) {
	PostMessage(window->hwnd, WM_APP + 1, (WPARAM) message, (LPARAM) _dp);
}

void *_UIHeapReAlloc(void *pointer, size_t size) {
	if (pointer) {
		if (size) {
			return HeapReAlloc(ui.heap, 0, pointer, size);
		} else {
			UI_FREE(pointer);
			return NULL;
		}
	} else {
		if (size) {
			return UI_MALLOC(size);
		} else {
			return NULL;
		}
	}
}

#endif
