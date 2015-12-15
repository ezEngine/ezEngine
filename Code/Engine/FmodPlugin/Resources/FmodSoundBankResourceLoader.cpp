#include <FmodPlugin/PCH.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <FmodPlugin/FmodSingleton.h>


ezResourceLoadData ezFmodSoundBankResourceLoader::OpenDataStream(const ezResourceBase* pResource)
{
  EZ_LOG_BLOCK("ezFmodSoundBankResourceLoader::OpenDataStream", pResource->GetResourceID().GetData());

  LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

  ezResourceLoadData res;

  {
    ezString sAbsolutePath;
    if (ezFileSystem::ResolvePath(pResource->GetResourceID().GetData(), false, &sAbsolutePath, nullptr).Failed())
    {
      ezLog::Error("Failed to resolve resource ID to absolute path: '%s'", pResource->GetResourceID().GetData());
      return res;
    }

    // The Fmod documentation says it is fully thread-safe, so I assume we can call loadBankFile at any time
    pData->m_pSoundBank = nullptr;
    EZ_FMOD_ASSERT(ezFmod::GetSingleton()->GetSystem()->loadBankFile(sAbsolutePath.GetData(), FMOD_STUDIO_LOAD_BANK_NORMAL, &pData->m_pSoundBank));

    res.m_sResourceDescription = sAbsolutePath;

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
    {
      ezFileStats stat;
      if (ezOSFile::GetFileStats(sAbsolutePath, stat).Succeeded())
      {
        res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
      }
    }
#endif

  }

  ezMemoryStreamWriter w(&pData->m_Storage);

  FMOD::Studio::Bank* pBank = pData->m_pSoundBank;
  w.WriteBytes(&pBank, sizeof(FMOD::Studio::Bank*));

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void ezFmodSoundBankResourceLoader::CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData)
{
  LoadedData* pData = (LoadedData*)LoaderData.m_pCustomLoaderData;

  EZ_DEFAULT_DELETE(pData);
}

bool ezFmodSoundBankResourceLoader::IsResourceOutdated(const ezResourceBase* pResource) const
{
#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    ezString sAbs;
    if (ezFileSystem::ResolvePath(pResource->GetResourceID(), false, &sAbs, nullptr).Failed())
      return false;

    ezFileStats stat;
    if (ezOSFile::GetFileStats(sAbs, stat).Failed())
      return false;

    return !stat.m_LastModificationTime.IsEqual(pResource->GetLoadedFileModificationTime(), ezTimestamp::CompareMode::FileTime);
  }

#endif

  return true;
}
