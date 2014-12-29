#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Timestamp.h>

struct ezResourceLoadData
{
  ezResourceLoadData()
  {
    m_pDataStream = nullptr;
    m_pCustomLoaderData = nullptr;
  }

  ezTimestamp m_LoadedFileModificationDate;
  ezStreamReaderBase* m_pDataStream;
  void* m_pCustomLoaderData;
};

class ezResourceTypeLoader
{
public:
  ezResourceTypeLoader() { }
  virtual ~ezResourceTypeLoader () { }

  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) = 0;
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) = 0;

  virtual bool IsResourceOutdated(const ezResourceBase* pResource) const { return false; }
};

class ezResourceLoaderFromFile : public ezResourceTypeLoader
{
public:

  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) override;
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) override;

  virtual bool IsResourceOutdated(const ezResourceBase* pResource) const override;
};

