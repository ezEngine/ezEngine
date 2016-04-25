#include <Core/PCH.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/ResourceManager/ResourceManager.h>

ezThreadLocalPointer<ezResourceHandleReadContext> ezResourceHandleReadContext::s_ActiveContext;

ezResourceHandleReadContext::ezResourceHandleReadContext()
{
  Reset();
}

ezResourceHandleReadContext::~ezResourceHandleReadContext()
{
  EZ_ASSERT_DEV(s_ActiveContext != this, "ezResourceHandleReadContext::EndRestoringHandles() was not called");
}

void ezResourceHandleReadContext::Reset()
{
  m_uiVersion = 0;
  m_bReadData = false;
  m_StoredHandles.Clear();
  m_AllResources.Clear();
}


void ezResourceHandleReadContext::ReadHandle(ezStreamReader* pStream, ezTypelessResourceHandle* pResourceHandle)
{
  ezResourceHandleReadContext* pContext = s_ActiveContext;
  EZ_ASSERT_DEBUG(pContext != nullptr, "No ezResourceHandleReadContext is active on this thread");

  pContext->ReadResourceReference(pStream, pResourceHandle);
}

void ezResourceHandleReadContext::ReadResourceReference(ezStreamReader* pStream, ezTypelessResourceHandle* pResourceHandle)
{
  ezUInt32 uiID = 0;
  *pStream >> uiID;

  if (uiID != 0xFFFFFFFF)
  {
    auto& hd = m_StoredHandles.ExpandAndGetRef();
    hd.m_pHandle = pResourceHandle;
    hd.m_uiResourceID = uiID;
  }
  else
  {
    pResourceHandle->Invalidate();
  }
}

void ezResourceHandleReadContext::BeginRestoringHandles(ezStreamReader* pStream)
{
  EZ_ASSERT_DEV(s_ActiveContext == nullptr, "Instances of ezResourceHandleReadContext cannot be nested on the callstack");
  s_ActiveContext = this;

  EZ_ASSERT_DEV(m_uiVersion != 0, "ezResourceHandleReadContext::BeginRestoringHandles must be called after ezResourceHandleReadContext::BeginReadingFromStream");

  m_StoredHandles.Clear();
}

void ezResourceHandleReadContext::EndRestoringHandles()
{
  EZ_ASSERT_DEV(s_ActiveContext == this, "Incorrect usage of ezResourceHandleReadContext::BeginRestoringHandles / EndRestoringHandles");
  EZ_ASSERT_DEV(m_bReadData, "ezResourceHandleReadContext::EndRestoringHandles must be called AFTER ezResourceHandleReadContext::EndReadingFromStream");

  for (const auto& hd : m_StoredHandles)
  {
    *hd.m_pHandle = m_AllResources[hd.m_uiResourceID];
  }

  m_StoredHandles.Clear();

  s_ActiveContext = nullptr;
}

void ezResourceHandleReadContext::BeginReadingFromStream(ezStreamReader* pStream)
{
  EZ_ASSERT_DEV(m_uiVersion == 0, "ezResourceHandleReadContext::BeginReadingFromStream cannot be called twice on the same instance");

  *pStream >> m_uiVersion;
  EZ_ASSERT_DEV(m_uiVersion == 1, "Invalid version %u of ezResourceHandleReadContext", m_uiVersion);
}

void ezResourceHandleReadContext::EndReadingFromStream(ezStreamReader* pStream)
{
  EZ_ASSERT_DEV(!m_bReadData, "ezResourceHandleReadContext::EndReadingFromStream cannot be called twice on the same instance");
  m_bReadData = true;

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
