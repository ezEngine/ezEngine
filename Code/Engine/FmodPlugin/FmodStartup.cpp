#include <FmodPlugin/PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <FmodPlugin/FmodSceneModule.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>

static ezFmodSoundBankResourceLoader s_SoundBankResourceLoader;

EZ_BEGIN_SUBSYSTEM_DECLARATION(Fmod, FmodPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezResourceManager::SetResourceTypeLoader<ezFmodSoundBankResource>(&s_SoundBankResourceLoader);
    ezFmod::GetSingleton()->Startup();
  }

  ON_CORE_SHUTDOWN
  {
    ezFmod::GetSingleton()->Shutdown();
  ezResourceManager::SetResourceTypeLoader<ezFmodSoundBankResource>(nullptr);
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION
