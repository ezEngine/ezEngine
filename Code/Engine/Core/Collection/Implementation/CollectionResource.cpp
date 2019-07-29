#include <CorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Collection/CollectionResource.h>
#include <Foundation/Profiling/Profiling.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollectionResource, 1, ezRTTIDefaultAllocator<ezCollectionResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezCollectionResource);

ezCollectionResource::ezCollectionResource()
    : ezResource(DoUpdate::OnAnyThread, 1)
{
}

void ezCollectionResource::PreloadResources()
{
  EZ_LOCK(m_preloadMutex);
  EZ_PROFILE_SCOPE("Inject Resources to Preload");

  m_hPreloadedResources.Clear();
  m_hPreloadedResources.Reserve(m_Collection.m_Resources.GetCount());

  for (const auto& e : m_Collection.m_Resources)
  {
    ezTypelessResourceHandle hTypeless;

    if (!e.m_sAssetTypeName.IsEmpty())
    {
      if (const ezRTTI* pRtti = ezResourceManager::FindResourceForAssetType(e.m_sAssetTypeName))
      {
        hTypeless = ezResourceManager::LoadResourceByType(pRtti, e.m_sResourceID);
      }
    }

    m_hPreloadedResources.PushBack(hTypeless);

    if (hTypeless.IsValid())
    {
      ezResourceManager::PreloadResource(hTypeless);
    }
  }
}

float ezCollectionResource::GetPreloadProgress() const
{
  EZ_LOCK(m_preloadMutex);

  EZ_ASSERT_DEBUG(m_hPreloadedResources.GetCount() == m_Collection.m_Resources.GetCount(), "Collection size mismatch. PreloadResources not called?");

  double loadedWeight = 0;
  double totalWeight = 0;

  for (ezUInt32 i = 0; i < m_hPreloadedResources.GetCount(); i++)
  {
    const ezTypelessResourceHandle& hResource = m_hPreloadedResources[i];
    if (!hResource.IsValid())
      continue;

    const ezCollectionEntry& entry = m_Collection.m_Resources[i];
    double thisWeight = static_cast<double>(ezMath::Max(entry.m_uiFileSize, 1ull)); // if file sizes are not specified, we weight by 1
    ezResourceState state = ezResourceManager::GetLoadingState(hResource);

    if (state == ezResourceState::Loaded || state == ezResourceState::LoadedResourceMissing)
    {
      loadedWeight += thisWeight;
    }

    if (state != ezResourceState::Invalid)
    {
      totalWeight += thisWeight;
    }
  }

  if (totalWeight == 0)
    return 1.0f;

  return static_cast<float>(loadedWeight / totalWeight);
}


const ezCollectionResourceDescriptor& ezCollectionResource::GetDescriptor() const
{
  return m_Collection;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezCollectionResource, ezCollectionResourceDescriptor)
{
  m_Collection = descriptor;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

ezResourceLoadDesc ezCollectionResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  {
    UnregisterNames();

    EZ_LOCK(m_preloadMutex);
    m_hPreloadedResources.Clear();
    m_Collection.m_Resources.Clear();

    m_hPreloadedResources.Compact();
    m_Collection.m_Resources.Compact();
  }

  return res;
}

ezResourceLoadDesc ezCollectionResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezCollectionResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  m_Collection.Load(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezCollectionResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  EZ_LOCK(m_preloadMutex);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU =
      static_cast<ezUInt32>(m_hPreloadedResources.GetHeapMemoryUsage() + m_Collection.m_Resources.GetHeapMemoryUsage());
}


void ezCollectionResource::RegisterNames()
{
  if (m_bRegistered)
    return;

  m_bRegistered = true;

  EZ_LOCK(ezResourceManager::GetMutex());

  for (const auto& entry : m_Collection.m_Resources)
  {
    if (!entry.m_sOptionalNiceLookupName.IsEmpty())
    {
      ezResourceManager::RegisterNamedResource(entry.m_sOptionalNiceLookupName, entry.m_sResourceID);
    }
  }
}


void ezCollectionResource::UnregisterNames()
{
  if (!m_bRegistered)
    return;

  m_bRegistered = false;

  EZ_LOCK(ezResourceManager::GetMutex());

  for (const auto& entry : m_Collection.m_Resources)
  {
    if (!entry.m_sOptionalNiceLookupName.IsEmpty())
    {
      ezResourceManager::UnregisterNamedResource(entry.m_sOptionalNiceLookupName);
    }
  }
}

void ezCollectionResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 3;
  const ezUInt8 uiIdentifier = 0xC0;
  const ezUInt32 uiNumResources = m_Resources.GetCount();

  stream << uiVersion;
  stream << uiIdentifier;
  stream << uiNumResources;

  for (ezUInt32 i = 0; i < uiNumResources; ++i)
  {
    stream << m_Resources[i].m_sAssetTypeName;
    stream << m_Resources[i].m_sOptionalNiceLookupName;
    stream << m_Resources[i].m_sResourceID;
    stream << m_Resources[i].m_uiFileSize;
  }
}

void ezCollectionResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  ezUInt8 uiIdentifier = 0;
  ezUInt32 uiNumResources = 0;

  stream >> uiVersion;
  stream >> uiIdentifier;
  
  if (uiVersion == 1)
  {
    ezUInt16 uiNumResourcesShort;
    stream >> uiNumResourcesShort;
    uiNumResources = uiNumResourcesShort;
  }
  else
  {
    stream >> uiNumResources;
  }

  EZ_ASSERT_DEV(uiIdentifier == 0xC0, "File does not contain a valid ezCollectionResourceDescriptor");
  EZ_ASSERT_DEV(uiVersion > 0 && uiVersion <= 3, "Invalid file version {0}", uiVersion);

  m_Resources.SetCount(uiNumResources);

  for (ezUInt32 i = 0; i < uiNumResources; ++i)
  {
    stream >> m_Resources[i].m_sAssetTypeName;
    stream >> m_Resources[i].m_sOptionalNiceLookupName;
    stream >> m_Resources[i].m_sResourceID;
    if (uiVersion >= 3)
    {
      stream >> m_Resources[i].m_uiFileSize;
    }
  }
}



EZ_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionResource);

