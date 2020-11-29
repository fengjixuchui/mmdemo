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

void LargePageTest();

wchar_t gChildArg[] = L":)";

int wmain(int argc, wchar_t *argv[]) {
  HMODULE mod = GetModuleHandle(nullptr);
  Log(L"%04x: %p\n", GetCurrentProcessId(), mod);

  if (wcscmp(argv[0], gChildArg) == 0)
    return 0;

  LargePageTest();

  SameBoat boat;
  if (!boat) return 1;

  // argv[0] can be a relative path.  Let's get a full path.
  std::unique_ptr<wchar_t[]> path = GetFullExePath(mod);

  ChildProcess child1(path.get(), gChildArg);
  if (!child1 || !boat.AddProcess(child1)) return 1;

  if (argc >= 2) {
    ChildProcess child2(argv[1], path.get());
    if (!child2 || !boat.AddProcess(child2)) return 1;
  }

  child1.ResumeAndWait();
  return 0;
}
