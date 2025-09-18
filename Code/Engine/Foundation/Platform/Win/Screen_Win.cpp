#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/System/Screen.h>

EZ_DEFINE_AS_POD_TYPE(DISPLAYCONFIG_PATH_INFO);
EZ_DEFINE_AS_POD_TYPE(DISPLAYCONFIG_MODE_INFO);

static void QueryMonitorNames(ezMap<ezString, ezString>& out_deviceIDtoName)
{
  out_deviceIDtoName.Clear();

  ezHybridArray<DISPLAYCONFIG_PATH_INFO, 4> paths;
  ezHybridArray<DISPLAYCONFIG_MODE_INFO, 4> modes;
  UINT32 flags = QDC_ONLY_ACTIVE_PATHS;
  LONG isError = ERROR_INSUFFICIENT_BUFFER;

  UINT32 pathCount, modeCount;
  isError = GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount);

  if (isError)
    return;

  // Allocate the path and mode arrays
  paths.SetCount(pathCount);
  modes.SetCount(modeCount);

  // Get all active paths and their modes
  isError = QueryDisplayConfig(flags, &pathCount, paths.GetData(), &modeCount, modes.GetData(), nullptr);

  // The function may have returned fewer paths/modes than estimated
  paths.SetCount(pathCount);
  modes.SetCount(modeCount);

  if (isError)
    return;

  ezStringBuilder tmp;

  // For each active path
  for (ezUInt32 i = 0; i < paths.GetCount(); i++)
  {
    // Find the target (monitor) friendly name
    DISPLAYCONFIG_TARGET_DEVICE_NAME targetName = {};
    targetName.header.adapterId = paths[i].targetInfo.adapterId;
    targetName.header.id = paths[i].targetInfo.id;
    targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    targetName.header.size = sizeof(targetName);
    isError = DisplayConfigGetDeviceInfo(&targetName.header);

    if (isError)
      return;

    if (targetName.flags.friendlyNameFromEdid)
    {
      tmp = targetName.monitorDevicePath;
      out_deviceIDtoName[tmp] = targetName.monitorFriendlyDeviceName;
    }
  }
}

static void EnumerateDisplayModes(ezStringView sDeviceName, ezDynamicArray<ezScreenResolution>& inout_modes)
{
  inout_modes.Clear();

  const ezStringWChar wName(sDeviceName);

  DEVMODEW devMode = {};
  devMode.dmSize = sizeof(DEVMODEW);

  int modeNum = 0;
  while (EnumDisplaySettingsW(wName.GetData(), modeNum++, &devMode))
  {
    ezScreenResolution& mode = inout_modes.ExpandAndGetRef();
    mode.m_uiResolutionX = devMode.dmPelsWidth;
    mode.m_uiResolutionY = devMode.dmPelsHeight;
    mode.m_uiBitsPerPixel = static_cast<ezUInt8>(devMode.dmBitsPerPel);
    mode.m_uiRefreshRate = static_cast<ezUInt16>(devMode.dmDisplayFrequency);
  }

  inout_modes.Sort();
}

static BOOL CALLBACK ezMonitorEnumProc(HMONITOR pMonitor, HDC pHdcMonitor, LPRECT pLprcMonitor, LPARAM data)
{
  EZ_IGNORE_UNUSED(pHdcMonitor);
  EZ_IGNORE_UNUSED(pLprcMonitor);

  ezHybridArray<ezScreenInfo, 2>* pScreens = (ezHybridArray<ezScreenInfo, 2>*)data;

  MONITORINFOEXW info;
  info.cbSize = sizeof(info);

  if (!GetMonitorInfoW(pMonitor, &info))
    return TRUE;

  // In Windows screen coordinates are from top/left to bottom/right
  // ie. 0,0 is left/top , resx/resy is right/bottom

  auto& mon = pScreens->ExpandAndGetRef();
  mon.m_iOffsetX = info.rcMonitor.left;
  mon.m_iOffsetY = info.rcMonitor.top;
  mon.m_iResolutionX = info.rcMonitor.right - info.rcMonitor.left;
  mon.m_iResolutionY = info.rcMonitor.bottom - info.rcMonitor.top;
  mon.m_sDisplayID = info.szDevice;
  mon.m_sDisplayName = info.szDevice;
  mon.m_bIsPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;

  DISPLAY_DEVICEW ddev;
  ddev.cb = sizeof(ddev);

  ezMap<ezString, ezString> monitorNames;
  QueryMonitorNames(monitorNames);

  if (EnumDisplayDevicesW(info.szDevice, 0, &ddev, 1) != FALSE)
  {
    ezStringBuilder tmp;
    tmp = ddev.DeviceID;

    if (auto it = monitorNames.Find(tmp); it.IsValid())
    {
      mon.m_sDisplayName = it.Value();
    }
    else
    {
      mon.m_sDisplayName = ddev.DeviceString;
    }

    EnumerateDisplayModes(mon.m_sDisplayID, mon.m_SupportedResolutions);
  }

  return TRUE;
}

ezResult ezScreen::EnumerateScreens(ezDynamicArray<ezScreenInfo>& out_screens)
{
  out_screens.Clear();
  if (EnumDisplayMonitors(nullptr, nullptr, ezMonitorEnumProc, (LPARAM)&out_screens) == FALSE)
    return EZ_FAILURE;

  if (out_screens.IsEmpty())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

#endif
