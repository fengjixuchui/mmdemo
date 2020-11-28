#pragma once

extern void Log(LPCWSTR format, ...);

class ChildProcess final {
  PROCESS_INFORMATION pi_;

 public:
  ChildProcess(const wchar_t* path, wchar_t* cmd) : pi_{} {
    STARTUPINFO si = {sizeof(si)};
    BOOL ok = CreateProcess(
        path, cmd,
        /*lpProcessAttributes*/nullptr,
        /*lpThreadAttributes*/nullptr,
        /*bInheritHandles*/FALSE,
        CREATE_SUSPENDED,
        /*lpEnvironment*/nullptr,
        /*lpCurrentDirectory*/nullptr,
        &si,
        &pi_);
    if (!ok) {
      Log(L"CreateProcess failed - %08lx\n",  GetLastError());
    }
  }

  operator bool() const { return !!pi_.hProcess; }
  operator HANDLE() const { return pi_.hProcess; }

  DWORD ResumeAndWait() const {
    ResumeThread(pi_.hThread);
    return WaitForSingleObject(pi_.hProcess, INFINITE);
  }

  ~ChildProcess() {
    ResumeAndWait();
    CloseHandle(pi_.hThread);
    CloseHandle(pi_.hProcess);
  }
};

class SameBoat {
  struct HandleCloser {
    typedef HANDLE pointer;
    void operator()(HANDLE h) { CloseHandle(h); }
  };

  std::unique_ptr<HANDLE, HandleCloser> job_;

 public:
  SameBoat() : job_{} {
    std::unique_ptr<HANDLE, HandleCloser> job(
        CreateJobObject(/*lpJobAttributes*/nullptr, /*lpName*/nullptr));
    if (!job) {
      Log(L"CreateJobObject failed - %08x\n", GetLastError());
      return;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {};
    jobInfo.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation,
                                 &jobInfo, sizeof(jobInfo))) {
      Log(L"SetInformationJobObject failed - %08x\n", GetLastError());
      return;
    }

    job_.swap(job);
  }

  operator bool() const { return !!job_; }

  bool AddProcess(HANDLE proc) {
    if (!AssignProcessToJobObject(job_.get(), proc)) {
      Log(L"AssignProcessToJobObject failed - %08x\n", GetLastError());
      return false;
    }
    return true;
  }
};

std::unique_ptr<wchar_t[]> GetFullExePath(HMODULE mod) {
  DWORD len = MAX_PATH;
  do {
    auto buf = std::make_unique<wchar_t[]>(len);
    DWORD ret = GetModuleFileName(mod, buf.get(), len);
    DWORD gle = ::GetLastError();
    if (ret == 0
        || (gle != ERROR_SUCCESS && gle != ERROR_INSUFFICIENT_BUFFER)) {
      Log(L"GetModuleFileName failed - %08lx\n", gle);
      return nullptr;
    }

    if (gle == ERROR_SUCCESS && ret < len) {
      return buf;
    }

    len *= 2;
  } while (1);
}
