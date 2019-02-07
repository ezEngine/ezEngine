#pragma once

#include <System/SystemDLL.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Strings/String.h>

/// \brief Describes the properties of a screen
struct EZ_SYSTEM_DLL ezScreenInfo
{
  ezString m_sDisplayName;///< Some OS provided name for the screen, typically the manufacturer and model name.

  ezInt32 m_iOffsetX; ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  ezInt32 m_iOffsetY; ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  ezInt32 m_iResolutionX; ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  ezInt32 m_iResolutionY; ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  bool m_bIsPrimary; ///< Whether this is the primary/main screen.
};

/// \brief Provides functionality to detect available monitors
class EZ_SYSTEM_DLL ezScreen
{
public:
  /// \brief Enumerates all available screens. When it returns EZ_SUCCESS, at least one screen has been found.
  static ezResult EnumerateScreens(ezHybridArray<ezScreenInfo, 2>& out_Screens);

  /// \brief Prints the available screen information to the provided log.
  static void PrintScreenInfo(const ezHybridArray<ezScreenInfo, 2>& screens, ezLogInterface* pLog = ezLog::GetThreadLocalLogSystem());
};

