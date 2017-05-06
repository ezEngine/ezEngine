#include <PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <FmodPlugin/FmodSingleton.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

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

    ezResourceManager::RegisterResourceForAssetType("Sound Event", ezGetStaticRTTI<ezFmodSoundEventResource>());

    {
      ezFmodSoundEventResourceDescriptor desc;
      ezFmodSoundEventResourceHandle hResource = ezResourceManager::CreateResource<ezFmodSoundEventResource>("FmodEventMissing", desc, "Fallback for missing sound event");
      ezFmodSoundEventResource::SetTypeMissingResource(hResource);
    }

    {
      ezFmodSoundBankResourceDescriptor desc;
      ezFmodSoundBankResourceHandle hResource = ezResourceManager::CreateResource<ezFmodSoundBankResource>("FmodBankMissing", desc, "Fallback for missing sound bank");
      ezFmodSoundBankResource::SetTypeMissingResource(hResource);
    }

    ezGameApplication::GetGameApplicationInstance()->m_Events.AddEventHandler(&ezFmod::GameApplicationEventHandler);

    ezFmod::GetSingleton()->Startup();
  }

  ON_ENGINE_SHUTDOWN
  {
    ezGameApplication::GetGameApplicationInstance()->m_Events.RemoveEventHandler(&ezFmod::GameApplicationEventHandler);

  ezFmod::GetSingleton()->Shutdown();
  ezResourceManager::SetResourceTypeLoader<ezFmodSoundBankResource>(nullptr);
  ezResourceManager::SetResourceTypeLoader<ezFmodSoundEventResource>(nullptr);
  }

EZ_END_SUBSYSTEM_DECLARATION

void ezFmodConfiguration::Save(ezOpenDdlWriter& ddl) const
{
    ezOpenDdlUtils::StoreString(ddl, m_sMasterSoundBank, "MasterBank");
    ezOpenDdlUtils::StoreUInt16(ddl, m_uiVirtualChannels, "VirtualChannels");
    ezOpenDdlUtils::StoreUInt32(ddl, m_uiSamplerRate, "SamplerRate");

    switch (m_SpeakerMode)
    {
    case FMOD_SPEAKERMODE_STEREO:
      ezOpenDdlUtils::StoreString(ddl, "Stereo", "Mode");
      break;
    case FMOD_SPEAKERMODE_5POINT1:
      ezOpenDdlUtils::StoreString(ddl, "5.1", "Mode");
      break;
    case FMOD_SPEAKERMODE_7POINT1:
      ezOpenDdlUtils::StoreString(ddl, "7.1", "Mode");
      break;
    }
}

void ezFmodConfiguration::Load(const ezOpenDdlReaderElement& ddl)
{
  if (const ezOpenDdlReaderElement* pElement = ddl.FindChildOfType(ezOpenDdlPrimitiveType::String, "MasterBank"))
  {
    m_sMasterSoundBank = pElement->GetPrimitivesString()[0];
  }

  if (const ezOpenDdlReaderElement* pElement = ddl.FindChildOfType(ezOpenDdlPrimitiveType::UInt16, "VirtualChannels"))
  {
    m_uiVirtualChannels = pElement->GetPrimitivesUInt16()[0];
  }

  if (const ezOpenDdlReaderElement* pElement = ddl.FindChildOfType(ezOpenDdlPrimitiveType::UInt32, "SamplerRate"))
  {
    m_uiSamplerRate = pElement->GetPrimitivesUInt32()[0];
  }

  if (const ezOpenDdlReaderElement* pElement = ddl.FindChildOfType(ezOpenDdlPrimitiveType::String, "Mode"))
  {
    auto mode = pElement->GetPrimitivesString()[0];

    if (mode == "Stereo")
      m_SpeakerMode = FMOD_SPEAKERMODE_STEREO;
    else if (mode == "7.1")
      m_SpeakerMode = FMOD_SPEAKERMODE_7POINT1;
    else
      m_SpeakerMode = FMOD_SPEAKERMODE_5POINT1;
  }
}

ezResult ezFmodPlatformConfigs::Save(const char* szFile) const
{
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  ezOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  for (auto it = m_PlatformConfigs.GetIterator(); it.IsValid(); ++it)
  {
    ddl.BeginObject("Platform", it.Key());

    it.Value().Save(ddl);

    ddl.EndObject();
  }

  return EZ_SUCCESS;
}

ezResult ezFmodPlatformConfigs::Load(const char* szFile)
{
  m_PlatformConfigs.Clear();

  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  ezOpenDdlReader ddl;
  EZ_SUCCEED_OR_RETURN(ddl.ParseDocument(file));

  const ezOpenDdlReaderElement* pRoot = ddl.GetRootElement();
  const ezOpenDdlReaderElement* pChild = pRoot->GetFirstChild();

  while (pChild)
  {
    if (pChild->IsCustomType("Platform") && pChild->HasName())
    {
      auto& cfg = m_PlatformConfigs[pChild->GetName()];

      cfg.Load(*pChild);
    }

    pChild = pChild->GetSibling();
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_FmodStartup);

