#include <PCH.h>

#include <Core/ResourceManager/ResourceBase.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

static thread_local ezResourceHandleWriteContext* s_pActiveWriteContext = nullptr;

ezResourceHandleWriteContext::ezResourceHandleWriteContext()
{
  m_State = State::NotStarted;
}

ezResourceHandleWriteContext::~ezResourceHandleWriteContext()
{
  EZ_ASSERT_DEV(m_State == State::NotStarted || m_State == State::Finished, "ezResourceHandleWriteContext was not used correctly");
}

void ezResourceHandleWriteContext::WriteHandle(ezStreamWriter* pStream, const ezResourceBase* pResource)
{
  ezResourceHandleWriteContext* pContext = s_pActiveWriteContext;
  EZ_ASSERT_DEBUG(pContext != nullptr, "No ezResourceHandleWriteContext is active on this thread");

  pContext->WriteResourceReference(pStream, pResource);
}

void ezResourceHandleWriteContext::WriteResourceReference(ezStreamWriter* pStream, const ezResourceBase* pResource)
{
  EZ_ASSERT_DEV(m_State == State::Writing, "Resource handles can only be written between calls to "
                                           "ezResourceHandleWriteContext::BeginWritingToStream() and EndWritingToStream()");

  ezUInt32 uiValue = 0xFFFFFFFF;

  if (pResource != nullptr)
  {
    uiValue = m_ResourcesToStore.GetCount();
    if (!m_StoredHandles.TryGetValue(pResource, uiValue))
    {
      m_ResourcesToStore.PushBack(pResource);
      m_StoredHandles[pResource] = uiValue;
    }
  }

  *pStream << uiValue;
}

void ezResourceHandleWriteContext::BeginWritingToStream(ezStreamWriter* pStream)
{
  EZ_ASSERT_DEV(s_pActiveWriteContext == nullptr, "Instances of ezResourceHandleWriteContext cannot be nested on the callstack");
  EZ_ASSERT_DEV(m_State == State::NotStarted,
                "ezResourceHandleWriteContext::BeginWritingToStream cannot be called twice on the same instance");

  m_State = State::Writing;

  s_pActiveWriteContext = this;

  const ezUInt8 uiVersion = 1;
  *pStream << uiVersion;
}

void ezResourceHandleWriteContext::EndWritingToStream(ezStreamWriter* pStream)
{
  EZ_ASSERT_DEV(
      m_State == State::Writing,
      "ezResourceHandleWriteContext::EndWritingToStream must be called once after ezResourceHandleWriteContext::BeginWritingToStream");

  // number of all resources
  *pStream << m_ResourcesToStore.GetCount();

  ezMap<const ezRTTI*, ezDeque<ezUInt32>> SortedResourcesToStore;

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

  m_State = State::Finished;
  s_pActiveWriteContext = nullptr;
}



EZ_STATICLINK_FILE(Core, Core_WorldSerializer_Implementation_ResourceHandleWriter);
