#pragma once

#include <Core/Basics.h>
#include <Foundation/Threading/ThreadLocalPointer.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezResourceBase;

class EZ_CORE_DLL ezResourceHandleWriteContext
{
public:
  ezResourceHandleWriteContext();
  ~ezResourceHandleWriteContext();

  void SetStream(ezStreamWriter* pStream);

  template<typename ResourceType>
  static void WriteHandle(const ezTypedResourceHandle<ResourceType>& hResource)
  {
    WriteHandle(hResource.m_Typeless.m_pResource);
  }

  static void WriteHandle(const ezResourceBase* pResource);

  void WriteResourceReference(const ezResourceBase* pResource);

  void Finalize(ezStreamWriter* pStream);

private:
  static ezThreadLocalPointer<ezResourceHandleWriteContext> s_ActiveContext;

  ezHashTable<const ezResourceBase*, ezUInt32> m_StoredHandles;
  ezDeque<const ezResourceBase*> m_ResourcesToStore;

  bool m_bFinalized;
  ezStreamWriter* m_pStream;
};

class EZ_CORE_DLL ezResourceHandleReadContext
{
public:
  ezResourceHandleReadContext();
  ~ezResourceHandleReadContext();


  template<typename ResourceType>
  static void ReadHandle(ezTypedResourceHandle<ResourceType>* pResourceHandle)
  {
    ReadHandle(&pResourceHandle->m_Typeless);
  }

  static void ReadHandle(ezTypelessResourceHandle* pResourceHandle);

  void ReadResourceReference(ezTypelessResourceHandle* pResourceHandle);

  void BeginRestoringHandles(ezStreamReader* pStream);

  void EndRestoringHandles();

  void ReadFinalizedData(ezStreamReader* pStream);
private:
  static ezThreadLocalPointer<ezResourceHandleReadContext> s_ActiveContext;

  struct HandleData
  {
    EZ_DECLARE_POD_TYPE();

    ezTypelessResourceHandle* m_pHandle;
    ezUInt32 m_uiResourceID;
  };

  ezDeque<HandleData> m_StoredHandles;
  ezDynamicArray<ezTypelessResourceHandle> m_AllResources;

  ezStreamReader* m_pStream;
};


