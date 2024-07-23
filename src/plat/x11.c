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

#ifdef UI_LINUX

const int UI_KEYCODE_A = XK_a;
const int UI_KEYCODE_BACKSPACE = XK_BackSpace;
const int UI_KEYCODE_DELETE = XK_Delete;
const int UI_KEYCODE_DOWN = XK_Down;
const int UI_KEYCODE_END = XK_End;
const int UI_KEYCODE_ENTER = XK_Return;
const int UI_KEYCODE_ESCAPE = XK_Escape;
const int UI_KEYCODE_F1 = XK_F1;
const int UI_KEYCODE_HOME = XK_Home;
const int UI_KEYCODE_LEFT = XK_Left;
const int UI_KEYCODE_RIGHT = XK_Right;
const int UI_KEYCODE_SPACE = XK_space;
const int UI_KEYCODE_TAB = XK_Tab;
const int UI_KEYCODE_UP = XK_Up;
const int UI_KEYCODE_INSERT = XK_Insert;
const int UI_KEYCODE_0 = XK_0;

int _UIWindowMessage(UIElement *element, UIMessage message, int di, void *dp) {
	if (message == UI_MSG_DESTROY) {
		UIWindow *window = (UIWindow *) element;
		_UIWindowDestroyCommon(window);
		window->image->data = NULL;
		XDestroyImage(window->image);
		XDestroyIC(window->xic);
		XDestroyWindow(ui.display, ((UIWindow *) element)->window);
	}

	return _UIWindowMessageCommon(element, message, di, dp);
}

UIWindow *UIWindowCreate(UIWindow *owner, uint32_t flags, const char *cTitle, int _width, int _height) {
	_UIMenusClose();

	UIWindow *window = (UIWindow *) UIElementCreate(sizeof(UIWindow), NULL, flags | UI_ELEMENT_WINDOW, _UIWindowMessage, "Window");
	_UIWindowAdd(window);
	if (owner) window->scale = owner->scale;

	int width = (flags & UI_WINDOW_MENU) ? 1 : _width ? _width : 800;
	int height = (flags & UI_WINDOW_MENU) ? 1 : _height ? _height : 600;

	XSetWindowAttributes attributes = {};
	attributes.override_redirect = flags & UI_WINDOW_MENU;

	window->window = XCreateWindow(ui.display, DefaultRootWindow(ui.display), 0, 0, width, height, 0, 0, 
		InputOutput, CopyFromParent, CWOverrideRedirect, &attributes);
	if (cTitle) XStoreName(ui.display, window->window, cTitle);
	XSelectInput(ui.display, window->window, SubstructureNotifyMask | ExposureMask | PointerMotionMask 
		| ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask
		| EnterWindowMask | LeaveWindowMask | ButtonMotionMask | KeymapStateMask | FocusChangeMask | PropertyChangeMask);

	if (flags & UI_WINDOW_MAXIMIZE) {
		Atom atoms[2] = { XInternAtom(ui.display, "_NET_WM_STATE_MAXIMIZED_HORZ", 0), XInternAtom(ui.display, "_NET_WM_STATE_MAXIMIZED_VERT", 0) };
		XChangeProperty(ui.display, window->window, XInternAtom(ui.display, "_NET_WM_STATE", 0), XA_ATOM, 32, PropModeReplace, (unsigned char *) atoms, 2);
	}

	if (~flags & UI_WINDOW_MENU) {
		XMapRaised(ui.display, window->window);
	}

	if (flags & UI_WINDOW_CENTER_IN_OWNER) {
		int x = 0, y = 0;
		_UIWindowGetScreenPosition(owner, &x, &y);
		XMoveResizeWindow(ui.display, window->window, x + owner->width / 2 - width / 2, y + owner->height / 2 - height / 2, width, height);
	}

	XSetWMProtocols(ui.display, window->window, &ui.windowClosedID, 1);
	window->image = XCreateImage(ui.display, ui.visual, 24, ZPixmap, 0, NULL, 10, 10, 32, 0);

	window->xic = XCreateIC(ui.xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, window->window, XNFocusWindow, window->window, NULL);

	int dndVersion = 4;
	XChangeProperty(ui.display, window->window, ui.dndAwareID, XA_ATOM, 32 /* bits */, PropModeReplace, (uint8_t *) &dndVersion, 1);

	return window;
}

Display *_UIX11GetDisplay() {
	return ui.display;
}

UIWindow *X11FindWindow(Window window) {
	UIWindow *w = ui.windows;

	while (w) {
		if (w->window == window) {
			return w;
		}

		w = w->next;
	}

	return NULL;
}

void UIInitialise() {
	_UIInitialiseCommon();

	XInitThreads();

	ui.display = XOpenDisplay(NULL);
	ui.visual = XDefaultVisual(ui.display, 0);

	ui.windowClosedID = XInternAtom(ui.display, "WM_DELETE_WINDOW", 0);
	ui.primaryID = XInternAtom(ui.display, "PRIMARY", 0);
	ui.dndEnterID = XInternAtom(ui.display, "XdndEnter", 0);
	ui.dndPositionID = XInternAtom(ui.display, "XdndPosition", 0);
	ui.dndStatusID = XInternAtom(ui.display, "XdndStatus", 0);
	ui.dndActionCopyID = XInternAtom(ui.display, "XdndActionCopy", 0);
	ui.dndDropID = XInternAtom(ui.display, "XdndDrop", 0);
	ui.dndSelectionID = XInternAtom(ui.display, "XdndSelection", 0);
	ui.dndFinishedID = XInternAtom(ui.display, "XdndFinished", 0);
	ui.dndAwareID = XInternAtom(ui.display, "XdndAware", 0);
	ui.uriListID = XInternAtom(ui.display, "text/uri-list", 0);
	ui.plainTextID = XInternAtom(ui.display, "text/plain", 0);
	ui.clipboardID = XInternAtom(ui.display, "CLIPBOARD", 0);
	ui.xSelectionDataID = XInternAtom(ui.display, "XSEL_DATA", 0);
	ui.textID = XInternAtom(ui.display, "TEXT", 0);
	ui.targetID = XInternAtom(ui.display, "TARGETS", 0);
	ui.incrID = XInternAtom(ui.display, "INCR", 0);

	ui.cursors[UI_CURSOR_ARROW] = XCreateFontCursor(ui.display, XC_left_ptr);
	ui.cursors[UI_CURSOR_TEXT] = XCreateFontCursor(ui.display, XC_xterm);
	ui.cursors[UI_CURSOR_SPLIT_V] = XCreateFontCursor(ui.display, XC_sb_v_double_arrow);
	ui.cursors[UI_CURSOR_SPLIT_H] = XCreateFontCursor(ui.display, XC_sb_h_double_arrow);
	ui.cursors[UI_CURSOR_FLIPPED_ARROW] = XCreateFontCursor(ui.display, XC_right_ptr);
	ui.cursors[UI_CURSOR_CROSS_HAIR] = XCreateFontCursor(ui.display, XC_crosshair);
	ui.cursors[UI_CURSOR_HAND] = XCreateFontCursor(ui.display, XC_hand1);
	ui.cursors[UI_CURSOR_RESIZE_UP] = XCreateFontCursor(ui.display, XC_top_side);
	ui.cursors[UI_CURSOR_RESIZE_LEFT] = XCreateFontCursor(ui.display, XC_left_side);
	ui.cursors[UI_CURSOR_RESIZE_UP_RIGHT] = XCreateFontCursor(ui.display, XC_top_right_corner);
	ui.cursors[UI_CURSOR_RESIZE_UP_LEFT] = XCreateFontCursor(ui.display, XC_top_left_corner);
	ui.cursors[UI_CURSOR_RESIZE_DOWN] = XCreateFontCursor(ui.display, XC_bottom_side);
	ui.cursors[UI_CURSOR_RESIZE_RIGHT] = XCreateFontCursor(ui.display, XC_right_side);
	ui.cursors[UI_CURSOR_RESIZE_DOWN_LEFT] = XCreateFontCursor(ui.display, XC_bottom_left_corner);
	ui.cursors[UI_CURSOR_RESIZE_DOWN_RIGHT] = XCreateFontCursor(ui.display, XC_bottom_right_corner);

	XSetLocaleModifiers("");

	ui.xim = XOpenIM(ui.display, 0, 0, 0);

	if(!ui.xim){
		XSetLocaleModifiers("@im=none");
		ui.xim = XOpenIM(ui.display, 0, 0, 0);
	}
}

void _UIWindowSetCursor(UIWindow *window, int cursor) {
	XDefineCursor(ui.display, window->window, ui.cursors[cursor]);
}

void _UIX11ResetCursor(UIWindow *window) {
	XDefineCursor(ui.display, window->window, ui.cursors[UI_CURSOR_ARROW]);
}

void _UIWindowEndPaint(UIWindow *window, UIPainter *painter) {
	(void) painter;

	XPutImage(ui.display, window->window, DefaultGC(ui.display, 0), window->image, 
		UI_RECT_TOP_LEFT(window->updateRegion), UI_RECT_TOP_LEFT(window->updateRegion),
		UI_RECT_SIZE(window->updateRegion));
}

void _UIWindowGetScreenPosition(UIWindow *window, int *_x, int *_y) {
	Window child;
	XTranslateCoordinates(ui.display, window->window, DefaultRootWindow(ui.display), 0, 0, _x, _y, &child);
}

void UIMenuShow(UIMenu *menu) {
	int width, height;
	_UIMenuPrepare(menu, &width, &height);

	for (int i = 0; i < ScreenCount(ui.display); i++) {
		Screen *screen = ScreenOfDisplay(ui.display, i);

		int x, y;
		Window child;
		XTranslateCoordinates(ui.display, screen->root, DefaultRootWindow(ui.display), 0, 0, &x, &y, &child);

		if (menu->pointX >= x && menu->pointX < x + screen->width 
				&& menu->pointY >= y && menu->pointY < y + screen->height) {
			if (menu->pointX + width > x + screen->width) menu->pointX = x + screen->width - width;
			if (menu->pointY + height > y + screen->height) menu->pointY = y + screen->height - height;
			if (menu->pointX < x) menu->pointX = x;
			if (menu->pointY < y) menu->pointY = y;
			if (menu->pointX + width > x + screen->width) width = x + screen->width - menu->pointX;
			if (menu->pointY + height > y + screen->height) height = y + screen->height - menu->pointY;
			break;
		}
	}

	Atom properties[] = {
		XInternAtom(ui.display, "_NET_WM_WINDOW_TYPE", true),
		XInternAtom(ui.display, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", true),
		XInternAtom(ui.display, "_MOTIF_WM_HINTS", true),
	};

	XChangeProperty(ui.display, menu->e.window->window, properties[0], XA_ATOM, 32, PropModeReplace, (uint8_t *) properties, 2);
	XSetTransientForHint(ui.display, menu->e.window->window, DefaultRootWindow(ui.display));

	struct Hints {
		int flags;
		int functions;
		int decorations;
		int inputMode;
		int status;
	};

	struct Hints hints = { 0 };
	hints.flags = 2;
	XChangeProperty(ui.display, menu->e.window->window, properties[2], properties[2], 32, PropModeReplace, (uint8_t *) &hints, 5);

	XMapWindow(ui.display, menu->e.window->window);
	XMoveResizeWindow(ui.display, menu->e.window->window, menu->pointX, menu->pointY, width, height);
}

void UIWindowPack(UIWindow *window, int _width) {
	int width = _width ? _width : UIElementMessage(window->e.children, UI_MSG_GET_WIDTH, 0, 0);
	int height = UIElementMessage(window->e.children, UI_MSG_GET_HEIGHT, width, 0);
	XResizeWindow(ui.display, window->window, width, height);
}

bool _UIProcessEvent(XEvent *event) {
	if (event->type == ClientMessage && (Atom) event->xclient.data.l[0] == ui.windowClosedID) {
		UIWindow *window = X11FindWindow(event->xclient.window);
		if (!window) return false;
		bool exit = !UIElementMessage(&window->e, UI_MSG_WINDOW_CLOSE, 0, 0);
		if (exit) return true;
		_UIUpdate();
		return false;
	} else if (event->type == Expose) {
		UIWindow *window = X11FindWindow(event->xexpose.window);
		if (!window) return false;
		XPutImage(ui.display, window->window, DefaultGC(ui.display, 0), window->image, 0, 0, 0, 0, window->width, window->height);
	} else if (event->type == ConfigureNotify) {
		UIWindow *window = X11FindWindow(event->xconfigure.window);
		if (!window) return false;

		if (window->width != event->xconfigure.width || window->height != event->xconfigure.height) {
			window->width = event->xconfigure.width;
			window->height = event->xconfigure.height;
			window->bits = (uint32_t *) UI_REALLOC(window->bits, window->width * window->height * 4);
			window->image->width = window->width;
			window->image->height = window->height;
			window->image->bytes_per_line = window->width * 4;
			window->image->data = (char *) window->bits;
			window->e.bounds = UI_RECT_2S(window->width, window->height);
			window->e.clip = UI_RECT_2S(window->width, window->height);
#ifdef UI_DEBUG
			for (int i = 0; i < window->width * window->height; i++) window->bits[i] = 0xFF00FF;
#endif
			UIElementMessage(&window->e, UI_MSG_LAYOUT, 0, 0);
			_UIUpdate();
		}
	} else if (event->type == MotionNotify) {
		UIWindow *window = X11FindWindow(event->xmotion.window);
		if (!window) return false;
		window->cursorX = event->xmotion.x;
		window->cursorY = event->xmotion.y;
		_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
	} else if (event->type == LeaveNotify) {
		UIWindow *window = X11FindWindow(event->xcrossing.window);
		if (!window) return false;

		if (!window->pressed) {
			window->cursorX = -1;
			window->cursorY = -1;
		}

		_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
	} else if (event->type == ButtonPress || event->type == ButtonRelease) {
		UIWindow *window = X11FindWindow(event->xbutton.window);
		if (!window) return false;
		window->cursorX = event->xbutton.x;
		window->cursorY = event->xbutton.y;

		if (event->xbutton.button >= 1 && event->xbutton.button <= 3) {
			_UIWindowInputEvent(window, (UIMessage) ((event->type == ButtonPress ? UI_MSG_LEFT_DOWN : UI_MSG_LEFT_UP) 
				+ event->xbutton.button * 2 - 2), 0, 0);
		} else if (event->xbutton.button == 4) {
			_UIWindowInputEvent(window, UI_MSG_MOUSE_WHEEL, -72, 0);
		} else if (event->xbutton.button == 5) {
			_UIWindowInputEvent(window, UI_MSG_MOUSE_WHEEL, 72, 0);
		}

		_UIInspectorSetFocusedWindow(window);
	} else if (event->type == KeyPress) {
		UIWindow *window = X11FindWindow(event->xkey.window);
		if (!window) return false;

		if (event->xkey.x == 0x7123 && event->xkey.y == 0x7456) {
			// HACK! See UIWindowPostMessage.
			UIElementMessage(&window->e, (UIMessage) event->xkey.state, 0, 
				(void *) (((uintptr_t) (event->xkey.time & 0xFFFFFFFF) << 32) 
					| ((uintptr_t) (event->xkey.x_root & 0xFFFF) << 0) 
					| ((uintptr_t) (event->xkey.y_root & 0xFFFF) << 16)));
			_UIUpdate();
		} else {
			char text[32];
			KeySym symbol = NoSymbol;
			Status status;
			// printf("%ld, %s\n", symbol, text);
			UIKeyTyped m = { 0 };
			m.textBytes = Xutf8LookupString(window->xic, &event->xkey, text, sizeof(text) - 1, &symbol, &status); 
			m.text = text;
			m.code = XLookupKeysym(&event->xkey, 0);

			if (symbol == XK_Control_L || symbol == XK_Control_R) {
				window->ctrl = true;
				window->ctrlCode = event->xkey.keycode;
				_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
			} else if (symbol == XK_Shift_L || symbol == XK_Shift_R) {
				window->shift = true;
				window->shiftCode = event->xkey.keycode;
				_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
			} else if (symbol == XK_Alt_L || symbol == XK_Alt_R) {
				window->alt = true;
				window->altCode = event->xkey.keycode;
				_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
			} else if (symbol == XK_KP_Left) {
				m.code = UI_KEYCODE_LEFT;
			} else if (symbol == XK_KP_Right) {
				m.code = UI_KEYCODE_RIGHT;
			} else if (symbol == XK_KP_Up) {
				m.code = UI_KEYCODE_UP;
			} else if (symbol == XK_KP_Down) {
				m.code = UI_KEYCODE_DOWN;
			} else if (symbol == XK_KP_Home) {
				m.code = UI_KEYCODE_HOME;
			} else if (symbol == XK_KP_End) {
				m.code = UI_KEYCODE_END;
			} else if (symbol == XK_KP_Enter) {
				m.code = UI_KEYCODE_ENTER;
			} else if (symbol == XK_KP_Delete) {
				m.code = UI_KEYCODE_DELETE;
			}

			_UIWindowInputEvent(window, UI_MSG_KEY_TYPED, 0, &m);
		}
	} else if (event->type == KeyRelease) {
		UIWindow *window = X11FindWindow(event->xkey.window);
		if (!window) return false;

		if (event->xkey.keycode == window->ctrlCode) {
			window->ctrl = false;
			_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
		} else if (event->xkey.keycode == window->shiftCode) {
			window->shift = false;
			_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
		} else if (event->xkey.keycode == window->altCode) {
			window->alt = false;
			_UIWindowInputEvent(window, UI_MSG_MOUSE_MOVE, 0, 0);
		}
	} else if (event->type == FocusIn) {
		UIWindow *window = X11FindWindow(event->xfocus.window);
		if (!window) return false;
		window->ctrl = window->shift = window->alt = false;
		UIElementMessage(&window->e, UI_MSG_WINDOW_ACTIVATE, 0, 0);
	} else if (event->type == ClientMessage && event->xclient.message_type == ui.dndEnterID) {
		UIWindow *window = X11FindWindow(event->xclient.window);
		if (!window) return false;
		window->dragSource = (Window) event->xclient.data.l[0];
	} else if (event->type == ClientMessage && event->xclient.message_type == ui.dndPositionID) {
		UIWindow *window = X11FindWindow(event->xclient.window);
		if (!window) return false;
		XClientMessageEvent m = { 0 };
		m.type = ClientMessage;
		m.display = event->xclient.display;
		m.window = (Window) event->xclient.data.l[0];
		m.message_type = ui.dndStatusID;
		m.format = 32;
		m.data.l[0] = window->window;
		m.data.l[1] = true;
		m.data.l[4] = ui.dndActionCopyID;
		XSendEvent(ui.display, m.window, False, NoEventMask, (XEvent *) &m);
		XFlush(ui.display);
	} else if (event->type == ClientMessage && event->xclient.message_type == ui.dndDropID) {
		UIWindow *window = X11FindWindow(event->xclient.window);
		if (!window) return false;

		// TODO Dropping text.

		if (!XConvertSelection(ui.display, ui.dndSelectionID, ui.uriListID, ui.primaryID, window->window, event->xclient.data.l[2])) {
			XClientMessageEvent m = { 0 };
			m.type = ClientMessage;
			m.display = ui.display;
			m.window = window->dragSource;
			m.message_type = ui.dndFinishedID;
			m.format = 32;
			m.data.l[0] = window->window;
			m.data.l[1] = 0;
			m.data.l[2] = ui.dndActionCopyID;
			XSendEvent(ui.display, m.window, False, NoEventMask, (XEvent *) &m);
			XFlush(ui.display);
		}
	} else if (event->type == SelectionNotify) {
		UIWindow *window = X11FindWindow(event->xselection.requestor);
		if (!window) return false;
		if (!window->dragSource) return false;

		Atom type = None;
		int format = 0;
		uint64_t count = 0, bytesLeft = 0;
		uint8_t *data = NULL;
		XGetWindowProperty(ui.display, window->window, ui.primaryID, 0, 65536, False, AnyPropertyType, &type, &format, &count, &bytesLeft, &data);

		if (format == 8 /* bits per character */) {
			if (event->xselection.target == ui.uriListID) {
				char *copy = (char *) UI_MALLOC(count);
				int fileCount = 0;

				for (int i = 0; i < (int) count; i++) {
					copy[i] = data[i];

					if (i && data[i - 1] == '\r' && data[i] == '\n') {
						fileCount++;
					}
				}

				char **files = (char **) UI_MALLOC(sizeof(char *) * fileCount);
				fileCount = 0;

				for (int i = 0; i < (int) count; i++) {
					char *s = copy + i;
					while (!(i && data[i - 1] == '\r' && data[i] == '\n' && i < (int) count)) i++;
					copy[i - 1] = 0;

					for (int j = 0; s[j]; j++) {
						if (s[j] == '%' && s[j + 1] && s[j + 2]) {
							char n[3];
							n[0] = s[j + 1], n[1] = s[j + 2], n[2] = 0;
							s[j] = strtol(n, NULL, 16);
							if (!s[j]) break;
							memmove(s + j + 1, s + j + 3, strlen(s) - j - 2);
						}
					}

					if (s[0] == 'f' && s[1] == 'i' && s[2] == 'l' && s[3] == 'e' && s[4] == ':' && s[5] == '/' && s[6] == '/') {
						files[fileCount++] = s + 7;
					}
				}

				UIElementMessage(&window->e, UI_MSG_WINDOW_DROP_FILES, fileCount, files);

				UI_FREE(files);
				UI_FREE(copy);
			} else if (event->xselection.target == ui.plainTextID) {
				// TODO.
			}
		}

		XFree(data);

		XClientMessageEvent m = { 0 };
		m.type = ClientMessage;
		m.display = ui.display;
		m.window = window->dragSource;
		m.message_type = ui.dndFinishedID;
		m.format = 32;
		m.data.l[0] = window->window;
		m.data.l[1] = true;
		m.data.l[2] = ui.dndActionCopyID;
		XSendEvent(ui.display, m.window, False, NoEventMask, (XEvent *) &m);
		XFlush(ui.display);

		window->dragSource = 0; // Drag complete.
		_UIUpdate();
	} else if (event->type == SelectionRequest) {
		UIWindow *window = X11FindWindow(event->xclient.window);
		if (!window) return false;

		if ((XGetSelectionOwner(ui.display, ui.clipboardID) == window->window) 
				&& (event->xselectionrequest.selection == ui.clipboardID)) {
			XSelectionRequestEvent requestEvent = event->xselectionrequest;
			Atom utf8ID = XInternAtom(ui.display, "UTF8_STRING", 1);
			if (utf8ID == None) utf8ID = XA_STRING;

			Atom type = requestEvent.target;
			type = (type == ui.textID) ? XA_STRING : type;
			int changePropertyResult = 0;

			if(requestEvent.target == XA_STRING || requestEvent.target == ui.textID || requestEvent.target == utf8ID) {
				changePropertyResult = XChangeProperty(requestEvent.display, requestEvent.requestor, requestEvent.property, 
						type, 8, PropModeReplace, (const unsigned char *) ui.pasteText, strlen(ui.pasteText));
			} else if (requestEvent.target == ui.targetID) {
				changePropertyResult = XChangeProperty(requestEvent.display, requestEvent.requestor, requestEvent.property, 
						XA_ATOM, 32, PropModeReplace, (unsigned char *) &utf8ID, 1);
			}

			if(changePropertyResult == 0 || changePropertyResult == 1) {
				XSelectionEvent sendEvent = {
					.type = SelectionNotify,
					.serial = requestEvent.serial,
					.send_event = requestEvent.send_event,
					.display = requestEvent.display,
					.requestor = requestEvent.requestor,
					.selection = requestEvent.selection,
					.target = requestEvent.target,
					.property = requestEvent.property,
					.time = requestEvent.time
				};

				XSendEvent(ui.display, requestEvent.requestor, 0, 0, (XEvent *) &sendEvent);
			}
		}
	}

	return false;
}

bool _UIMessageLoopSingle(int *result) {
	XEvent events[64];

	if (ui.animating) {
		if (XPending(ui.display)) {
			XNextEvent(ui.display, events + 0);
		} else {
			_UIProcessAnimations();
			return true;
		}
	} else {
		XNextEvent(ui.display, events + 0);
	}

	int p = 1;

	int configureIndex = -1, motionIndex = -1, exposeIndex = -1;

	while (p < 64 && XPending(ui.display)) {
		XNextEvent(ui.display, events + p);

#define _UI_MERGE_EVENTS(a, b) \
	if (events[p].type == a) { \
		if (b != -1) events[b].type = 0; \
		b = p; \
	}

		_UI_MERGE_EVENTS(ConfigureNotify, configureIndex);
		_UI_MERGE_EVENTS(MotionNotify, motionIndex);
		_UI_MERGE_EVENTS(Expose, exposeIndex);

		p++;
	}

	for (int i = 0; i < p; i++) {
		if (!events[i].type) {
			continue;
		}

		if (_UIProcessEvent(events + i)) {
			return false;
		}
	}

	return true;
}

void UIWindowPostMessage(UIWindow *window, UIMessage message, void *_dp) {
	// HACK! Xlib doesn't seem to have a nice way to do this,
	// so send a specially crafted key press event instead.
	// TODO Maybe ClientMessage is what this should use?
	uintptr_t dp = (uintptr_t) _dp;
	XKeyEvent event = { 0 };
	event.display = ui.display;
	event.window = window->window;
	event.root = DefaultRootWindow(ui.display);
	event.subwindow = None;
	event.time = dp >> 32;
	event.x = 0x7123;
	event.y = 0x7456;
	event.x_root = (dp >> 0) & 0xFFFF;
	event.y_root = (dp >> 16) & 0xFFFF;
	event.same_screen = True;
	event.keycode = 1;
	event.state = message;
	event.type = KeyPress;
	XSendEvent(ui.display, window->window, True, KeyPressMask, (XEvent *) &event);
	XFlush(ui.display);
}

#endif
