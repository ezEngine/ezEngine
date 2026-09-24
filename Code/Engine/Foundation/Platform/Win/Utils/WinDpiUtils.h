#pragma once

#include <Foundation/Basics.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Math/Rect.h>
#  include <Foundation/Math/Size.h>
#  include <Foundation/Platform/Win/Utils/MinWindows.h>

struct EZ_FOUNDATION_DLL ezWindowsDpiUtils
{
  /// The display density that a content scale of 1.0 corresponds to.
  static constexpr ezUInt32 ReferenceDpi = 96;

  static constexpr float DpiToContentScale(ezUInt32 uiDpi)
  {
    return (float)uiDpi / (float)ReferenceDpi;
  }

  /// Returns the DPI of the display that the given window is on.
  ///
  /// Returns ReferenceDpi (ie. "no scaling") for a process that is not DPI aware, because such a process is
  /// told that everything is 96 DPI.
  static ezUInt32 GetWindowDpi(ezMinWindows::HWND hWnd);

  /// Returns the DPI of the given display, or ReferenceDpi when it can't be determined.
  static ezUInt32 GetDpiForMonitor(ezMinWindows::HMONITOR pMonitor);

  /// Returns the DPI of the display that contains the given position on the virtual desktop.
  ///
  /// Needed to size a window before it exists. Positions that are not on any display fall back to the primary one.
  static ezUInt32 GetDpiAtPosition(ezInt32 iPosX, ezInt32 iPosY);

  /// Grows the given client area rectangle by the size of the window decorations at the given DPI.
  ///
  /// The plain AdjustWindowRectEx() always uses the DPI of the primary display, which gives a window on a
  /// differently scaled display a client area that is off by the difference between the two.
  static void AdjustWindowRectForDpi(ezRectI32& ref_rect, ezUInt32 uiWindowStyle, ezUInt32 uiExStyle, ezUInt32 uiDpi);

  /// Computes the outer window size (including decorations) for a given client area size, at a given DPI.
  static ezSizeU32 ComputeWindowSizeForDpi(const ezSizeU32& clientSize, ezUInt32 uiWindowStyle, ezUInt32 uiExStyle, ezUInt32 uiDpi);

  /// Tells the operating system that this process handles high DPI screens by itself.
  ///
  /// A process that doesn't declare it is told scaled down screen and window sizes,
  /// renders at that lower resolution and gets bitmap stretched up to the physical pixels, which looks blurry.
  /// Once declared, every size the OS reports is in physical pixels and the application has to scale its own UI,
  /// see ezScreenInfo::m_fContentScale and ezWindowBase::GetContentScaleFactor().
  ///
  /// Must be called before the first window is created or the first screen is enumerated. The
  /// EZ_APPLICATION_ENTRY_POINT macros already do this, so only applications with their own entry point have to
  /// call it.
  static void MakeProcessDpiAware();
};

#endif
