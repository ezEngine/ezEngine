#include <GameUtils/PCH.h>
#include <GameUtils/Surfaces/SurfaceResource.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <GameUtils/Prefabs/PrefabResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceResource, 1, ezRTTIDefaultAllocator<ezSurfaceResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezEvent<const ezSurfaceResource::Event&, ezMutex> ezSurfaceResource::s_Events;

ezSurfaceResource::ezSurfaceResource() : ezResource<ezSurfaceResource, ezSurfaceResourceDescriptor>(DoUpdate::OnMainThread, 1)
{
  m_pPhysicsMaterial = nullptr;
}

ezSurfaceResource::~ezSurfaceResource()
{
  EZ_ASSERT_DEV(m_pPhysicsMaterial == nullptr, "Physics material has not been cleaned up properly");
}

ezResourceLoadDesc ezSurfaceResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  Event e;
  e.m_pSurface = this;
  e.m_Type = Event::Type::Destroyed;
  s_Events.Broadcast(e);

  return res;
}

ezResourceLoadDesc ezSurfaceResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezSurfaceResource::UpdateContent", GetResourceDescription().GetData());

  m_Interactions.Clear();

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


  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  {
    ezResourceHandleReadContext context;
    context.BeginReadingFromStream(Stream);
    context.BeginRestoringHandles(Stream);

    ezSurfaceResourceDescriptor dummy;
    dummy.Load(*Stream);

    context.EndReadingFromStream(Stream);
    context.EndRestoringHandles();

    CreateResource(dummy);
  }

  // configure the lookup table
  {
    m_Interactions.Reserve(m_Descriptor.m_Interactions.GetCount());
    for (const auto& i : m_Descriptor.m_Interactions)
    {
      ezTempHashedString s(i.m_sInteractionType.GetData());
      m_Interactions[s.GetHash()] = &i;
    }
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezSurfaceResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezSurfaceResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezSurfaceResource::CreateResource(const ezSurfaceResourceDescriptor& descriptor)
{
  m_Descriptor = descriptor;

  Event e;
  e.m_pSurface = this;
  e.m_Type = Event::Type::Created;
  s_Events.Broadcast(e);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}


bool ezSurfaceResource::InteractWithSurface(ezWorld* pWorld, const ezVec3& vPosition, const ezVec3& vSurfaceNormal, const ezVec3& vIncomingDirection, const ezTempHashedString& sInteraction)
{
  const ezSurfaceInteraction* pIA = nullptr;
  if (!m_Interactions.TryGetValue(sInteraction.GetHash(), pIA))
  {
    // if this type of interaction is not defined on this surface, try to find it on the base surface

    if (m_Descriptor.m_hBaseSurface.IsValid())
    {
      ezResourceLock<ezSurfaceResource> pBase(m_Descriptor.m_hBaseSurface, ezResourceAcquireMode::NoFallback);
      return pBase->InteractWithSurface(pWorld, vPosition, vSurfaceNormal, vIncomingDirection, sInteraction);
    }

    return false;
  }

  ezResourceLock<ezPrefabResource> pPrefab(pIA->m_hPrefab, ezResourceAcquireMode::NoFallback);

  ezVec3 vDir;

  switch (pIA->m_Alignment)
  {
  case  ezSurfaceInteractionAlignment::SurfaceNormal:
    vDir = vSurfaceNormal;
    break;

  case ezSurfaceInteractionAlignment::IncidentDirection:
    vDir = -vIncomingDirection;;
    break;

  case ezSurfaceInteractionAlignment::ReflectedDirection:
    vDir = vIncomingDirection.GetReflectedVector(vSurfaceNormal);
    break;
  }

  if (pIA->m_Deviation > ezAngle::Radian(0.0f))
  {
    // do random deviation
    // make sure to clamp max deviation angle


  }

  vDir.Normalize();
  const ezVec3 vTangent = vDir.GetOrthogonalVector().GetNormalized();
  const ezVec3 vBiTangent = vDir.Cross(vTangent);

  ezTransform t;
  t.m_vPosition = vPosition;
  t.m_Rotation.SetColumn(0, vTangent);
  t.m_Rotation.SetColumn(1, vBiTangent);
  t.m_Rotation.SetColumn(2, vDir);

  pPrefab->InstantiatePrefab(*pWorld, t);

  return true;
}

