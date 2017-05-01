#include <PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <FmodPlugin/FmodSingleton.h>
#include <GameEngine/GameApplication/GameApplication.h>

static ezFmodSoundBankResourceLoader s_SoundBankResourceLoader;
static ezFmodSoundEventResourceLoader s_SoundEventResourceLoader;

EZ_BEGIN_SUBSYSTEM_DECLARATION(Fmod, FmodPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
  }

  ON_CORE_SHUTDOWN
  {
  }

  ON_ENGINE_STARTUP
  {
    ezResourceManager::SetResourceTypeLoader<ezFmodSoundBankResource>(&s_SoundBankResourceLoader);
    ezResourceManager::SetResourceTypeLoader<ezFmodSoundEventResource>(&s_SoundEventResourceLoader);
    ezFmod::GetSingleton()->Startup();

    ezResourceManager::RegisterResourceForAssetType("Sound Event", ezGetStaticRTTI<ezFmodSoundEventResource>());

    ezFmodSoundEventResourceDescriptor desc;
    ezFmodSoundEventResourceHandle hResource = ezResourceManager::CreateResource<ezFmodSoundEventResource>("FmodEventMissing", desc, "Fallback for missing Sound event");
    ezFmodSoundEventResource::SetTypeMissingResource(hResource);

    /// \todo Missing sound bank resource

    ezGameApplication::GetGameApplicationInstance()->m_Events.AddEventHandler(&ezFmod::GameApplicationEventHandler);
  }

  ON_ENGINE_SHUTDOWN
  {
    ezGameApplication::GetGameApplicationInstance()->m_Events.RemoveEventHandler(&ezFmod::GameApplicationEventHandler);

  ezFmod::GetSingleton()->Shutdown();
  ezResourceManager::SetResourceTypeLoader<ezFmodSoundBankResource>(nullptr);
  ezResourceManager::SetResourceTypeLoader<ezFmodSoundEventResource>(nullptr);
  }

EZ_END_SUBSYSTEM_DECLARATION



EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_FmodStartup);

