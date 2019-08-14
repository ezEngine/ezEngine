#include <CorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>

struct FileResourceLoadData
{
  ezBlob m_Storage;
  ezRawMemoryStreamReader m_Reader;
};

ezResourceLoadData ezResourceLoaderFromFile::OpenDataStream(const ezResource* pResource)
{
  EZ_PROFILE_SCOPE("ReadResourceFile");

  ezResourceLoadData res;

  ezFileReader File;
  if (File.Open(pResource->GetResourceID().GetData()).Failed())
    return res;

  res.m_sResourceDescription = File.GetFilePathRelative().GetData();

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  ezFileStats stat;
  if (ezFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
  {
    res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
  }

#endif

  FileResourceLoadData* pData = EZ_DEFAULT_NEW(FileResourceLoadData);

  const ezUInt64 uiFileSize = File.GetFileSize();

  const ezUInt64 uiBlobCapacity = uiFileSize + File.GetFilePathAbsolute().GetElementCount() + 8; // +8 for the string overhead
  pData->m_Storage.SetCountUninitialized(uiBlobCapacity);

  ezUInt8* pBlobPtr = pData->m_Storage.GetBlobPtr<ezUInt8>().GetPtr();

  ezRawMemoryStreamWriter w(pBlobPtr, uiBlobCapacity);

  // write the absolute path to the read file into the memory stream
  w << File.GetFilePathAbsolute();

  const ezUInt64 uiOffset = w.GetNumWrittenBytes();

  File.ReadBytes(pBlobPtr + uiOffset, uiFileSize);

  pData->m_Reader.Reset(pBlobPtr, w.GetNumWrittenBytes() + uiFileSize);
  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void ezResourceLoaderFromFile::CloseDataStream(const ezResource* pResource, const ezResourceLoadData& LoaderData)
{
  FileResourceLoadData* pData = static_cast<FileResourceLoadData*>(LoaderData.m_pCustomLoaderData);

  EZ_DEFAULT_DELETE(pData);
}

bool ezResourceLoaderFromFile::IsResourceOutdated(const ezResource* pResource) const
{
  // if we cannot find the target file, there is no point in trying to reload it -> claim it's up to date
  ezStringBuilder sAbs;
  if (ezFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)

  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    ezFileStats stat;
    if (ezFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), ezTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;
}

//////////////////////////////////////////////////////////////////////////

ezResourceLoadData ezResourceLoaderFromMemory::OpenDataStream(const ezResource* pResource)
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

void ezResourceLoaderFromMemory::CloseDataStream(const ezResource* pResource, const ezResourceLoadData& LoaderData)
{
  m_Reader.SetStorage(nullptr);
}

bool ezResourceLoaderFromMemory::IsResourceOutdated(const ezResource* pResource) const
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
