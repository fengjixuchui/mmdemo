#include <memory>
#include <windows.h>
#include "common.h"

bool EnablePrivilege(HANDLE token, LPCWSTR privName, bool trueToEnable = true) {
  LUID luid;
  if (!LookupPrivilegeValue(nullptr, privName, &luid)) {
    Log(L"LookupPrivilegeValue failed - 0x%08x\n", GetLastError());
    return false;
  }

  TOKEN_PRIVILEGES tp;
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = trueToEnable ? SE_PRIVILEGE_ENABLED : 0;
  bool ok = !!AdjustTokenPrivileges(token,
                                    /*DisableAllPrivileges*/FALSE,
                                    &tp, sizeof(tp),
                                    /*PreviousState*/nullptr,
                                    /*ReturnLength*/nullptr);
  DWORD gle = GetLastError();
  if (!ok) {
    Log(L"AdjustTokenPrivileges failed - 0x%08x\n", gle);
    return false;
  }

  if (gle == ERROR_NOT_ALL_ASSIGNED) {
    Log(L"The process token does not have the %s privilege.\n", privName);
    return false;
  }

  return true;
}

void LargePageTest() {
  HANDLE rawToken;
  if (!OpenProcessToken(GetCurrentProcess(),
                        TOKEN_ADJUST_PRIVILEGES,
                        &rawToken)) {
    Log(L"OpenProcessToken failed - %08lx\n", GetLastError());
    return;
  }

  std::unique_ptr<HANDLE, HandleCloser> token(rawToken);
  rawToken = nullptr;

  if (!EnablePrivilege(token.get(), SE_LOCK_MEMORY_NAME)) {
    return;
  }

#ifdef _WIN64
  std::unique_ptr<void*, PageFreer> page1gb(
      VirtualAlloc(/*lpAddress*/nullptr,
                   1 << 30,
                   MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES,
                   PAGE_READWRITE));
  if (page1gb) {
    *reinterpret_cast<uint32_t*>(page1gb.get()) = 0xdeadbeef;
    Log(L"1GB Page: %p\n", page1gb.get());
  }
  else {
    Log(L"VirtualAlloc(1GB) failed - %08lx\n", GetLastError());
  }

  std::unique_ptr<void*, PageFreer> page2mb(
      VirtualAlloc(/*lpAddress*/nullptr,
                   1 << 21,
                   MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES,
                   PAGE_READONLY));
  if (page2mb) {
    Log(L"2MB Page: %p\n", page2mb.get());
  }
  else {
    Log(L"VirtualAlloc(2MB) failed - %08lx\n", GetLastError());
  }
#else
  std::unique_ptr<void*, PageFreer> page4mb(
      VirtualAlloc(/*lpAddress*/nullptr,
                   1 << 22,
                   MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES,
                   PAGE_READONLY));
  if (page4mb) {
    Log(L"4MB Page: %p\n", page4mb.get());
  }
  else {
    Log(L"VirtualAlloc(4MB) failed - %08lx\n", GetLastError());
  }
#endif

  if (getenv("PLEASE_PAUSE")) __debugbreak();
}
