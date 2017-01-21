#include <System/PCH.h>
#include <Foundation/Logging/Log.h>

BOOL CALLBACK ezMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
  ezHybridArray<ezScreenInfo, 2>* pScreens = (ezHybridArray<ezScreenInfo, 2>*) dwData;

  MONITORINFOEXW info;
  info.cbSize = sizeof(info);

  if (!GetMonitorInfoW(hMonitor, &info))
    return TRUE;

  // In Windows screen coordinates are from top/left to bottom/right
  // ie. 0,0 is left/top , resx/resy is right/bottom

  auto& mon = pScreens->ExpandAndGetRef();
  mon.m_iOffsetX = info.rcMonitor.left;
  mon.m_iOffsetY = info.rcMonitor.top;
  mon.m_iResolutionX = info.rcMonitor.right - info.rcMonitor.left;
  mon.m_iResolutionY = info.rcMonitor.bottom - info.rcMonitor.top;
  mon.m_sDisplayName = info.szDevice;
  mon.m_bIsPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;

  DISPLAY_DEVICEW ddev;
  ddev.cb = sizeof(ddev);

  if (EnumDisplayDevicesW(info.szDevice, 0, &ddev, 1) == TRUE)
  {
    mon.m_sDisplayName = ddev.DeviceString;
  }

  return TRUE;
}

ezResult ezScreen::EnumerateScreens(ezHybridArray<ezScreenInfo, 2>& out_Screens)
{
  out_Screens.Clear();
  if (EnumDisplayMonitors(nullptr, nullptr, ezMonitorEnumProc, (LPARAM)&out_Screens) == FALSE)
    return EZ_FAILURE;

  if (out_Screens.IsEmpty())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}


