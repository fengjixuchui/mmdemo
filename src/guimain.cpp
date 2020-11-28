#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <memory>
#include "basewindow.h"
#include "common.h"

HANDLE gSharedSection = 0;

void Log(LPCWSTR format, ...) {
  wchar_t linebuf[1024];
  va_list v;
  va_start(v, format);
  StringCbVPrintf(linebuf, sizeof(linebuf), format, v);
  va_end(v);
  OutputDebugString(linebuf);
}

class MainWindow : public BaseWindow<MainWindow> {
private:
  enum class Control {
    Touch = 100,
    Forget,
  };

  void* view_;
  HWND btnTouch_, btnForget_;

  bool InitWindow() {
    view_ = MapViewOfFile(
        gSharedSection,
        FILE_MAP_WRITE,
        0, 0, 0);
    if (!view_) {
      Log(L"MapViewOfFile failed - %08x\n", GetLastError());
      return false;
    }

    constexpr DWORD buttonStyle = WS_VISIBLE | WS_CHILD | BS_FLAT;
    constexpr SIZE size = {70, 30};
    constexpr int margin = 5;
    btnTouch_ = CreateWindow(
        L"Button",
        L"Touch",
        buttonStyle,
        margin, margin, size.cx, size.cy,
        hwnd(),
        reinterpret_cast<HMENU>(Control::Touch),
        GetModuleHandle(nullptr),
        /*lpParam*/nullptr);
    btnForget_ = CreateWindow(
        L"Button",
        L"Forget",
        buttonStyle,
        margin * 2 + size.cx, margin, size.cx, size.cy,
        hwnd(),
        reinterpret_cast<HMENU>(Control::Forget),
        GetModuleHandle(nullptr),
        /*lpParam*/nullptr);

    return btnTouch_ && btnForget_;
  }

public:
  LPCTSTR ClassName() const {
    return _T("MainWindow");
  }

  LRESULT HandleMessage(UINT msg, WPARAM w, LPARAM l) {
    LRESULT ret = 0;
    switch (msg) {
    case WM_CREATE:
      if (!InitWindow()) {
        return -1;
      }
      break;
    case WM_DESTROY:
      if (!UnmapViewOfFile(view_)) {
        Log(L"UnmapViewOfFile failed - %08x\n", GetLastError());
      }
      PostQuitMessage(0);
      break;
    case WM_COMMAND:
      switch (static_cast<Control>(LOWORD(w))) {
        case Control::Touch:
          Log(L"[%p] = %d\n", view_, ++(*reinterpret_cast<uint32_t*>(view_)));
          break;
        case Control::Forget:
          Log(L"SetProcessWorkingSetSize - %d\n",
              SetProcessWorkingSetSize(GetCurrentProcess(),
                                       static_cast<size_t>(-1LL),
                                       static_cast<size_t>(-1LL)));
          break;
      }
      break;
    default:
      ret = DefWindowProc(hwnd(), msg, w, l);
      break;
    }
    return ret;
  }
};

class SharedSection {
  HANDLE mSecionHandle;
  LPVOID mMappedView;

 public:
  SharedSection(uint32_t size) noexcept
      : mSecionHandle{}, mMappedView{} {
    mSecionHandle = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        /*lpFileMappingAttributes*/nullptr,
        PAGE_READWRITE,
        0, size,
        /*lpName*/nullptr);
    if (!mSecionHandle) {
      Log(L"CreateFileMapping failed - %08x\n", GetLastError());
      return;
    }

    mMappedView = MapViewOfFile(
        mSecionHandle,
        FILE_MAP_READ,
        0, 0, 0);
    if (!mMappedView) {
      Log(L"MapViewOfFile failed - %08x\n", GetLastError());
      return;
    }

    Log(L"Mapped onto %p\n", mMappedView);
  }

  SharedSection(SharedSection&& aOther)
      : mSecionHandle(aOther.mSecionHandle),
        mMappedView(aOther.mMappedView) {
    aOther.mSecionHandle = nullptr;
    aOther.mMappedView = nullptr;
  }

  SharedSection& operator=(SharedSection&& aOther) {
    if (this != &aOther) {
      mSecionHandle = aOther.mSecionHandle;
      aOther.mSecionHandle = nullptr;
      mMappedView = aOther.mMappedView;
      aOther.mMappedView = nullptr;
    }
    return *this;
  }

  SharedSection(const SharedSection&) = delete;
  SharedSection& operator=(const SharedSection&) = delete;

  ~SharedSection() {
    if (mMappedView) {
      if (!UnmapViewOfFile(mMappedView)) {
        Log(L"UnmapViewOfFile failed - %08x\n", GetLastError());
      }
    }

    if (mSecionHandle) {
      if (!CloseHandle(mSecionHandle)) {
        Log(L"CloseHandle(section) failed - %08x\n", GetLastError());
      }
    }
  }

  operator HANDLE() const { return mSecionHandle; }
  void Touch() const {
    Log(L"[%p] = %d\n", mMappedView, *reinterpret_cast<uint32_t*>(mMappedView));
  }

  HANDLE GetHandleFor(HANDLE targetProc) const {
    HANDLE remoteHandle = nullptr;
    if (!DuplicateHandle(GetCurrentProcess(),
                         mSecionHandle,
                         targetProc,
                         &remoteHandle,
                         DUPLICATE_SAME_ACCESS,
                         /*bInheritHandle*/FALSE,
                         /*dwOptions*/0)) {
      Log(L"DuplicateHandle failed - %08x\n", GetLastError());
    }
    return remoteHandle;
  }
};

int ChildMain(int show) {
  if (auto p = std::make_unique<MainWindow>()) {
    if (p->Create(_T("MainWindow Title"),
                  WS_OVERLAPPEDWINDOW,
                  /*style_ex*/0,
                  CW_USEDEFAULT, 0,
                  486, 300)) {
      ShowWindow(p->hwnd(), show);
      MSG msg{};
      while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }
  return 0;
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR cmdLine, int show) {
  if (cmdLine[0]) {
    if (!gSharedSection) return 1;
    return ChildMain(show);
  }

  SameBoat boat;
  if (!boat) return 1;

  std::unique_ptr<wchar_t[]> path =
      GetFullExePath(reinterpret_cast<HMODULE>(inst));
  if (!path) return 1;

  wchar_t writableBuf[] = L"dummy :)";
  ChildProcess child(path.get(), writableBuf);
  if (!child || !boat.AddProcess(child)) return 1;

  SharedSection section(0x1000);
  gSharedSection = section.GetHandleFor(child);
  if (!WriteProcessMemory(child,
                          &gSharedSection,
                          &gSharedSection,
                          sizeof(gSharedSection),
                          nullptr)) {
    Log(L"WriteProcessMemory failed - %08x\n", GetLastError());
  }
  gSharedSection = section;

  section.Touch();
  child.ResumeAndWait();
  return 0;
}
