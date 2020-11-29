#include <cwchar>
#include <windows.h>

void Log(LPCWSTR format, ...) {
  va_list v;
  va_start(v, format);
  vwprintf(format, v);
  va_end(v);
}

void LargePageTest();

int wmain(int argc, wchar_t *argv[]) {
  LargePageTest();
  return 0;
}
