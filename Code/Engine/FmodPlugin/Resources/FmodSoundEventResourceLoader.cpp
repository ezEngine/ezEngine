#include <FmodPlugin/PCH.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/FmodSingleton.h>


ezResourceLoadData ezFmodSoundEventResourceLoader::OpenDataStream(const ezResourceBase* pResource)
{
  EZ_LOG_BLOCK("ezFmodSoundEventResourceLoader::OpenDataStream", pResource->GetResourceID().GetData());

  LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

  ezResourceLoadData res;

  ezStringBuilder sResID = pResource->GetResourceID();

  const char* szSeperator = sResID.FindSubString("|");

  EZ_ASSERT_DEV(szSeperator != nullptr, "No sub-resource seperator '|' in path '{0}'", sResID.GetData());

  ezStringBuilder sBankPath, sSubPath;
  sBankPath.SetSubString_FromTo(sResID, szSeperator);
  sSubPath = szSeperator + 1;

  pData->m_hSoundBank = ezResourceManager::LoadResource<ezFmodSoundBankResource>(sBankPath);

  // make sure the sound bank is fully loaded before trying to get the event descriptor (even though we go through the Fmod 'system' the bank resource must be loaded first)
  {
    ezResourceLock<ezFmodSoundBankResource> pBank(pData->m_hSoundBank, ezResourceAcquireMode::NoFallback);

    EZ_FMOD_ASSERT(ezFmod::GetSingleton()->GetSystem()->getEvent(sSubPath.GetData(), &pData->m_pEventDescription));
  }

  // make sure to load the sample data (on this thread)
  pData->m_pEventDescription->loadSampleData();

  ezFmodSoundBankResourceHandle* pHandle = &pData->m_hSoundBank;

  ezMemoryStreamWriter w(&pData->m_Storage);
  w.WriteBytes(&pHandle, sizeof(ezFmodSoundBankResourceHandle*));
  w.WriteBytes(&pData->m_pEventDescription, sizeof(FMOD::Studio::EventDescription*));

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void ezFmodSoundEventResourceLoader::CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData)
{
  LoadedData* pData = (LoadedData*)LoaderData.m_pCustomLoaderData;

  EZ_DEFAULT_DELETE(pData);
}

bool ezFmodSoundEventResourceLoader::IsResourceOutdated(const ezResourceBase* pResource) const
{
  // if the sound bank ever gets reloaded, the sound events may be invalid, so always reload all events
  /// \todo not sure whether this can be reduced to only reloading when the bank is outdated
  return true;
}
