#include <Core/PCH.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/ResourceManager/ResourceBase.h>
#include <Core/ResourceManager/ResourceManager.h>

ezThreadLocalPointer<ezResourceHandleReadContext> ezResourceHandleReadContext::s_ActiveContext;

ezResourceHandleReadContext::ezResourceHandleReadContext()
{
}

ezResourceHandleReadContext::~ezResourceHandleReadContext()
{
}

void ezResourceHandleReadContext::ReadHandle(ezStreamReader* pStream, ezTypelessResourceHandle* pResourceHandle)
{
  ezResourceHandleReadContext* pContext = s_ActiveContext;
  EZ_ASSERT_DEBUG(pContext != nullptr, "No ezResourceHandleReadContext is active on this thread");

  pContext->ReadResourceReference(pStream, pResourceHandle);
}

void ezResourceHandleReadContext::ReadResourceReference(ezStreamReader* pStream, ezTypelessResourceHandle* pResourceHandle)
{
  auto& hd = m_StoredHandles.ExpandAndGetRef();
  hd.m_pHandle = pResourceHandle;
  *pStream >> hd.m_uiResourceID;
}

void ezResourceHandleReadContext::BeginRestoringHandles(ezStreamReader* pStream)
{
  EZ_ASSERT_DEV(s_ActiveContext == nullptr, "Instances of ezResourceHandleReadContext cannot be nested on the callstack");

  s_ActiveContext = this;

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
