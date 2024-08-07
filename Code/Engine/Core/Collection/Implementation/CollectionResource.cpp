#include <Core/CorePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollectionResource, 1, ezRTTIDefaultAllocator<ezCollectionResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezCollectionResource);

ezCollectionResource::ezCollectionResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

// UnloadData() already makes sure to call UnregisterNames();
ezCollectionResource::~ezCollectionResource() = default;

bool ezCollectionResource::PreloadResources(ezUInt32 uiNumResourcesToPreload)
{
  EZ_LOCK(m_PreloadMutex);
  EZ_PROFILE_SCOPE("Inject Resources to Preload");

  if (m_PreloadedResources.GetCount() == m_Collection.m_Resources.GetCount())
  {
    // All resources have already been queued so there is no need
    // to redo the work. Clearing the array would in fact potentially
    // trigger one of the resources to be unloaded, undoing the work
    // that was already done to preload the collection.
    return false;
  }

  m_PreloadedResources.Reserve(m_Collection.m_Resources.GetCount());

  const ezUInt32 remainingResources = m_Collection.m_Resources.GetCount() - m_PreloadedResources.GetCount();
  const ezUInt32 end = ezMath::Min(remainingResources, uiNumResourcesToPreload) + m_PreloadedResources.GetCount();
  for (ezUInt32 i = m_PreloadedResources.GetCount(); i < end; ++i)
  {
    const ezCollectionEntry& e = m_Collection.m_Resources[i];
    ezTypelessResourceHandle hTypeless;

    if (!e.m_sAssetTypeName.IsEmpty())
    {
      if (const ezRTTI* pRtti = ezResourceManager::FindResourceForAssetType(e.m_sAssetTypeName))
      {
        hTypeless = ezResourceManager::LoadResourceByType(pRtti, e.m_sResourceID);
      }
      else
      {
        ezLog::Warning("There was no valid RTTI available for assets with type name '{}'. Could not pre-load resource '{}'. Did you forget to register the resource type with the ezResourceManager?", e.m_sAssetTypeName, ezArgSensitive(e.m_sResourceID, "ResourceID"));
      }
    }
    else
    {
      ezLog::Error("Asset '{}' had an empty asset type name. Cannot pre-load it.", ezArgSensitive(e.m_sResourceID, "ResourceID"));
    }

    m_PreloadedResources.PushBack(hTypeless);

    if (hTypeless.IsValid())
    {
      ezResourceManager::PreloadResource(hTypeless);
    }
  }

  return m_PreloadedResources.GetCount() < m_Collection.m_Resources.GetCount();
}

bool ezCollectionResource::IsLoadingFinished(float* out_pProgress) const
{
  EZ_LOCK(m_PreloadMutex);

  ezUInt64 loadedWeight = 0;
  ezUInt64 totalWeight = 0;

  for (ezUInt32 i = 0; i < m_PreloadedResources.GetCount(); i++)
  {
    const ezTypelessResourceHandle& hResource = m_PreloadedResources[i];
    if (!hResource.IsValid())
      continue;

    const ezCollectionEntry& entry = m_Collection.m_Resources[i];
    ezUInt64 thisWeight = ezMath::Max(entry.m_uiFileSize, 1ull); // if file sizes are not specified, we weight by 1
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

  if (out_pProgress != nullptr)
  {
    const float maxLoadedFraction = m_Collection.m_Resources.GetCount() == 0 ? 1.f : (float)m_PreloadedResources.GetCount() / m_Collection.m_Resources.GetCount();
    if (totalWeight != 0 && totalWeight != loadedWeight)
    {
      *out_pProgress = static_cast<float>(static_cast<double>(loadedWeight) / totalWeight) * maxLoadedFraction;
    }
    else
    {
      *out_pProgress = maxLoadedFraction;
    }
  }

  if (totalWeight == 0 || totalWeight == loadedWeight)
  {
    return true;
  }

  return false;
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
  EZ_IGNORE_UNUSED(WhatToUnload);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  {
    UnregisterNames();
    // This lock unnecessary as this function is only called when the reference count is 0, i.e. if we deallocate this.
    // It is intentionally removed as it caused this lock and the resource manager lock to be locked in reverse order.
    // To prevent potential deadlocks and be able to sanity check our locking the entire codebase should never lock any
    // locks in reverse order, even if this lock is probably fine it prevents us from reasoning over the entire system.
    // EZ_LOCK(m_preloadMutex);
    m_PreloadedResources.Clear();
    m_Collection.m_Resources.Clear();

    m_PreloadedResources.Compact();
    m_Collection.m_Resources.Compact();
  }

  return res;
}

ezResourceLoadDesc ezCollectionResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezCollectionResource::UpdateContent", GetResourceIdOrDescription());

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
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Collection.Load(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezCollectionResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  EZ_LOCK(m_PreloadMutex);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<ezUInt32>(m_PreloadedResources.GetHeapMemoryUsage() + m_Collection.m_Resources.GetHeapMemoryUsage());
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

void ezCollectionResourceDescriptor::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 3;
  const ezUInt8 uiIdentifier = 0xC0;
  const ezUInt32 uiNumResources = m_Resources.GetCount();

  inout_stream << uiVersion;
  inout_stream << uiIdentifier;
  inout_stream << uiNumResources;

  for (ezUInt32 i = 0; i < uiNumResources; ++i)
  {
    inout_stream << m_Resources[i].m_sAssetTypeName;
    inout_stream << m_Resources[i].m_sOptionalNiceLookupName;
    inout_stream << m_Resources[i].m_sResourceID;
    inout_stream << m_Resources[i].m_uiFileSize;
  }
}

void ezCollectionResourceDescriptor::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  ezUInt8 uiIdentifier = 0;
  ezUInt32 uiNumResources = 0;

  inout_stream >> uiVersion;
  inout_stream >> uiIdentifier;

  if (uiVersion == 1)
  {
    ezUInt16 uiNumResourcesShort;
    inout_stream >> uiNumResourcesShort;
    uiNumResources = uiNumResourcesShort;
  }
  else
  {
    inout_stream >> uiNumResources;
  }

  EZ_ASSERT_DEV(uiIdentifier == 0xC0, "File does not contain a valid ezCollectionResourceDescriptor");
  EZ_ASSERT_DEV(uiVersion > 0 && uiVersion <= 3, "Invalid file version {0}", uiVersion);

  m_Resources.SetCount(uiNumResources);

  for (ezUInt32 i = 0; i < uiNumResources; ++i)
  {
    inout_stream >> m_Resources[i].m_sAssetTypeName;
    inout_stream >> m_Resources[i].m_sOptionalNiceLookupName;
    inout_stream >> m_Resources[i].m_sResourceID;
    if (uiVersion >= 3)
    {
      inout_stream >> m_Resources[i].m_uiFileSize;
    }
  }
}



EZ_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionResource);
