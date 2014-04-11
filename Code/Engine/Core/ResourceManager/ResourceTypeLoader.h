#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Foundation/IO/Stream.h>

struct ezResourceLoadData
{
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
};

