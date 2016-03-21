#pragma once

#include <Core/Basics.h>
#include <Foundation/Threading/ThreadLocalPointer.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Deque.h>

class ezResourceBase;
class ezStreamWriter;

class EZ_CORE_DLL ezResourceHandleWriteContext
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezResourceHandleWriteContext);

public:
  ezResourceHandleWriteContext();
  ~ezResourceHandleWriteContext();

  template<typename ResourceType>
  static void WriteHandle(ezStreamWriter* pStream, const ezTypedResourceHandle<ResourceType>& hResource)
  {
    WriteHandle(pStream, hResource.m_Typeless.m_pResource);
  }

  static void WriteHandle(ezStreamWriter* pStream, const ezResourceBase* pResource);

  void WriteResourceReference(ezStreamWriter* pStream, const ezResourceBase* pResource);

  void Finalize(ezStreamWriter* pStream);

private:
  static ezThreadLocalPointer<ezResourceHandleWriteContext> s_ActiveContext;

  ezHashTable<const ezResourceBase*, ezUInt32> m_StoredHandles;
  ezDeque<const ezResourceBase*> m_ResourcesToStore;

  bool m_bFinalized;
};

/// \brief Operator to serialize resource handles
template<typename ResourceType>
void operator<< (ezStreamWriter& Stream, ezTypedResourceHandle<ResourceType> Value)
{
  ezResourceHandleWriteContext::WriteHandle(&Stream, Value);
}

