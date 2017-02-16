#include <PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <FmodPlugin/FmodWorldModule.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>

static ezFmodSoundBankResourceLoader s_SoundBankResourceLoader;
static ezFmodSoundEventResourceLoader s_SoundEventResourceLoader;

EZ_BEGIN_SUBSYSTEM_DECLARATION(Fmod, FmodPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    /// \todo Setup 'missing' sound resource (fallback)

    ezResourceManager::SetResourceTypeLoader<ezFmodSoundBankResource>(&s_SoundBankResourceLoader);
    ezResourceManager::SetResourceTypeLoader<ezFmodSoundEventResource>(&s_SoundEventResourceLoader);
    ezFmod::GetSingleton()->Startup();
  }

  ON_CORE_SHUTDOWN
  {
    ezFmod::GetSingleton()->Shutdown();
    ezResourceManager::SetResourceTypeLoader<ezFmodSoundBankResource>(nullptr);
    ezResourceManager::SetResourceTypeLoader<ezFmodSoundEventResource>(nullptr);
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION



EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_FmodStartup);

