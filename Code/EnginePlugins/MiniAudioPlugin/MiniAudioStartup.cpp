#include <MiniAudioPlugin/MiniAudioPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Configuration/Startup.h>
#include <MiniAudioPlugin/MiniAudioSingleton.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(MiniAudio, MiniAudioPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(&ezMiniAudioSingleton::GameApplicationEventHandler);

    ezMiniAudioSingleton::GetSingleton()->Startup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(&ezMiniAudioSingleton::GameApplicationEventHandler);

    ezMiniAudioSingleton::GetSingleton()->Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on
