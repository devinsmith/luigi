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

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#endif // WIN32


#include <luigi.h>
#include <luigi_private.h>

void UIAssert(const char *exp, const char *filename, unsigned int line)
{
#if defined(WIN32)
  LPCSTR assertMsg;

  LPCSTR args[3];
  args[0] = filename;
  args[1] = (LPCSTR)(uintptr_t)line;
  args[2] = exp;

  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
      "%1:%2!u!: UIAssert(%3) failed."
      NULL,
      0, // No message
      0, // No Language Id
      (LPSTR)&assertMsg,
      0, // No minimum size
      (va_list*)args);

  ui.assertionFailure = true;
  MessageBox(0, assertMsg, 0, 0);
  ExitProcess(1);
#else
  if (isatty(fileno(stderr))) {
    fprintf(stderr, "%s:%d: \033[1;33mUIAssert(%s)\033[0m failed.\n", filename, line, exp);
  } else {
    fprintf(stdout, "%s:%d: UIAssert(%s) failed.\n", filename, line, exp);
  }
#endif
}

void UIFree(void *ptr)
{
#ifdef WIN32
  HeapFree(ui.heap, 0, ptr);
#else
  free(ptr);
#endif
}
