#include <cwchar>
#include <memory>
#include <windows.h>
#include "common.h"

void Log(LPCWSTR format, ...) {
  va_list v;
  va_start(v, format);
  vwprintf(format, v);
  va_end(v);
}

int wmain(int argc, wchar_t *argv[]) {
  if (getenv("WARMUP")) {
    void* target;
    {
      std::unique_ptr<HMODULE, ModFreer> warmup(LoadLibrary(argv[0]));
      target = warmup.get();
    }
    VirtualAlloc(target, 0x10000, MEM_RESERVE, PAGE_NOACCESS);
  }

  std::unique_ptr<HMODULE, ModFreer> extra(LoadLibrary(argv[0]));
  Log(L"%04x: %p %s\n", GetCurrentProcessId(), extra.get(), argv[0]);

  if (getenv("PLEASE_PAUSE")) __debugbreak();
  return 0;
}
