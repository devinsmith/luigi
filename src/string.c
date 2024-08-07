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
#include "luigi_private.h"

bool UICharIsAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool UICharIsDigit(char c) {
	return c >= '0' && c <= '9';
}

bool UICharIsAlphaOrDigitOrUnderscore(char c) {
	return UICharIsAlpha(c) || UICharIsDigit(c) || c == '_';
}

ptrdiff_t UIStringLength(const char *cString) {
	if (!cString) return 0;
	ptrdiff_t length;
	for (length = 0; cString[length]; length++);
	return length;
}

char *UIStringCopy(const char *in, ptrdiff_t inBytes) {
	if (inBytes == -1) {
		inBytes = UIStringLength(in);
	}

	char *buffer = (char *) UI_MALLOC(inBytes + 1);
	
	for (intptr_t i = 0; i < inBytes; i++) {
		buffer[i] = in[i];
	}
	
	buffer[inBytes] = 0;
	return buffer;
}

int UIMeasureStringWidth(const char *string, ptrdiff_t bytes) {
	if (bytes == -1) {
		bytes = UIStringLength(string);
	}
	
	return bytes * ui.activeFont->glyphWidth;
}

int UIMeasureStringHeight() {
	return ui.activeFont->glyphHeight;
}
