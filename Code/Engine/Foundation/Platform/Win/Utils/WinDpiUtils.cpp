#include <Foundation/FoundationPCH.h>

#include <Foundation/Platform/Win/Utils/WinDpiUtils.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

using PFN_GetDpiForMonitor = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
using PFN_GetDpiForWindow = UINT(WINAPI*)(HWND);
using PFN_AdjustWindowRectExForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
using PFN_SetProcessDpiAwarenessContext = BOOL(WINAPI*)(HANDLE);
using PFN_SetProcessDpiAwareness = HRESULT(WINAPI*)(int);
using PFN_SetProcessDPIAware = BOOL(WINAPI*)();

static bool s_bInitializedDpiFuncs = false;
static PFN_GetDpiForMonitor GetDpiForMonitorFunc = nullptr;
static PFN_GetDpiForWindow GetDpiForWindowFunc = nullptr;
static PFN_AdjustWindowRectExForDpi AdjustWindowRectExForDpiFunc = nullptr;
static PFN_SetProcessDpiAwarenessContext SetProcessDpiAwarenessContextFunc = nullptr;
static PFN_SetProcessDpiAwareness SetProcessDpiAwarenessFunc = nullptr;
static PFN_SetProcessDPIAware SetProcessDPIAwareFunc = nullptr;

static void InitializeDpiFuncs()
{
  if (s_bInitializedDpiFuncs)
    return;

  s_bInitializedDpiFuncs = true;

  HMODULE hShcore = LoadLibraryW(L"shcore.dll");
  HMODULE hUser32 = LoadLibraryW(L"user32.dll");

  if (hShcore)
  {
    GetDpiForMonitorFunc = reinterpret_cast<PFN_GetDpiForMonitor>(GetProcAddress(hShcore, "GetDpiForMonitor"));
    SetProcessDpiAwarenessFunc = reinterpret_cast<PFN_SetProcessDpiAwareness>(GetProcAddress(hShcore, "SetProcessDpiAwareness"));
  }

  if (hUser32)
  {
    GetDpiForWindowFunc = reinterpret_cast<PFN_GetDpiForWindow>(GetProcAddress(hUser32, "GetDpiForWindow"));
    AdjustWindowRectExForDpiFunc = reinterpret_cast<PFN_AdjustWindowRectExForDpi>(GetProcAddress(hUser32, "AdjustWindowRectExForDpi"));
    SetProcessDpiAwarenessContextFunc = reinterpret_cast<PFN_SetProcessDpiAwarenessContext>(GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));
    SetProcessDPIAwareFunc = reinterpret_cast<PFN_SetProcessDPIAware>(GetProcAddress(hUser32, "SetProcessDPIAware"));
  }
}

ezUInt32 ezWindowsDpiUtils::GetWindowDpi(ezMinWindows::HWND hWnd)
{
  InitializeDpiFuncs();

  if (GetDpiForWindowFunc != nullptr)
  {
    if (const ezUInt32 uiDpi = GetDpiForWindowFunc(ezMinWindows::ToNative(hWnd)); uiDpi != 0)
      return uiDpi;
  }

  return ReferenceDpi;
}

ezUInt32 ezWindowsDpiUtils::GetDpiForMonitor(ezMinWindows::HMONITOR pMonitor)
{
  InitializeDpiFuncs();

  if (GetDpiForMonitorFunc)
  {
    UINT uiDpiX = 0, uiDpiY = 0;
    if (SUCCEEDED(GetDpiForMonitorFunc(ezMinWindows::ToNative(pMonitor), 0 /* MDT_EFFECTIVE_DPI */, &uiDpiX, &uiDpiY)) && uiDpiX != 0)
      return uiDpiX;
  }

  return ReferenceDpi;
}

UINT ezWindowsDpiUtils::GetDpiAtPosition(ezInt32 iPosX, ezInt32 iPosY)
{
  InitializeDpiFuncs();

  if (GetDpiForMonitorFunc)
  {
    const POINT pt = {iPosX, iPosY};
    const HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);

    UINT uiDpiX = 0, uiDpiY = 0;
    if (hMonitor != nullptr && SUCCEEDED(GetDpiForMonitorFunc(hMonitor, 0 /* MDT_EFFECTIVE_DPI */, &uiDpiX, &uiDpiY)) && uiDpiX != 0)
      return uiDpiX;
  }

  return ReferenceDpi;
}

void ezWindowsDpiUtils::AdjustWindowRectForDpi(ezRectI32& ref_rect, ezUInt32 uiWindowStyle, ezUInt32 uiExStyle, ezUInt32 uiDpi)
{
  InitializeDpiFuncs();

  RECT r;
  r.left = ref_rect.Left();
  r.right = ref_rect.Right();
  r.top = ref_rect.Top();
  r.bottom = ref_rect.Bottom();

  if (AdjustWindowRectExForDpiFunc == nullptr || AdjustWindowRectExForDpiFunc(&r, uiWindowStyle, FALSE, uiExStyle, uiDpi) == FALSE)
  {
    AdjustWindowRectEx(&r, uiWindowStyle, FALSE, uiExStyle);
  }

  ref_rect.x = r.left;
  ref_rect.y = r.top;
  ref_rect.width = r.right - r.left;
  ref_rect.height = r.bottom - r.top;
}

ezSizeU32 ezWindowsDpiUtils::ComputeWindowSizeForDpi(const ezSizeU32& clientSize, ezUInt32 uiWindowStyle, ezUInt32 uiExStyle, ezUInt32 uiDpi)
{
  ezRectI32 rect(0, 0, (ezInt32)clientSize.width, (ezInt32)clientSize.height);
  AdjustWindowRectForDpi(rect, uiWindowStyle, uiExStyle, uiDpi);

  return ezSizeU32(rect.Right() - rect.Left(), rect.Bottom() - rect.Top());
}

void ezWindowsDpiUtils::MakeProcessDpiAware()
{
  InitializeDpiFuncs();

  // 'per monitor v2' (Windows 10 1703 and later). In contrast to the older 'per monitor' mode this also scales
  // the non-client area (title bar, menus, scroll bars) and child dialogs automatically.
  if (SetProcessDpiAwarenessContextFunc)
  {
    // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 / _PER_MONITOR_AWARE, spelled out to not depend on the SDK version.
    // Both fail with ERROR_ACCESS_DENIED when the awareness was already set (manifest, earlier call), which is fine.
    if (SetProcessDpiAwarenessContextFunc(reinterpret_cast<HANDLE>(-4)) == FALSE)
    {
      SetProcessDpiAwarenessContextFunc(reinterpret_cast<HANDLE>(-3));
    }

    return;
  }

  // Windows 8.1 and later
  if (SetProcessDpiAwarenessFunc)
  {
    if (SUCCEEDED(SetProcessDpiAwarenessFunc(2 /* PROCESS_PER_MONITOR_DPI_AWARE */)))
      return;
  }

  // last resort: system wide DPI awareness, the process gets the scaling of the primary screen at startup
  // and is virtualized on every screen that differs from it


  if (SetProcessDPIAwareFunc)
  {
    SetProcessDPIAwareFunc();
  }
}

#endif
