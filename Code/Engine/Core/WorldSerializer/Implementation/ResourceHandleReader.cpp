#include <CorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>

static thread_local ezResourceHandleReadContext* s_pActiveReadContext = nullptr;

ezResourceHandleReadContext::ezResourceHandleReadContext()
{
  Reset();
}

ezResourceHandleReadContext::~ezResourceHandleReadContext()
{
  EZ_ASSERT_DEV(s_pActiveReadContext != this, "ezResourceHandleReadContext::EndRestoringHandles() was not called");
}

void ezResourceHandleReadContext::Reset()
{
  m_uiVersion = 0;
  m_bReadData = false;
  m_StoredHandles.Clear();
  m_AllResources.Clear();
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
  EZ_ASSERT_DEV(s_pActiveReadContext == nullptr, "Instances of ezResourceHandleReadContext cannot be nested on the callstack");
  s_pActiveReadContext = this;

  EZ_ASSERT_DEV(
    m_uiVersion != 0,
    "ezResourceHandleReadContext::BeginRestoringHandles must be called after ezResourceHandleReadContext::BeginReadingFromStream");

  m_StoredHandles.Clear();
}

void ezResourceHandleReadContext::EndRestoringHandles()
{
  EZ_ASSERT_DEV(s_pActiveReadContext == this,
    "Incorrect usage of ezResourceHandleReadContext::BeginRestoringHandles / EndRestoringHandles");
  EZ_ASSERT_DEV(m_bReadData,
    "ezResourceHandleReadContext::EndRestoringHandles must be called AFTER ezResourceHandleReadContext::EndReadingFromStream");

  for (const auto& hd : m_StoredHandles)
  {
    *hd.m_pHandle = m_AllResources[hd.m_uiResourceID];
  }

  m_StoredHandles.Clear();

  s_pActiveReadContext = nullptr;
}

ezResult ezResourceHandleReadContext::BeginReadingFromStream(ezStreamReader* pStream)
{
  EZ_ASSERT_DEV(m_uiVersion == 0, "ezResourceHandleReadContext::BeginReadingFromStream cannot be called twice on the same instance");

  //ezLog::Warning("File uses deprecated ezResourceHandleReadContext, please re-export.");

  *pStream >> m_uiVersion;
  if(m_uiVersion == 1)
  {
    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Error("Invalid version {0} of ezResourceHandleReadContext", m_uiVersion);
    return EZ_FAILURE;
  }
}

ezResult ezResourceHandleReadContext::EndReadingFromStream(ezStreamReader* pStream)
{
  EZ_ASSERT_DEV(!m_bReadData, "ezResourceHandleReadContext::EndReadingFromStream cannot be called twice on the same instance");
  m_bReadData = true;

  // number of all resources
  ezUInt32 uiAllResourcesCount = 0;
  *pStream >> uiAllResourcesCount;

  m_AllResources.SetCount(uiAllResourcesCount);

  ezUInt32 uiNumTypes = 0;
  *pStream >> uiNumTypes;

  if (uiNumTypes > 16 * 1024)
  {
    ezLog::Error("Unreasonable amount of types in resource handle context, got {0}", uiNumTypes);
    return EZ_FAILURE;
  }

  // for each type
  {
    ezStringBuilder sTemp;

    for (ezUInt32 type = 0; type < uiNumTypes; ++type)
    {
      *pStream >> sTemp;
      const ezRTTI* pRtti = ezRTTI::FindTypeByName(sTemp);

      if (pRtti == nullptr)
      {
        ezLog::Error("Unknown resource type '{0}'", sTemp);
      }

      // number of resources of this type
      ezUInt32 uiNumResourcesOfType = 0;
      *pStream >> uiNumResourcesOfType;

      for (ezUInt32 i = 0; i < uiNumResourcesOfType; ++i)
      {
        ezUInt32 uiInternalID = 0;
        *pStream >> uiInternalID;

        if (uiInternalID < uiAllResourcesCount)
        {
          // read unique ID for restoring the resource (from file)
          *pStream >> sTemp;

          if (pRtti != nullptr)
          {
            // load the resource of the given type
            const ezTypelessResourceHandle hResource(ezResourceManager::LoadResourceByType(pRtti, sTemp));

            m_AllResources[uiInternalID] = hResource;
          }
          else
          {
            // store an invalid handle, hope this doesn't break somewhere later
            m_AllResources[uiInternalID].Invalidate();
          }
        }
        else
        {
          ezLog::Error("Resource id out of range ({0}, {1})", uiInternalID, uiAllResourcesCount);
          return EZ_FAILURE;
        }
      }
    }
  }

  return EZ_FAILURE;
}

// TODO: move to ResourceHandleStreamOperations.cpp once ezResourceHandleReadContext is not used anymore
// static
void ezResourceHandleStreamOperations::ReadHandle(ezStreamReader& Stream, ezTypelessResourceHandle& ResourceHandle)
{
  if (ezResourceHandleReadContext* pContext = s_pActiveReadContext)
  {
    pContext->ReadResourceReference(&Stream, &ResourceHandle);
  }
  else
  {
    ezStringBuilder sTemp;

    Stream >> sTemp;
    if (sTemp.IsEmpty())
    {
      ResourceHandle.Invalidate();
      return;
    }

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sTemp);
    if (pRtti == nullptr)
    {
      ezLog::Error("Unknown resource type '{0}'", sTemp);
      ResourceHandle.Invalidate();
    }

    // read unique ID for restoring the resource (from file)
    Stream >> sTemp;

    if (pRtti != nullptr)
    {
      ResourceHandle = ezResourceManager::LoadResourceByType(pRtti, sTemp);
    }
  }
}

EZ_STATICLINK_FILE(Core, Core_WorldSerializer_Implementation_ResourceHandleReader);
