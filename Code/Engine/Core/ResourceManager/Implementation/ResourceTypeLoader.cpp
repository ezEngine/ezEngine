#include <Core/PCH.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>

struct FileResourceLoadData
{
  FileResourceLoadData() : m_Reader(&m_Storage)
  {
  }

  ezMemoryStreamStorage m_Storage;
  ezMemoryStreamReader m_Reader;
};

ezResourceLoadData ezResourceLoaderFromFile::OpenDataStream(const ezResourceBase* pResource)
{
  ezResourceLoadData res;

  ezFileReader File;
  if (File.Open(pResource->GetResourceID().GetData()).Failed())
    return res;

  res.m_sResourceDescription = File.GetFilePathRelative().GetData();

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  ezFileStats stat;
  if (ezOSFile::GetFileStats(File.GetFilePathAbsolute(), stat).Succeeded())
  {
    res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
  }

#endif


  FileResourceLoadData* pData = EZ_DEFAULT_NEW(FileResourceLoadData);

  ezMemoryStreamWriter w(&pData->m_Storage);

  // write the absolute path to the read file into the memory stream
  w << File.GetFilePathAbsolute();

  ezUInt8 uiTemp[1024];

  while (true)
  {
    const ezUInt64 uiRead = File.ReadBytes(uiTemp, 1024);
    w.WriteBytes(uiTemp, uiRead);

    if (uiRead < 1024)
      break;
  }

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void ezResourceLoaderFromFile::CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData)
{
  FileResourceLoadData* pData = static_cast<FileResourceLoadData*>(LoaderData.m_pCustomLoaderData);

  EZ_DEFAULT_DELETE(pData);
}

bool ezResourceLoaderFromFile::IsResourceOutdated(const ezResourceBase* pResource) const
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





EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceTypeLoader);

