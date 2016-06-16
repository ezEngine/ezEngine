#pragma once

#include <System/Basics.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Strings/String.h>

struct EZ_SYSTEM_DLL ezScreenInfo
{
  ezString m_sDisplayName;

  ezInt32 m_iOffsetX;
  ezInt32 m_iOffsetY;
  ezInt32 m_iResolutionX;
  ezInt32 m_iResolutionY;
};

class EZ_SYSTEM_DLL ezScreen
{
public:
  /// \brief Enumerates all available screens
  static void EnumerateScreens(ezHybridArray<ezScreenInfo, 2>& out_Screens);


};