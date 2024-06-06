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

#ifdef WIN32

void UIClipboardWriteText(UIWindow *window, char *text) {
	if (OpenClipboard(window->hwnd)) {
		EmptyClipboard();
		HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, UIStringLength(text) + 1);
		char *copy = (char *) GlobalLock(memory);
		for (uintptr_t i = 0; text[i]; i++) copy[i] = text[i];
		GlobalUnlock(copy);
		SetClipboardData(CF_TEXT, memory);
		CloseClipboard();
	}
}

char *UIClipboardReadTextStart(UIWindow *window, size_t *bytes) {
	if (!OpenClipboard(window->hwnd)) {
		return NULL;
	}
	
	HANDLE memory = GetClipboardData(CF_TEXT);
	
	if (!memory) {
		CloseClipboard();
		return NULL;
	}
	
	char *buffer = (char *) GlobalLock(memory);
	
	if (!buffer) {
		CloseClipboard();
		return NULL;
	}
	
	size_t byteCount = GlobalSize(memory);
	
	if (byteCount < 1) {
		GlobalUnlock(memory);
		CloseClipboard();
		return NULL;
	}

	char *copy = (char *) UI_MALLOC(byteCount + 1);
	for (uintptr_t i = 0; i < byteCount; i++) copy[i] = buffer[i];
	copy[byteCount] = 0; // Just in case.
	
	GlobalUnlock(memory);
	CloseClipboard();
	
	if (bytes) *bytes = UIStringLength(copy);
	return copy;
}

void UIClipboardReadTextEnd(UIWindow *window, char *text) {
	UI_FREE(text);
}

#else

void UIClipboardWriteText(UIWindow *window, char *text) {
	UI_FREE(ui.pasteText);
	ui.pasteText = text;
	XSetSelectionOwner(ui.display, ui.clipboardID, window->window, 0);
}

char *UIClipboardReadTextStart(UIWindow *window, size_t *bytes) {
	Window clipboardOwner = XGetSelectionOwner(ui.display, ui.clipboardID);

	if (clipboardOwner == None) {
		return NULL;
	}

	if (X11FindWindow(clipboardOwner)) {
		*bytes = strlen(ui.pasteText);
		char *copy = (char *) UI_MALLOC(*bytes);
		memcpy(copy, ui.pasteText, *bytes);
		return copy;
	}

	XConvertSelection(ui.display, ui.clipboardID, XA_STRING, ui.xSelectionDataID, window->window, CurrentTime);
	XSync(ui.display, 0);
	XNextEvent(ui.display, &ui.copyEvent);

	// Hack to get around the fact that PropertyNotify arrives before SelectionNotify.
	// We need PropertyNotify for incremental transfers.
	while (ui.copyEvent.type == PropertyNotify) {
		XNextEvent(ui.display, &ui.copyEvent);
	}

	if (ui.copyEvent.type == SelectionNotify && ui.copyEvent.xselection.selection == ui.clipboardID && ui.copyEvent.xselection.property) {
		Atom target;
		// This `itemAmount` is actually `bytes_after_return`
		unsigned long size, itemAmount;
		char *data;
		int format;
		XGetWindowProperty(ui.copyEvent.xselection.display, ui.copyEvent.xselection.requestor, ui.copyEvent.xselection.property, 0L, ~0L, 0, 
				AnyPropertyType, &target, &format, &size, &itemAmount, (unsigned char **) &data);

		// We have to allocate for incremental transfers but we don't have to allocate for non-incremental transfers.
		// I'm allocating for both here to make _UIClipboardReadTextEnd work the same for both
		if (target != ui.incrID) {
			*bytes = size;
			char *copy = (char *) UI_MALLOC(*bytes);
			memcpy(copy, data, *bytes);
			XFree(data);
			XDeleteProperty(ui.copyEvent.xselection.display, ui.copyEvent.xselection.requestor, ui.copyEvent.xselection.property);
			return copy;
		}

		XFree(data);
		XDeleteProperty(ui.display, ui.copyEvent.xselection.requestor, ui.copyEvent.xselection.property);
		XSync(ui.display, 0);

		*bytes = 0;
		char *fullData = NULL;

		while (true) {
			// TODO Timeout.
			XNextEvent(ui.display, &ui.copyEvent);

			if (ui.copyEvent.type == PropertyNotify) {
				// The other case - PropertyDelete would be caused by us and can be ignored
				if (ui.copyEvent.xproperty.state == PropertyNewValue) {
					unsigned long chunkSize;

					// Note that this call deletes the property.
					XGetWindowProperty(ui.display, ui.copyEvent.xproperty.window, ui.copyEvent.xproperty.atom, 0L, ~0L, 
						True, AnyPropertyType, &target, &format, &chunkSize, &itemAmount, (unsigned char **) &data);
					
					if (chunkSize == 0) {
						return fullData;
					} else {
						ptrdiff_t currentOffset = *bytes;
						*bytes += chunkSize;
						fullData = (char *) UI_REALLOC(fullData, *bytes);
						memcpy(fullData + currentOffset, data, chunkSize);
					}

					XFree(data);
				}
			}
		}
	} else {
		// TODO What should happen in this case? Is the next event always going to be the selection event?
		return NULL;
	}
}

void UIClipboardReadTextEnd(UIWindow *window, char *text) {
	if (text) {
		//XFree(text);
		//XDeleteProperty(ui.copyEvent.xselection.display, ui.copyEvent.xselection.requestor, ui.copyEvent.xselection.property);
		UI_FREE(text);
	}
}

#endif // WIN32
