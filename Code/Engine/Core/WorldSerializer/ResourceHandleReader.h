#pragma once

#include <Core/Basics.h>
#include <Foundation/Threading/ThreadLocalPointer.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>

class ezResourceBase;
class ezStreamReader;

class EZ_CORE_DLL ezResourceHandleReadContext
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezResourceHandleReadContext);

public:
  ezResourceHandleReadContext();
  ~ezResourceHandleReadContext();


  template<typename ResourceType>
  static void ReadHandle(ezStreamReader* pStream, ezTypedResourceHandle<ResourceType>& ResourceHandle)
  {
    ReadHandle(pStream, &ResourceHandle.m_Typeless);
  }

  static void ReadHandle(ezStreamReader* pStream, ezTypelessResourceHandle* pResourceHandle);

  void ReadResourceReference(ezStreamReader* pStream, ezTypelessResourceHandle* pResourceHandle);

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
};


/// \brief Operator to serialize resource handles
template<typename ResourceType>
void operator>> (ezStreamReader& Stream, ezTypedResourceHandle<ResourceType>& Value)
{
  ezResourceHandleReadContext::ReadHandle(&Stream, Value);
}