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

// TODO UITextbox features - mouse input, multi-line, clipboard, undo, IME support, number dragging.
// TODO New elements - list view, menu bar.
// TODO Keyboard navigation - menus, dialogs, tables.
// TODO Easier to use fonts; GDI font support.
// TODO Formalize the notion of size-stability? See _UIExpandPaneButtonInvoke.

#ifndef LUIGI_H
#define LUIGI_H

#include <config.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#define _UI_TO_STRING_1(x) #x
#define _UI_TO_STRING_2(x) _UI_TO_STRING_1(x)

// Branch prediction optimization
#if (__GNUC__ >= 3)
#define __likely(cond)    __builtin_expect(!!(cond),1)
#define __unlikely(cond)  __builtin_expect(!!(cond),0)
#else
#define __likely(cond)    (!!(cond))
#define __unlikely(cond)  (!!(cond))
#endif

#ifndef NDEBUG
#define UI_ASSERT(exp) (__likely(exp)?((void)0):(void)UIAssert(#exp,__FILE__,__LINE__))
#else
#define UI_ASSERT(x) ((void)0)
#endif

#define UI_FREE UIFree

#ifdef WIN32
#undef _UNICODE
#undef UNICODE
#include <windows.h>
#include <shellapi.h>

#define UI_CALLOC(x) HeapAlloc(ui.heap, HEAP_ZERO_MEMORY, (x))
#define UI_MALLOC(x) HeapAlloc(ui.heap, 0, (x))
#define UI_REALLOC _UIHeapReAlloc
#define UI_CLOCK GetTickCount
#define UI_CLOCKS_PER_SECOND (1000)
#define UI_CLOCK_T DWORD
#else // Unix/Linux
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include <xmmintrin.h>

#define UI_CALLOC(x) calloc(1, (x))
#define UI_MALLOC malloc
#define UI_REALLOC realloc
#define UI_CLOCK clock
#define UI_CLOCKS_PER_SECOND CLOCKS_PER_SEC
#define UI_CLOCK_T clock_t
#endif // WIN32

#ifdef UI_DEBUG
#include <stdio.h>
#endif

#ifdef UI_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftbitmap.h>
#endif

#define UI_SIZE_BUTTON_MINIMUM_WIDTH (100)
#define UI_SIZE_BUTTON_PADDING (16)
#define UI_SIZE_BUTTON_HEIGHT (27)
#define UI_SIZE_BUTTON_CHECKED_AREA (4)

#define UI_SIZE_CHECKBOX_BOX (14)
#define UI_SIZE_CHECKBOX_GAP (8)

#define UI_SIZE_MENU_ITEM_HEIGHT (24)
#define UI_SIZE_MENU_ITEM_MINIMUM_WIDTH (160)
#define UI_SIZE_MENU_ITEM_MARGIN (9)

#define UI_SIZE_GAUGE_WIDTH (200)
#define UI_SIZE_GAUGE_HEIGHT (22)

#define UI_SIZE_SLIDER_WIDTH (200)
#define UI_SIZE_SLIDER_HEIGHT (25)
#define UI_SIZE_SLIDER_THUMB (15)
#define UI_SIZE_SLIDER_TRACK (5)

#define UI_SIZE_TEXTBOX_MARGIN (3)
#define UI_SIZE_TEXTBOX_WIDTH (200)
#define UI_SIZE_TEXTBOX_HEIGHT (27)

#define UI_SIZE_TAB_PANE_SPACE_TOP (2)
#define UI_SIZE_TAB_PANE_SPACE_LEFT (4)

#define UI_SIZE_SPLITTER (8)

#define UI_SIZE_SCROLL_BAR (16)
#define UI_SIZE_SCROLL_MINIMUM_THUMB (20)

#define UI_SIZE_CODE_MARGIN (ui.activeFont->glyphWidth * 5)
#define UI_SIZE_CODE_MARGIN_GAP (ui.activeFont->glyphWidth * 1)

#define UI_SIZE_TABLE_HEADER (26)
#define UI_SIZE_TABLE_COLUMN_GAP (20)
#define UI_SIZE_TABLE_ROW (20)

#define UI_SIZE_PANE_MEDIUM_BORDER (5)
#define UI_SIZE_PANE_MEDIUM_GAP (5)
#define UI_SIZE_PANE_SMALL_BORDER (3)
#define UI_SIZE_PANE_SMALL_GAP (3)

#define UI_SIZE_MDI_CHILD_BORDER (6)
#define UI_SIZE_MDI_CHILD_TITLE (30)
#define UI_SIZE_MDI_CHILD_CORNER (12)
#define UI_SIZE_MDI_CHILD_MINIMUM_WIDTH (100)
#define UI_SIZE_MDI_CHILD_MINIMUM_HEIGHT (50)
#define UI_SIZE_MDI_CASCADE (30)

#define UI_UPDATE_HOVERED (1)
#define UI_UPDATE_PRESSED (2)
#define UI_UPDATE_FOCUSED (3)

typedef enum UIMessage {
	UI_MSG_PAINT, // dp = pointer to UIPainter
	UI_MSG_LAYOUT,
	UI_MSG_DESTROY,
	UI_MSG_UPDATE, // di = UI_UPDATE_... constant
	UI_MSG_ANIMATE,
	UI_MSG_SCROLLED,
	UI_MSG_GET_WIDTH, // di = height (if known); return width
	UI_MSG_GET_HEIGHT, // di = width (if known); return height
	UI_MSG_FIND_BY_POINT, // dp = pointer to UIFindByPoint; return 1 if handled
	UI_MSG_CLIENT_PARENT, // dp = pointer to UIElement *, set it to the parent for client elements

	UI_MSG_INPUT_EVENTS_START, // not sent to disabled elements
	UI_MSG_LEFT_DOWN,
	UI_MSG_LEFT_UP,
	UI_MSG_MIDDLE_DOWN,
	UI_MSG_MIDDLE_UP,
	UI_MSG_RIGHT_DOWN,
	UI_MSG_RIGHT_UP,
	UI_MSG_KEY_TYPED, // dp = pointer to UIKeyTyped; return 1 if handled
	UI_MSG_MOUSE_MOVE,
	UI_MSG_MOUSE_DRAG,
	UI_MSG_MOUSE_WHEEL, // di = delta; return 1 if handled
	UI_MSG_CLICKED,
	UI_MSG_GET_CURSOR, // return cursor code
	UI_MSG_PRESSED_DESCENDENT, // dp = pointer to child that is/contains pressed element
	UI_MSG_INPUT_EVENTS_END,

	UI_MSG_VALUE_CHANGED, // sent to notify that the element's value has changed
	UI_MSG_TABLE_GET_ITEM, // dp = pointer to UITableGetItem; return string length
	UI_MSG_CODE_GET_MARGIN_COLOR, // di = line index (starts at 1); return color
	UI_MSG_CODE_DECORATE_LINE, // dp = pointer to UICodeDecorateLine
	UI_MSG_WINDOW_CLOSE, // return 1 to prevent default (process exit for UIWindow; close for UIMDIChild)
	UI_MSG_TAB_SELECTED, // sent to the tab that was selected (not the tab pane itself)
	UI_MSG_WINDOW_DROP_FILES, // di = count, dp = char ** of paths
	UI_MSG_WINDOW_ACTIVATE,

	UI_MSG_USER,
} UIMessage;

typedef struct UIRectangle {
	int l, r, t, b;
} UIRectangle;

typedef struct UITheme {
	uint32_t panel1, panel2, selected, border;
	uint32_t text, textDisabled, textSelected;
	uint32_t buttonNormal, buttonHovered, buttonPressed, buttonDisabled;
	uint32_t textboxNormal, textboxFocused;
	uint32_t codeFocused, codeBackground, codeDefault, codeComment, codeString, codeNumber, codeOperator, codePreprocessor;
} UITheme;

typedef struct UIPainter {
	UIRectangle clip;
	uint32_t *bits;
	int width, height;
#ifdef UI_DEBUG
	int fillCount;
#endif
} UIPainter;

typedef struct UIFont {
	int glyphWidth, glyphHeight;

#ifdef UI_FREETYPE
	bool isFreeType;
	FT_Face font;
	FT_Bitmap glyphs[128];
	bool glyphsRendered[128];
	int glyphOffsetsX[128], glyphOffsetsY[128];
#endif
} UIFont;

typedef struct UIShortcut {
	intptr_t code;
	bool ctrl, shift, alt;
	void (*invoke)(void *cp);
	void *cp;
} UIShortcut;

typedef struct UIStringSelection {
	int carets[2];
	uint32_t colorText, colorBackground;
} UIStringSelection;

typedef struct UIKeyTyped {
	char *text;
	int textBytes;
	intptr_t code;
} UIKeyTyped;

typedef struct UITableGetItem {
	char *buffer;
	size_t bufferBytes;
	int index, column;
	bool isSelected;
} UITableGetItem;

typedef struct UICodeDecorateLine {
	UIRectangle bounds;
	int index; // Starting at 1!
	int x, y; // Position where additional text can be drawn.
	UIPainter *painter;
} UICodeDecorateLine;

typedef struct UIFindByPoint {
	int x, y;
	struct UIElement *result;
} UIFindByPoint;

#define UI_RECT_1(x) ((UIRectangle) { (x), (x), (x), (x) })
#define UI_RECT_1I(x) ((UIRectangle) { (x), -(x), (x), -(x) })
#define UI_RECT_2(x, y) ((UIRectangle) { (x), (x), (y), (y) })
#define UI_RECT_2I(x, y) ((UIRectangle) { (x), -(x), (y), -(y) })
#define UI_RECT_2S(x, y) ((UIRectangle) { 0, (x), 0, (y) })
#define UI_RECT_4(x, y, z, w) ((UIRectangle) { (x), (y), (z), (w) })
#define UI_RECT_WIDTH(_r) ((_r).r - (_r).l)
#define UI_RECT_HEIGHT(_r) ((_r).b - (_r).t)
#define UI_RECT_TOTAL_H(_r) ((_r).r + (_r).l)
#define UI_RECT_TOTAL_V(_r) ((_r).b + (_r).t)
#define UI_RECT_SIZE(_r) UI_RECT_WIDTH(_r), UI_RECT_HEIGHT(_r)
#define UI_RECT_TOP_LEFT(_r) (_r).l, (_r).t
#define UI_RECT_BOTTOM_LEFT(_r) (_r).l, (_r).b
#define UI_RECT_BOTTOM_RIGHT(_r) (_r).r, (_r).b
#define UI_RECT_ALL(_r) (_r).l, (_r).r, (_r).t, (_r).b
#define UI_RECT_VALID(_r) (UI_RECT_WIDTH(_r) > 0 && UI_RECT_HEIGHT(_r) > 0)

#define UI_COLOR_ALPHA_F(x) ((((x) >> 24) & 0xFF) / 255.0f)
#define UI_COLOR_RED_F(x) ((((x) >> 16) & 0xFF) / 255.0f)
#define UI_COLOR_GREEN_F(x) ((((x) >> 8) & 0xFF) / 255.0f)
#define UI_COLOR_BLUE_F(x) ((((x) >> 0) & 0xFF) / 255.0f)
#define UI_COLOR_ALPHA(x) ((((x) >> 24) & 0xFF))
#define UI_COLOR_RED(x) ((((x) >> 16) & 0xFF))
#define UI_COLOR_GREEN(x) ((((x) >> 8) & 0xFF))
#define UI_COLOR_BLUE(x) ((((x) >> 0) & 0xFF))
#define UI_COLOR_FROM_FLOAT(r, g, b) (((uint32_t) ((r) * 255.0f) << 16) | ((uint32_t) ((g) * 255.0f) << 8) | ((uint32_t) ((b) * 255.0f) << 0))
#define UI_COLOR_FROM_RGBA_F(r, g, b, a) (((uint32_t) ((r) * 255.0f) << 16) | ((uint32_t) ((g) * 255.0f) << 8) \
		| ((uint32_t) ((b) * 255.0f) << 0) | ((uint32_t) ((a) * 255.0f) << 24))

#define UI_SWAP(s, a, b) do { s t = (a); (a) = (b); (b) = t; } while (0)

#define UI_CURSOR_ARROW (0)
#define UI_CURSOR_TEXT (1)
#define UI_CURSOR_SPLIT_V (2)
#define UI_CURSOR_SPLIT_H (3)
#define UI_CURSOR_FLIPPED_ARROW (4)
#define UI_CURSOR_CROSS_HAIR (5)
#define UI_CURSOR_HAND (6)
#define UI_CURSOR_RESIZE_UP (7)
#define UI_CURSOR_RESIZE_LEFT (8)
#define UI_CURSOR_RESIZE_UP_RIGHT (9)
#define UI_CURSOR_RESIZE_UP_LEFT (10)
#define UI_CURSOR_RESIZE_DOWN (11)
#define UI_CURSOR_RESIZE_RIGHT (12)
#define UI_CURSOR_RESIZE_DOWN_RIGHT (13)
#define UI_CURSOR_RESIZE_DOWN_LEFT (14)
#define UI_CURSOR_COUNT (15)

#define UI_ALIGN_LEFT (1)
#define UI_ALIGN_RIGHT (2)
#define UI_ALIGN_CENTER (3)

extern const int UI_KEYCODE_A;
extern const int UI_KEYCODE_BACKSPACE;
extern const int UI_KEYCODE_DELETE;
extern const int UI_KEYCODE_DOWN;
extern const int UI_KEYCODE_END;
extern const int UI_KEYCODE_ENTER;
extern const int UI_KEYCODE_ESCAPE;
extern const int UI_KEYCODE_F1;
extern const int UI_KEYCODE_HOME;
extern const int UI_KEYCODE_LEFT;
extern const int UI_KEYCODE_RIGHT;
extern const int UI_KEYCODE_SPACE;
extern const int UI_KEYCODE_TAB;
extern const int UI_KEYCODE_UP;
extern const int UI_KEYCODE_INSERT;
extern const int UI_KEYCODE_0;

#define UI_KEYCODE_LETTER(x) (UI_KEYCODE_A + (x) - 'A')
#define UI_KEYCODE_DIGIT(x) (UI_KEYCODE_0 + (x) - '0')
#define UI_KEYCODE_FKEY(x) (UI_KEYCODE_F1 + (x) - 1)

typedef struct UIElement {
#define UI_ELEMENT_V_FILL (1 << 16)
#define UI_ELEMENT_H_FILL (1 << 17)
#define UI_ELEMENT_WINDOW (1 << 18)
#define UI_ELEMENT_PARENT_PUSH (1 << 19)
#define UI_ELEMENT_TAB_STOP (1 << 20)
#define UI_ELEMENT_NON_CLIENT (1 << 21) // Don't destroy in UIElementDestroyDescendents, like scroll bars.
#define UI_ELEMENT_DISABLED (1 << 22) // Don't receive input events.

#define UI_ELEMENT_HIDE (1 << 29)
#define UI_ELEMENT_DESTROY (1 << 30)
#define UI_ELEMENT_DESTROY_DESCENDENT (1 << 31)

	uint32_t flags; // First 16 bits are element specific.
	uint32_t id;

	struct UIElement *parent;
	struct UIElement *next;
	struct UIElement *children;
	struct UIWindow *window;

	UIRectangle bounds, clip;
	
	void *cp; // Context pointer (for user).

	int (*messageClass)(struct UIElement *element, UIMessage message, int di /* data integer */, void *dp /* data pointer */);
	int (*messageUser)(struct UIElement *element, UIMessage message, int di, void *dp);

	const char *cClassName;
} UIElement;

#define UI_SHORTCUT(code, ctrl, shift, alt, invoke, cp) ((UIShortcut) { (code), (ctrl), (shift), (alt), (invoke), (cp) })

typedef struct UIWindow {
#define UI_WINDOW_MENU (1 << 0)
#define UI_WINDOW_INSPECTOR (1 << 1)
#define UI_WINDOW_CENTER_IN_OWNER (1 << 2)
#define UI_WINDOW_MAXIMIZE (1 << 3)

	UIElement e;

	UIElement *dialog;

	UIShortcut *shortcuts;
	size_t shortcutCount, shortcutAllocated;

	float scale;

	uint32_t *bits;
	int width, height;
	struct UIWindow *next;

	UIElement *hovered, *pressed, *focused, *dialogOldFocus;
	int pressedButton;

	int cursorX, cursorY;
	int cursorStyle;

	// Set when a textbox is modified. 
	// Useful for tracking whether changes to the loaded document have been saved.
	bool textboxModifiedFlag; 

	bool ctrl, shift, alt;

	UIRectangle updateRegion;

#ifdef UI_DEBUG
	float lastFullFillCount;
#endif

#ifdef WIN32
	HWND hwnd;
	bool trackingLeave;
#else
  Window window;
  XImage *image;
  XIC xic;
  unsigned ctrlCode, shiftCode, altCode;
  Window dragSource;
#endif
} UIWindow;

typedef struct UIPanel {
#define UI_PANEL_HORIZONTAL (1 << 0)
#define UI_PANEL_GRAY (1 << 2)
#define UI_PANEL_WHITE (1 << 3)
#define UI_PANEL_EXPAND (1 << 4)
#define UI_PANEL_MEDIUM_SPACING (1 << 5)
#define UI_PANEL_SMALL_SPACING (1 << 6)
#define UI_PANEL_SCROLL (1 << 7)
#define UI_PANEL_BORDER (1 << 8)
	UIElement e;
	struct UIScrollBar *scrollBar;
	UIRectangle border;
	int gap;
} UIPanel;

typedef struct UIButton {
#define UI_BUTTON_SMALL (1 << 0)
#define UI_BUTTON_MENU_ITEM (1 << 1)
#define UI_BUTTON_CAN_FOCUS (1 << 2)
#define UI_BUTTON_DROP_DOWN (1 << 3)
#define UI_BUTTON_CHECKED (1 << 15)
	UIElement e;
	char *label;
	ptrdiff_t labelBytes;
	void (*invoke)(void *cp);
} UIButton;

typedef struct UICheckbox {
#define UI_CHECKBOX_ALLOW_INDETERMINATE (1 << 0)
	UIElement e;
#define UI_CHECK_UNCHECKED (0)
#define UI_CHECK_CHECKED (1)
#define UI_CHECK_INDETERMINATE (2)
	uint8_t check;
	char *label;
	ptrdiff_t labelBytes;
	void (*invoke)(void *cp);
} UICheckbox;

typedef struct UILabel {
	UIElement e;
	char *label;
	ptrdiff_t labelBytes;
} UILabel;

typedef struct UISpacer {
#define UI_SPACER_LINE (1 << 0)
	UIElement e;
	int width, height;
} UISpacer;

typedef struct UISplitPane {
#define UI_SPLIT_PANE_VERTICAL (1 << 0)
	UIElement e;
	float weight;
} UISplitPane;

typedef struct UITabPane {
	UIElement e;
	char *tabs;
	int active;
} UITabPane;

typedef struct UIScrollBar {
#define UI_SCROLL_BAR_HORIZONTAL (1 << 0)
	UIElement e;
	int64_t maximum, page;
	int64_t dragOffset;
	double position;
	uint64_t lastAnimateTime;
	bool inDrag, horizontal;
} UIScrollBar;

typedef struct UICodeLine {
	int offset, bytes;
} UICodeLine;

typedef struct UICode {
#define UI_CODE_NO_MARGIN (1 << 0)
	UIElement e;
	UIScrollBar *vScroll;
	UICodeLine *lines;
	UIFont *font;
	int lineCount, focused;
	bool moveScrollToFocusNextLayout;
	char *content;
	size_t contentBytes;
	int tabSize;
} UICode;

typedef struct UIGauge {
	UIElement e;
	float position;
} UIGauge;

typedef struct UITable {
	UIElement e;
	UIScrollBar *vScroll;
	int itemCount;
	char *columns;
	int *columnWidths, columnCount, columnHighlight;
} UITable;

typedef struct UITextbox {
	UIElement e;
	char *string;
	ptrdiff_t bytes;
	int carets[2];
	int scroll;
	bool rejectNextKey;
} UITextbox;

#define UI_MENU_PLACE_ABOVE (1 << 0)
#define UI_MENU_NO_SCROLL (1 << 1)
typedef struct UIMenu {
	UIElement e;
	int pointX, pointY;
	UIScrollBar *vScroll;
} UIMenu;

typedef struct UISlider {
	UIElement e;
	float position;
	int steps;
} UISlider;

typedef struct UIColorPicker {
#define UI_COLOR_PICKER_HAS_OPACITY (1 << 0)
	UIElement e;
	float hue, saturation, value, opacity;
} UIColorPicker;

typedef struct UIMDIClient {
#define UI_MDI_CLIENT_TRANSPARENT (1 << 0)
	UIElement e;
	struct UIMDIChild *active;
	int cascade;
} UIMDIClient;

typedef struct UIMDIChild {
#define UI_MDI_CHILD_CLOSE_BUTTON (1 << 0)
	UIElement e;
	UIRectangle bounds;
	char *title;
	ptrdiff_t titleBytes;
	int dragHitTest;
	UIRectangle dragOffset;
	struct UIMDIChild *previous;
} UIMDIChild;

typedef struct UIExpandPane {
	UIElement e;
	UIButton *button;
	UIPanel *panel;
	bool expanded;
} UIExpandPane;

typedef struct UIImageDisplay {
#define UI_IMAGE_DISPLAY_INTERACTIVE (1 << 0)
#define _UI_IMAGE_DISPLAY_ZOOM_FIT (1 << 1)

	UIElement e;
	uint32_t *bits;
	int width, height;
	float panX, panY, zoom;
	
	// Internals:
	int previousWidth, previousHeight;
	int previousPanPointX, previousPanPointY;
} UIImageDisplay;

typedef struct UIWrapPanel {
	UIElement e;
} UIWrapPanel;

void UIInitialise();
int UIMessageLoop();

UIElement *UIElementCreate(size_t bytes, UIElement *parent, uint32_t flags, 
	int (*messageClass)(UIElement *, UIMessage, int, void *), const char *cClassName);

UIButton *UIButtonCreate(UIElement *parent, uint32_t flags, const char *label, ptrdiff_t labelBytes);
UICheckbox *UICheckboxCreate(UIElement *parent, uint32_t flags, const char *label, ptrdiff_t labelBytes);
UIColorPicker *UIColorPickerCreate(UIElement *parent, uint32_t flags);
UIExpandPane *UIExpandPaneCreate(UIElement *parent, uint32_t flags, const char *label, ptrdiff_t labelBytes, uint32_t panelFlags);
UIGauge *UIGaugeCreate(UIElement *parent, uint32_t flags);
UIMDIClient *UIMDIClientCreate(UIElement *parent, uint32_t flags);
UIMDIChild *UIMDIChildCreate(UIElement *parent, uint32_t flags, UIRectangle initialBounds, const char *title, ptrdiff_t titleBytes);

UIPanel *UIPanelCreate(UIElement *parent, uint32_t flags);

UIScrollBar *UIScrollBarCreate(UIElement *parent, uint32_t flags);
UISlider *UISliderCreate(UIElement *parent, uint32_t flags);
UISpacer *UISpacerCreate(UIElement *parent, uint32_t flags, int width, int height);
UISplitPane *UISplitPaneCreate(UIElement *parent, uint32_t flags, float weight);
UITabPane *UITabPaneCreate(UIElement *parent, uint32_t flags, const char *tabs /* separate with \t, terminate with \0 */);
UIWrapPanel *UIWrapPanelCreate(UIElement *parent, uint32_t flags);

UILabel *UILabelCreate(UIElement *parent, uint32_t flags, const char *label, ptrdiff_t labelBytes);
void UILabelSetContent(UILabel *code, const char *content, ptrdiff_t byteCount);

UIImageDisplay *UIImageDisplayCreate(UIElement *parent, uint32_t flags, uint32_t *bits, size_t width, size_t height, size_t stride);
void UIImageDisplaySetContent(UIImageDisplay *display, uint32_t *bits, size_t width, size_t height, size_t stride);

UIWindow *UIWindowCreate(UIWindow *owner, uint32_t flags, const char *cTitle, int width, int height);
void UIWindowRegisterShortcut(UIWindow *window, UIShortcut shortcut);
void UIWindowPostMessage(UIWindow *window, UIMessage message, void *dp); // Thread-safe.
void UIWindowPack(UIWindow *window, int width); // Change the size of the window to best match its contents.

typedef void (*UIDialogUserCallback)(UIElement *);
const char *UIDialogShow(UIWindow *window, uint32_t flags, const char *format, ...);

UIMenu *UIMenuCreate(UIElement *parent, uint32_t flags);
void UIMenuAddItem(UIMenu *menu, uint32_t flags, const char *label, ptrdiff_t labelBytes, void (*invoke)(void *cp), void *cp);
void UIMenuShow(UIMenu *menu);

UITextbox *UITextboxCreate(UIElement *parent, uint32_t flags);
void UITextboxReplace(UITextbox *textbox, const char *text, ptrdiff_t bytes, bool sendChangedMessage);
void UITextboxClear(UITextbox *textbox, bool sendChangedMessage);
void UITextboxMoveCaret(UITextbox *textbox, bool backward, bool word);

UITable *UITableCreate(UIElement *parent, uint32_t flags, const char *columns /* separate with \t, terminate with \0 */);
int UITableHitTest(UITable *table, int x, int y); // Returns item index. Returns -1 if not on an item.
int UITableHeaderHitTest(UITable *table, int x, int y); // Returns column index or -1.
bool UITableEnsureVisible(UITable *table, int index); // Returns false if the item was already visible.
void UITableResizeColumns(UITable *table);

UICode *UICodeCreate(UIElement *parent, uint32_t flags);
void UICodeFocusLine(UICode *code, int index); // Line numbers are 1-indexed!!
int UICodeHitTest(UICode *code, int x, int y); // Returns line number; negates if in margin. Returns 0 if not on a line.
void UICodeInsertContent(UICode *code, const char *content, ptrdiff_t byteCount, bool replace);

void UIDrawBlock(UIPainter *painter, UIRectangle rectangle, uint32_t color);
void UIDrawInvert(UIPainter *painter, UIRectangle rectangle);
bool UIDrawLine(UIPainter *painter, int x0, int y0, int x1, int y1, uint32_t color); // Returns false if the line was not visible.
void UIDrawTriangle(UIPainter *painter, int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
void UIDrawTriangleOutline(UIPainter *painter, int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
void UIDrawGlyph(UIPainter *painter, int x, int y, int c, uint32_t color);
void UIDrawRectangle(UIPainter *painter, UIRectangle r, uint32_t mainColor, uint32_t borderColor, UIRectangle borderSize);
void UIDrawBorder(UIPainter *painter, UIRectangle r, uint32_t borderColor, UIRectangle borderSize);
void UIDrawString(UIPainter *painter, UIRectangle r, const char *string, ptrdiff_t bytes, uint32_t color, int align, UIStringSelection *selection);
int UIDrawStringHighlighted(UIPainter *painter, UIRectangle r, const char *string, ptrdiff_t bytes, int tabSize);

int UIMeasureStringWidth(const char *string, ptrdiff_t bytes);
int UIMeasureStringHeight();

uint64_t UIAnimateClock(); // In ms.

bool UIElementAnimate(UIElement *element, bool stop);
void UIElementDestroy(UIElement *element);
void UIElementDestroyDescendents(UIElement *element);
UIElement *UIElementFindByPoint(UIElement *element, int x, int y);
void UIElementFocus(UIElement *element);
UIRectangle UIElementScreenBounds(UIElement *element); // Returns bounds of element in same coordinate system as used by UIWindowCreate.
void UIElementRefresh(UIElement *element);
void UIElementRepaint(UIElement *element, UIRectangle *region);
void UIElementMove(UIElement *element, UIRectangle bounds, bool alwaysLayout);
int UIElementMessage(UIElement *element, UIMessage message, int di, void *dp);
void UIElementChangeParent(UIElement *element, UIElement *newParent, UIElement *insertBefore); // Set insertBefore to null to insert at the end.

UIElement *UIParentPush(UIElement *element);
UIElement *UIParentPop();

UIRectangle UIRectangleIntersection(UIRectangle a, UIRectangle b);
UIRectangle UIRectangleBounding(UIRectangle a, UIRectangle b);
UIRectangle UIRectangleAdd(UIRectangle a, UIRectangle b);
UIRectangle UIRectangleTranslate(UIRectangle a, UIRectangle b);
bool UIRectangleEquals(UIRectangle a, UIRectangle b);
bool UIRectangleContains(UIRectangle a, int x, int y);

bool UIColorToHSV(uint32_t rgb, float *hue, float *saturation, float *value);
void UIColorToRGB(float hue, float saturation, float value, uint32_t *rgb);

char *UIStringCopy(const char *in, ptrdiff_t inBytes);
ptrdiff_t UIStringLength(const char *cString);

UIFont *UIFontCreate(const char *cPath, uint32_t size);
UIFont *UIFontActivate(UIFont *font); // Returns the previously active font.

// Clipboard interaction
void UIClipboardWriteText(UIWindow *window, char *text);
char *UIClipboardReadTextStart(UIWindow *window, size_t *bytes);
void UIClipboardReadTextEnd(UIWindow *window, char *text);

// String utils
bool UICharIsAlpha(char c);
bool UICharIsDigit(char c);
bool UICharIsAlphaOrDigitOrUnderscore(char c);

// Widget tests
bool UIIsPanel(const UIElement *element);
bool UIIsButton(const UIElement *element);
bool UIIsMDIChild(const UIElement *element);

// Themes
const UITheme *UIGetCurrentTheme();

void UIAssert(const char *exp, const char *filename, unsigned int line);
void UIFree(void *ptr);

#ifdef UI_DEBUG
void UIInspectorLog(const char *cFormat, ...);
#endif

#endif // LUIGI_H
