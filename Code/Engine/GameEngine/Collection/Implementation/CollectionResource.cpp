#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <GameEngine/Collection/CollectionResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollectionResource, 1, ezRTTIDefaultAllocator<ezCollectionResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


ezCollectionResource::ezCollectionResource()
    : ezResource<ezCollectionResource, ezCollectionResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
  m_bRegistered = false;
}

void ezCollectionResource::PreloadResources(ezTime tShouldBeAvailableIn)
{
  m_hPreloadedResources.Clear();
  m_hPreloadedResources.Reserve(m_Collection.m_Resources.GetCount());

  for (const auto& e : m_Collection.m_Resources)
  {
    if (e.m_sResourceTypeName.IsEmpty())
      continue;

    const ezRTTI* pRtti = ezResourceManager::FindResourceForAssetType(e.m_sResourceTypeName);

    if (pRtti == nullptr)
      continue;

    ezTypelessResourceHandle hTypeless = ezResourceManager::LoadResourceByType(pRtti, e.m_sRedirectionName);
    m_hPreloadedResources.PushBack(hTypeless);

    ezResourceManager::PreloadResource(hTypeless, tShouldBeAvailableIn);
  }
}

ezResourceLoadDesc ezCollectionResource::CreateResource(ezCollectionResourceDescriptor&& descriptor)
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
  res.m_uiQualityLevelsLoadable = 1;
  res.m_State = ezResourceState::Unloaded;

  {
    UnregisterNames();

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
    if (!entry.m_sLookupName.IsEmpty())
    {
      ezResourceManager::RegisterNamedResource(entry.m_sLookupName, entry.m_sRedirectionName);
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
    if (!entry.m_sLookupName.IsEmpty())
    {
      ezResourceManager::UnregisterNamedResource(entry.m_sLookupName);
    }
  }
}

void ezCollectionResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  const ezUInt8 uiIdentifier = 0xC0; // dummy to fill the header to 32 Bit
  const ezUInt16 uiNumResources = m_Resources.GetCount();

  stream << uiVersion;
  stream << uiIdentifier;
  stream << uiNumResources;

  for (ezUInt32 i = 0; i < uiNumResources; ++i)
  {
    stream << m_Resources[i].m_sResourceTypeName;
    stream << m_Resources[i].m_sLookupName;
    stream << m_Resources[i].m_sRedirectionName;
  }
}

void ezCollectionResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  ezUInt8 uiIdentifier = 0;
  ezUInt16 uiNumResources = 0;

  stream >> uiVersion;
  stream >> uiIdentifier;
  stream >> uiNumResources;

  EZ_ASSERT_DEV(uiIdentifier == 0xC0, "File does not contain a valid ezCollectionResourceDescriptor");
  EZ_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  m_Resources.SetCount(uiNumResources);

  for (ezUInt32 i = 0; i < uiNumResources; ++i)
  {
    stream >> m_Resources[i].m_sResourceTypeName;
    stream >> m_Resources[i].m_sLookupName;
    stream >> m_Resources[i].m_sRedirectionName;
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Collection_Implementation_CollectionResource);
