#include <Core/PCH.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>

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

  FileResourceLoadData* pData = EZ_DEFAULT_NEW(FileResourceLoadData);

  ezMemoryStreamWriter w(&pData->m_Storage);

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

