#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>
#include <AudioSystemPlugin/Resources/AudioControlCollectionResource.h>

#include <Foundation/Configuration/Startup.h>

static ezAudioSystemAllocator* s_pAudioSystemAllocator = nullptr;
static ezAudioMiddlewareAllocator* s_pAudioMiddlewareAllocator = nullptr;
static ezAudioSystem* s_pAudioSystemSingleton = nullptr;

EZ_BEGIN_SUBSYSTEM_DECLARATION(AudioSystem, AudioSystemPlugin)

  // clang-format off
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES
  // clang-format on

  ON_CORESYSTEMS_STARTUP
  {
    s_pAudioSystemAllocator = new ezAudioSystemAllocator();
    s_pAudioMiddlewareAllocator = new ezAudioMiddlewareAllocator(s_pAudioSystemAllocator);
    s_pAudioSystemSingleton = new ezAudioSystem();

    // Audio Control Collection Resources
    {
      ezResourceManager::RegisterResourceForAssetType("Audio Control Collection", ezGetStaticRTTI<ezAudioControlCollectionResource>());

      ezAudioControlCollectionResourceDescriptor desc;
      const auto hResource = ezResourceManager::CreateResource<ezAudioControlCollectionResource>("AudioControlCollectionMissing", std::move(desc), "Fallback for missing audio control collections.");
      ezResourceManager::SetResourceTypeMissingFallback<ezAudioControlCollectionResource>(hResource);
    }
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(&ezAudioSystem::GameApplicationEventHandler);

    ezAudioSystem::GetSingleton()->Startup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezAudioSystem::GetSingleton()->Shutdown();

    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(&ezAudioSystem::GameApplicationEventHandler);
  }

EZ_END_SUBSYSTEM_DECLARATION;

EZ_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_AudioSystemStartup);
