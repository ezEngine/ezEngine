#include <FmodPluginPCH.h>

#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

ezResourceLoadData ezFmodSoundEventResourceLoader::OpenDataStream(const ezResource* pResource)
{
  EZ_LOG_BLOCK("ezFmodSoundEventResourceLoader::OpenDataStream", pResource->GetResourceID().GetData());

  LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

  ezResourceLoadData res;

  if (ezFmod::GetSingleton()->GetStudioSystem() == nullptr)
  {
    ezLog::Warning("Fmod not initialized, ignoring sound event.");
    return res;
  }

  ezStringBuilder sResID;
  ezFileSystem::ResolveAssetRedirection(pResource->GetResourceID(), sResID);

  const char* szSeperator = sResID.FindSubString("|");

  if (szSeperator == nullptr)
  {
    ezLog::Error("Fmod event resource ID is invalid or could not be resolved: '{0}'", sResID);
    return res;
  }

  ezStringBuilder sBankPath, sSubPath;
  sBankPath.SetSubString_FromTo(sResID, szSeperator);
  sSubPath = szSeperator + 1;

  pData->m_hSoundBank = ezResourceManager::LoadResource<ezFmodSoundBankResource>(sBankPath);

  // make sure the sound bank is fully loaded before trying to get the event descriptor (even though we go through the Fmod 'system' the
  // bank resource must be loaded first)
  {
    ezResourceLock<ezFmodSoundBankResource> pBank(pData->m_hSoundBank, ezResourceAcquireMode::NoFallback);

    if (ezConversionUtils::IsStringUuid(sSubPath))
    {
      const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sSubPath);
      const FMOD_GUID* fmodGuid = reinterpret_cast<const FMOD_GUID*>(&guid);

      if (ezFmod::GetSingleton()->GetStudioSystem()->getEventByID(fmodGuid, &pData->m_pEventDescription) != FMOD_OK)
      {
        ezLog::Error("Fmod event could not be found. GUID: '{0}'", sSubPath);
        return res;
      }
    }
    else
    {
      if (ezFmod::GetSingleton()->GetStudioSystem()->getEvent(sSubPath.GetData(), &pData->m_pEventDescription) != FMOD_OK)
      {
        ezLog::Error("Fmod event could not be found. Path: '{0}'", sSubPath);
        return res;
      }
    }
  }

  // make sure to load the sample data (on this thread)
  if (pData->m_pEventDescription->loadSampleData() != FMOD_OK)
  {
    ezLog::Error("Fmod event sample data could not be loaded. Event: '{0}'", sSubPath);
    return res;
  }

  ezFmodSoundBankResourceHandle* pHandle = &pData->m_hSoundBank;

  ezMemoryStreamWriter w(&pData->m_Storage);
  w.WriteBytes(&pHandle, sizeof(ezFmodSoundBankResourceHandle*));
  w.WriteBytes(&pData->m_pEventDescription, sizeof(FMOD::Studio::EventDescription*));

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void ezFmodSoundEventResourceLoader::CloseDataStream(const ezResource* pResource, const ezResourceLoadData& LoaderData)
{
  LoadedData* pData = (LoadedData*)LoaderData.m_pCustomLoaderData;

  EZ_DEFAULT_DELETE(pData);
}

bool ezFmodSoundEventResourceLoader::IsResourceOutdated(const ezResource* pResource) const
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

  // if the sound bank ever gets reloaded, the sound events may be invalid, so always reload all events
  /// \todo not sure whether this can be reduced to only reloading when the bank is outdated
  return true;

#else

  return false; // we cannot reload these resources without overhead, so only allow this during development

#endif
}



EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_Resources_FmodSoundEventResourceLoader);
