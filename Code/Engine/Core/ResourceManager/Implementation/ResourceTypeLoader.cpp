#include <PCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>

struct FileResourceLoadData
{
  FileResourceLoadData()
      : m_Reader(&m_Storage)
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
  // if we cannot find the target file, there is no point in trying to reload it -> claim it's up to date
  ezStringBuilder sAbs;
  if (ezFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)

  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    ezFileStats stat;
    if (ezOSFile::GetFileStats(sAbs, stat).Failed())
      return false;

    if (!stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), ezTimestamp::CompareMode::FileTimeEqual))
      return true;

    return false;
  }

#endif

  return true;
}

//////////////////////////////////////////////////////////////////////////

ezResourceLoadData ezResourceLoaderFromMemory::OpenDataStream(const ezResourceBase* pResource)
{
  m_Reader.SetStorage(&m_CustomData);
  m_Reader.SetReadPosition(0);

  ezResourceLoadData res;

  res.m_sResourceDescription = m_sResourceDescription;
  res.m_LoadedFileModificationDate = m_ModificationTimestamp;
  res.m_pDataStream = &m_Reader;
  res.m_pCustomLoaderData = nullptr;

  return res;
}

void ezResourceLoaderFromMemory::CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData)
{
  m_Reader.SetStorage(nullptr);
}

bool ezResourceLoaderFromMemory::IsResourceOutdated(const ezResourceBase* pResource) const
{
  if (pResource->GetLoadedFileModificationTime().IsValid() && m_ModificationTimestamp.IsValid())
  {
    if (!m_ModificationTimestamp.Compare(pResource->GetLoadedFileModificationTime(), ezTimestamp::CompareMode::FileTimeEqual))
      return true;

    return false;
  }

  return true;
}



EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceTypeLoader);
