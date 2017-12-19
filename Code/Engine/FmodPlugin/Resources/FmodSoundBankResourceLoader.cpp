#include <PCH.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/FmodSingleton.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Core/Assets/AssetFileHeader.h>
#include <FmodPlugin/FmodIncludes.h>

ezResourceLoadData ezFmodSoundBankResourceLoader::OpenDataStream(const ezResourceBase* pResource)
{
  EZ_LOG_BLOCK("ezFmodSoundBankResourceLoader::OpenDataStream", pResource->GetResourceID().GetData());

  LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

  ezResourceLoadData res;

  {
    ezFileReader SoundBankAssetFile;
    if (SoundBankAssetFile.Open(pResource->GetResourceID()).Failed())
      return res;

    res.m_sResourceDescription = SoundBankAssetFile.GetFilePathRelative().GetData();
    ezUInt32 uiSoundBankSize = 0;

    if (SoundBankAssetFile.GetFilePathRelative().EndsWith_NoCase("ezFmodSoundBank")) // a transformed asset file
    {
      // skip the asset header
      ezAssetFileHeader header;
      header.Read(SoundBankAssetFile);

      ezUInt8 uiVersion = 0;
      SoundBankAssetFile >> uiVersion;

      EZ_ASSERT_DEV(uiVersion == 1, "Soundbank resource file version '{0}' is invalid", uiVersion);

      SoundBankAssetFile >> uiSoundBankSize;
    }
    else
    {
      // otherwise we assume it is directly an fmod sound bank file
      uiSoundBankSize = (ezUInt32)SoundBankAssetFile.GetFileSize();
    }

    if (uiSoundBankSize > 0)
    {
      pData->m_pSoundbankData = EZ_DEFAULT_NEW(ezDataBuffer);
      pData->m_pSoundbankData->SetCountUninitialized(uiSoundBankSize + FMOD_STUDIO_LOAD_MEMORY_ALIGNMENT);
      ezUInt8* pAlignedData = ezMemoryUtils::Align(pData->m_pSoundbankData->GetData() + FMOD_STUDIO_LOAD_MEMORY_ALIGNMENT, FMOD_STUDIO_LOAD_MEMORY_ALIGNMENT);

      SoundBankAssetFile.ReadBytes(pAlignedData, uiSoundBankSize);

      // The fmod documentation says it is fully thread-safe, so I assume we can call loadBankMemory at any time
      auto pStudio = ezFmod::GetSingleton()->GetStudioSystem();

      // this happens when fmod is not properly configured
      if (pStudio == nullptr)
        return res;

      auto fmodRes = pStudio->loadBankMemory((const char*)pAlignedData, (int)uiSoundBankSize, FMOD_STUDIO_LOAD_MEMORY_POINT, FMOD_STUDIO_LOAD_BANK_NORMAL, &pData->m_pSoundBank);

      // if this fails with res == FMOD_ERR_NOTREADY, that might be because two processes using fmod are running and both have the FMOD_STUDIO_INIT_LIVEUPDATE flag set
      // somehow fmod cannot handle this and bank loading then fails
      if (fmodRes != FMOD_OK)
      {
        ezLog::Error("Error '{1}' loading fmod sound bank '{0}'", SoundBankAssetFile.GetFilePathRelative().GetData(), (ezInt32)fmodRes);

        EZ_DEFAULT_DELETE(pData->m_pSoundbankData);
        EZ_DEFAULT_DELETE(pData);

        res.m_pCustomLoaderData = nullptr;
        res.m_pDataStream = nullptr;
        return res;
      }
    }

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
    {
      ezFileStats stat;
      if (ezOSFile::GetFileStats(SoundBankAssetFile.GetFilePathAbsolute(), stat).Succeeded())
      {
        res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
      }
    }
#endif
  }

  ezMemoryStreamWriter w(&pData->m_Storage);

  w.WriteBytes(&pData->m_pSoundBank, sizeof(FMOD::Studio::Bank*));
  w.WriteBytes(&pData->m_pSoundbankData, sizeof(ezDataBuffer*));

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
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    ezStringBuilder sAbs;
    if (ezFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
      return false;

    ezFileStats stat;
    if (ezOSFile::GetFileStats(sAbs, stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), ezTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;

#else

  return false; // we cannot reload these resources without overhead, so only allow this during development

#endif
}



EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_Resources_FmodSoundBankResourceLoader);

