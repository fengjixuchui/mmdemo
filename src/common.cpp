#include <memory>
#include <windows.h>
#include "common.h"

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

ChildProcess::ChildProcess(const wchar_t* path, wchar_t* cmd) : pi_{} {
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

ChildProcess::~ChildProcess() {
  ResumeAndWait();
  CloseHandle(pi_.hThread);
  CloseHandle(pi_.hProcess);
}

DWORD ChildProcess::ResumeAndWait() const {
  ResumeThread(pi_.hThread);
  return WaitForSingleObject(pi_.hProcess, INFINITE);
}

SameBoat::SameBoat() : job_{} {
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

bool SameBoat::AddProcess(HANDLE proc) {
  if (!AssignProcessToJobObject(job_.get(), proc)) {
    Log(L"AssignProcessToJobObject failed - %08x\n", GetLastError());
    return false;
  }
  return true;
}
