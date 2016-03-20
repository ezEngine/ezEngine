#include <Core/PCH.h>
#include <Core/ResourceManager/HandleSerialization.h>
#include <Core/ResourceManager/ResourceBase.h>
#include <Core/ResourceManager/ResourceManager.h>

ezTypelessResourceHandle::ezTypelessResourceHandle(ezResourceBase* pResource)
{
  m_pResource = pResource;

  if (m_pResource)
    IncreaseResourceRefCount(m_pResource);
}

void ezTypelessResourceHandle::Invalidate()
{
  if (m_pResource)
    DecreaseResourceRefCount(m_pResource);

  m_pResource = nullptr;
}

ezUInt32 ezTypelessResourceHandle::GetResourceIDHash() const
{
  return m_pResource->GetResourceIDHash();
}

void ezTypelessResourceHandle::operator=(const ezTypelessResourceHandle& rhs)
{
  Invalidate();

  m_pResource = rhs.m_pResource;

  if (m_pResource)
    IncreaseResourceRefCount(reinterpret_cast<ezResourceBase*>(m_pResource));
}

void ezTypelessResourceHandle::operator=(ezTypelessResourceHandle&& rhs)
{
  Invalidate();

  m_pResource = rhs.m_pResource;
  rhs.m_pResource = nullptr;
}


ezThreadLocalPointer<ezResourceHandleWriteContext> ezResourceHandleWriteContext::s_ActiveContext;
ezThreadLocalPointer<ezResourceHandleReadContext> ezResourceHandleReadContext::s_ActiveContext;


ezResourceHandleWriteContext::ezResourceHandleWriteContext()
{
  m_pStream = nullptr;

  EZ_ASSERT_DEV(s_ActiveContext == nullptr, "Instances of ezResourceHandleWriteContext cannot be nested on the callstack");

  s_ActiveContext = this;
  m_bFinalized = false;

  //const ezUInt8 uiVersion = 1;
  //*m_pStream << uiVersion;
}

ezResourceHandleWriteContext::~ezResourceHandleWriteContext()
{
  EZ_ASSERT_DEV(m_bFinalized, "ezResourceHandleWriteContext::Finalize was not called");

  s_ActiveContext = nullptr;
}

void ezResourceHandleWriteContext::SetStream(ezStreamWriter* pStream)
{
  m_pStream = pStream;
}

void ezResourceHandleWriteContext::WriteHandle(const ezResourceBase* pResource)
{
  ezResourceHandleWriteContext* pContext = s_ActiveContext;
  EZ_ASSERT_DEBUG(pContext != nullptr, "No ezResourceHandleWriteContext is active on this thread");

  pContext->WriteResourceReference(pResource);
}

void ezResourceHandleWriteContext::WriteResourceReference(const ezResourceBase* pResource)
{
  ezUInt32 uiValue = m_ResourcesToStore.GetCount();
  if (!m_StoredHandles.TryGetValue(pResource, uiValue))
  {
    m_ResourcesToStore.PushBack(pResource);
    m_StoredHandles[pResource] = uiValue;
  }

  *m_pStream << uiValue;
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



ezResourceHandleReadContext::ezResourceHandleReadContext()
{
  m_pStream = nullptr;
}

ezResourceHandleReadContext::~ezResourceHandleReadContext()
{
}

void ezResourceHandleReadContext::ReadHandle(ezTypelessResourceHandle* pResourceHandle)
{
  ezResourceHandleReadContext* pContext = s_ActiveContext;
  EZ_ASSERT_DEBUG(pContext != nullptr, "No ezResourceHandleReadContext is active on this thread");

  pContext->ReadResourceReference(pResourceHandle);
}

void ezResourceHandleReadContext::ReadResourceReference(ezTypelessResourceHandle* pResourceHandle)
{
  auto& hd = m_StoredHandles.ExpandAndGetRef();
  hd.m_pHandle = pResourceHandle;
  *m_pStream >> hd.m_uiResourceID;
}

void ezResourceHandleReadContext::BeginRestoringHandles(ezStreamReader* pStream)
{
  EZ_ASSERT_DEV(s_ActiveContext == nullptr, "Instances of ezResourceHandleReadContext cannot be nested on the callstack");

  s_ActiveContext = this;
  m_pStream = pStream;

  m_StoredHandles.Clear();
}

void ezResourceHandleReadContext::EndRestoringHandles()
{
  for (const auto& hd : m_StoredHandles)
  {
    *hd.m_pHandle = m_AllResources[hd.m_uiResourceID];
  }

  m_StoredHandles.Clear();

  s_ActiveContext = nullptr;
}

void ezResourceHandleReadContext::ReadFinalizedData(ezStreamReader* pStream)
{
  // number of all resources
  ezUInt32 uiAllResourcesCount = 0;
  *pStream >> uiAllResourcesCount;

  m_AllResources.SetCount(uiAllResourcesCount);

  ezUInt32 uiNumTypes = 0;
  *pStream >> uiNumTypes;

  // for each type
  {
    ezStringBuilder sTemp;

    for (ezUInt32 type = 0; type < uiNumTypes; ++type)
    {
      *pStream >> sTemp;
      const ezRTTI* pRtti = ezRTTI::FindTypeByName(sTemp);

      EZ_ASSERT_DEV(pRtti != nullptr, "Unknown resource type '%s'", sTemp.GetData());

      // number of resources of this type
      ezUInt32 uiNumResourcesOfType = 0;
      *pStream >> uiNumResourcesOfType;

      for (ezUInt32 i = 0; i < uiNumResourcesOfType; ++i)
      {
        ezUInt32 uiInternalID = 0;
        *pStream >> uiInternalID;

        // read unique ID for restoring the resource (from file)
        *pStream >> sTemp;

        // load the resource of the given type
        const ezTypelessResourceHandle hResource(ezResourceManager::GetResource(pRtti, sTemp, true));

        m_AllResources[uiInternalID] = hResource;
      }
    }
  }
}
