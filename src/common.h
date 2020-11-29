#pragma once

#include <memory>

void Log(LPCWSTR format, ...);
std::unique_ptr<wchar_t[]> GetFullExePath(HMODULE mod);

struct HandleCloser {
  typedef HANDLE pointer;
  void operator()(HANDLE h) { CloseHandle(h); }
};

struct PageFreer {
  typedef void* pointer;
  void operator()(void* p) { VirtualFree(p, 0, MEM_RELEASE); }
};

struct ModFreer {
  typedef HMODULE pointer;
  void operator()(HMODULE mod) { FreeLibrary(mod); }
};

class ChildProcess final {
  PROCESS_INFORMATION pi_;

 public:
  ChildProcess(const wchar_t* path, wchar_t* cmd);
  ~ChildProcess();

  operator bool() const { return !!pi_.hProcess; }
  operator HANDLE() const { return pi_.hProcess; }

  DWORD ResumeAndWait() const;
};

class SameBoat {
  std::unique_ptr<HANDLE, HandleCloser> job_;

 public:
  SameBoat();

  operator bool() const { return !!job_; }

  bool AddProcess(HANDLE proc);
};
