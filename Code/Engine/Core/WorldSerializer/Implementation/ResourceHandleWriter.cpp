#include <Core/PCH.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <Core/ResourceManager/ResourceBase.h>

ezThreadLocalPointer<ezResourceHandleWriteContext> ezResourceHandleWriteContext::s_ActiveContext;


ezResourceHandleWriteContext::ezResourceHandleWriteContext()
{
  EZ_ASSERT_DEV(s_ActiveContext == nullptr, "Instances of ezResourceHandleWriteContext cannot be nested on the callstack");

  s_ActiveContext = this;
  m_bFinalized = false;
}

ezResourceHandleWriteContext::~ezResourceHandleWriteContext()
{
  EZ_ASSERT_DEV(m_bFinalized, "ezResourceHandleWriteContext::Finalize was not called");

  s_ActiveContext = nullptr;
}

void ezResourceHandleWriteContext::WriteHandle(ezStreamWriter* pStream, const ezResourceBase* pResource)
{
  ezResourceHandleWriteContext* pContext = s_ActiveContext;
  EZ_ASSERT_DEBUG(pContext != nullptr, "No ezResourceHandleWriteContext is active on this thread");

  pContext->WriteResourceReference(pStream, pResource);
}

void ezResourceHandleWriteContext::WriteResourceReference(ezStreamWriter* pStream, const ezResourceBase* pResource)
{
  ezUInt32 uiValue = m_ResourcesToStore.GetCount();
  if (!m_StoredHandles.TryGetValue(pResource, uiValue))
  {
    m_ResourcesToStore.PushBack(pResource);
    m_StoredHandles[pResource] = uiValue;
  }

  *pStream << uiValue;
}

void ezResourceHandleWriteContext::Finalize(ezStreamWriter* pStream)
{
  EZ_ASSERT_DEV(!m_bFinalized, "ezResourceHandleWriteContext::Finalize was called twice");
  m_bFinalized = true;

  // number of all resources
  *pStream << m_ResourcesToStore.GetCount();

  ezMap<const ezRTTI*, ezDeque<ezUInt32> > SortedResourcesToStore;

  // sort all resources by type
  for (ezUInt32 id = 0; id < m_ResourcesToStore.GetCount(); ++id)
  {
    SortedResourcesToStore[m_ResourcesToStore[id]->GetDynamicRTTI()].PushBack(id);
  }

  // number of types
  *pStream << SortedResourcesToStore.GetCount();

  // for each type
  {
    for (auto itType = SortedResourcesToStore.GetIterator(); itType.IsValid(); ++itType)
    {
      // write type name
      *pStream << itType.Key()->GetTypeName();

      const auto& IDs = itType.Value();

      // number of resources of this type
      *pStream << IDs.GetCount();

      for (ezUInt32 id : IDs)
      {
        // write internal ID
        *pStream << id;

        // write unique ID for restoring the resource (from file)
        *pStream << m_ResourcesToStore[id]->GetResourceID();
      }
    }
  }
}

