#pragma once

#include <Foundation/Basics.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/FoundationInternal.h>
#  include <Foundation/Logging/Log.h>

EZ_FOUNDATION_INTERNAL_HEADER

class ezETWProvider
{
public:
  ezETWProvider();
  ~ezETWProvider();

  void LogMessge(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, const char* szText);

  static ezETWProvider& GetInstance();
};
#endif
