#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <GameEngine/Surfaces/SurfaceResource.h>
#include <RendererCore/Messages/ApplyOnlyToMessage.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceResource, 1, ezRTTIDefaultAllocator<ezSurfaceResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezEvent<const ezSurfaceResource::Event&, ezMutex> ezSurfaceResource::s_Events;

ezSurfaceResource::ezSurfaceResource()
    : ezResource<ezSurfaceResource, ezSurfaceResourceDescriptor>(DoUpdate::OnAnyThread, 1)
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


bool ezSurfaceResource::InteractWithSurface(ezWorld* pWorld, ezGameObjectHandle hObject, const ezVec3& vPosition,
                                            const ezVec3& vSurfaceNormal, const ezVec3& vIncomingDirection,
                                            const ezTempHashedString& sInteraction, const ezUInt16* pOverrideTeamID)
{
  const ezSurfaceInteraction* pIA = nullptr;
  if (!m_Interactions.TryGetValue(sInteraction.GetHash(), pIA))
  {
    // if this type of interaction is not defined on this surface, try to find it on the base surface

    if (m_Descriptor.m_hBaseSurface.IsValid())
    {
      ezResourceLock<ezSurfaceResource> pBase(m_Descriptor.m_hBaseSurface, ezResourceAcquireMode::NoFallback);
      return pBase->InteractWithSurface(pWorld, hObject, vPosition, vSurfaceNormal, vIncomingDirection, sInteraction, pOverrideTeamID);
    }

    return false;
  }

  // defined, but set to be empty
  if (!pIA->m_hPrefab.IsValid())
    return false;

  ezResourceLock<ezPrefabResource> pPrefab(pIA->m_hPrefab, ezResourceAcquireMode::NoFallback);

  ezVec3 vDir;

  switch (pIA->m_Alignment)
  {
    case ezSurfaceInteractionAlignment::SurfaceNormal:
      vDir = vSurfaceNormal;
      break;

    case ezSurfaceInteractionAlignment::IncidentDirection:
      vDir = -vIncomingDirection;
      ;
      break;

    case ezSurfaceInteractionAlignment::ReflectedDirection:
      vDir = vIncomingDirection.GetReflectedVector(vSurfaceNormal);
      break;

    case ezSurfaceInteractionAlignment::ReverseSurfaceNormal:
      vDir = -vSurfaceNormal;
      break;

    case ezSurfaceInteractionAlignment::ReverseIncidentDirection:
      vDir = vIncomingDirection;
      ;
      break;

    case ezSurfaceInteractionAlignment::ReverseReflectedDirection:
      vDir = -vIncomingDirection.GetReflectedVector(vSurfaceNormal);
      break;
  }

  vDir.Normalize();
  ezVec3 vTangent = vDir.GetOrthogonalVector().GetNormalized();

  // random rotation around the spawn direction
  {
    double randomAngle = pWorld->GetRandomNumberGenerator().DoubleInRange(0.0, ezMath::BasicType<double>::Pi() * 2.0);

    ezMat3 rotMat;
    rotMat.SetRotationMatrix(vDir, ezAngle::Radian((float)randomAngle));

    vTangent = rotMat * vTangent;
  }

  if (pIA->m_Deviation > ezAngle::Radian(0.0f))
  {
    ezAngle maxDeviation;

    /// \todo do random deviation, make sure to clamp max deviation angle
    switch (pIA->m_Alignment)
    {
      case ezSurfaceInteractionAlignment::IncidentDirection:
      case ezSurfaceInteractionAlignment::ReverseReflectedDirection:
      {
        const float fCosAngle = vDir.Dot(-vSurfaceNormal);
        const float fMaxDeviation = ezMath::BasicType<float>::Pi() - ezMath::ACos(fCosAngle).GetRadian();

        maxDeviation = ezMath::Min(pIA->m_Deviation, ezAngle::Radian(fMaxDeviation));
      }
      break;

      case ezSurfaceInteractionAlignment::ReflectedDirection:
      case ezSurfaceInteractionAlignment::ReverseIncidentDirection:
      {
        const float fCosAngle = vDir.Dot(vSurfaceNormal);
        const float fMaxDeviation = ezMath::BasicType<float>::Pi() - ezMath::ACos(fCosAngle).GetRadian();

        maxDeviation = ezMath::Min(pIA->m_Deviation, ezAngle::Radian(fMaxDeviation));
      }
      break;

      default:
        maxDeviation = pIA->m_Deviation;
        break;
    }

    const ezAngle deviation =
        ezAngle::Radian((float)pWorld->GetRandomNumberGenerator().DoubleMinMax(-maxDeviation.GetRadian(), maxDeviation.GetRadian()));

    // tilt around the tangent (we don't want to compute another random rotation here)
    ezMat3 matTilt;
    matTilt.SetRotationMatrix(vTangent, deviation);

    vDir = matTilt * vDir;
  }


  // finally compute the bi-tangent
  const ezVec3 vBiTangent = vDir.Cross(vTangent);

  ezMat3 mRot;
  mRot.SetColumn(0, vDir); // we always use X as the main axis, so align X with the direction
  mRot.SetColumn(1, vTangent);
  mRot.SetColumn(2, vBiTangent);

  ezTransform t;
  t.m_vPosition = vPosition;
  t.m_qRotation.SetFromMat3(mRot);
  t.m_vScale.Set(1.0f);

  // attach to dynamic objects
  ezGameObjectHandle hParent;

  ezGameObject* pObject = nullptr;
  if (pWorld->TryGetObject(hObject, pObject) && pObject->IsDynamic())
  {
    hParent = hObject;
    t.SetLocalTransform(pObject->GetGlobalTransform(), t);
  }

  ezHybridArray<ezGameObject*, 8> rootObjects;
  pPrefab->InstantiatePrefab(*pWorld, t, hParent, &rootObjects, pOverrideTeamID, nullptr);

  if (pObject != nullptr && pObject->IsDynamic())
  {
    ezMsgOnlyApplyToObject msg;
    msg.m_hObject = hParent;

    for (auto pRootObject : rootObjects)
    {
      pRootObject->PostMessageRecursive(msg, ezObjectMsgQueueType::AfterInitialized);
    }
  }

  return true;
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Surfaces_SurfaceResource);
