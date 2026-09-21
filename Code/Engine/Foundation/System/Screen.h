#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Strings/String.h>

struct ezScreenResolution
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiResolutionX = 0;
  ezUInt32 m_uiResolutionY = 0;
  ezUInt16 m_uiRefreshRate = 0;
  ezUInt8 m_uiBitsPerPixel = 0;

  inline bool operator<(const ezScreenResolution& rhs) const
  {
    if (m_uiBitsPerPixel != rhs.m_uiBitsPerPixel)
      return m_uiBitsPerPixel < rhs.m_uiBitsPerPixel;

    if (m_uiRefreshRate != rhs.m_uiRefreshRate)
      return m_uiRefreshRate < rhs.m_uiRefreshRate;

    if (m_uiResolutionX != rhs.m_uiResolutionX)
      return m_uiResolutionX < rhs.m_uiResolutionX;

    return m_uiResolutionY < rhs.m_uiResolutionY;
  }
};

/// Describes the properties of a screen
struct EZ_FOUNDATION_DLL ezScreenInfo
{
  ezString m_sDisplayID;   ///< Internal name used by the OS to identify the monitor.
  ezString m_sDisplayName; ///< Some OS provided name for the screen, typically the manufacturer and model name.

  ezInt32 m_iOffsetX;      ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  ezInt32 m_iOffsetY;      ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  ezInt32 m_iResolutionX;  ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  ezInt32 m_iResolutionY;  ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  bool m_bIsPrimary;       ///< Whether this is the primary/main screen.

  ezDynamicArray<ezScreenResolution> m_SupportedResolutions;

  /// By which factor content on this screen has to be scaled up to appear at a consistent physical size.
  ///
  /// 1.0 is the reference density (96 DPI on Windows). Does not affect the resolution above, which is in pixels.
  ///
  /// Stays at 1.0 where the value is not available, and on Windows also as long as the process didn't call
  /// ezScreen::MakeProcessDpiAware(), because such a process is told that every screen has the reference density.
  float m_fContentScale = 1.0f;
};

/// Provides functionality to detect available monitors
class EZ_FOUNDATION_DLL ezScreen
{
public:
  /// Enumerates all available screens. When it returns EZ_SUCCESS, at least one screen has been found.
  static ezResult EnumerateScreens(ezDynamicArray<ezScreenInfo>& out_screens);

  /// Prints the available screen information to the provided log.
  static void PrintScreenInfo(const ezArrayPtr<ezScreenInfo>& screens, ezLogInterface* pLog = ezLog::GetThreadLocalLogSystem());

  /// Tells the operating system that this process handles high DPI screens by itself.
  ///
  /// Only Windows needs this. A process that doesn't declare it is told scaled down screen and window sizes,
  /// renders at that lower resolution and gets bitmap stretched up to the physical pixels, which looks blurry.
  /// Once declared, every size the OS reports is in physical pixels and the application has to scale its own UI,
  /// see ezScreenInfo::m_fContentScale and ezWindowBase::GetContentScaleFactor().
  ///
  /// Must be called before the first window is created or the first screen is enumerated. The
  /// EZ_APPLICATION_ENTRY_POINT macros already do this, so only applications with their own entry point have to
  /// call it. Later calls have no effect, the awareness can only be set once per process, alternatively through
  /// the application manifest.
  ///
  /// Does nothing on the other platforms, where the application is always told the true pixel sizes.
  static void MakeProcessDpiAware();
};
