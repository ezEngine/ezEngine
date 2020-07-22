#include <FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/System/StackTracer.h>

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#    include <Foundation/System/Implementation/Win/StackTracer_uwp.h>
#  else
#    include <Foundation/System/Implementation/Win/StackTracer_win.h>
#  endif

#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <Foundation/System/Implementation/Posix/StackTracer_posix.h>
#elif EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Android/StackTracer_android.h>
#else
#  error "StackTracer is not implemented on current platform"
#endif

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, StackTracer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezPlugin::s_PluginEvents.AddEventHandler(ezStackTracer::OnPluginEvent);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezPlugin::s_PluginEvents.RemoveEventHandler(ezStackTracer::OnPluginEvent);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

EZ_STATICLINK_FILE(Foundation, Foundation_System_Implementation_StackTracer);
