#include <Core/CorePCH.h>

#include <Core/Messages/ApplyOnlyToMessage.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceResource, 1, ezRTTIDefaultAllocator<ezSurfaceResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezSurfaceResource);
// clang-format on

ezEvent<const ezSurfaceResourceEvent&, ezMutex> ezSurfaceResource::s_Events;

ezSurfaceResource::ezSurfaceResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezSurfaceResource::~ezSurfaceResource()
{
  ezSurfaceResourceEvent e;
  e.m_pSurface = this;
  e.m_Type = ezSurfaceResourceEvent::Type::Destroyed;
  s_Events.Broadcast(e);

  EZ_ASSERT_DEV(m_pPhysicsMaterialPhysX == nullptr, "Physics material has not been cleaned up properly");
  EZ_ASSERT_DEV(m_pPhysicsMaterialJolt == nullptr, "Physics material has not been cleaned up properly");
}

ezResourceLoadDesc ezSurfaceResource::UnloadData(Unload WhatToUnload)
{
  EZ_IGNORE_UNUSED(WhatToUnload);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezSurfaceResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezSurfaceResource::UpdateContent", GetResourceIdOrDescription());

  m_Interactions.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  {
    ezSurfaceResourceDescriptor dummy;
    dummy.Load(*Stream);

    CreateResource(std::move(dummy));
  }

  // configure the lookup table
  {
    m_Interactions.Reserve(m_Descriptor.m_Interactions.GetCount());
    for (const auto& i : m_Descriptor.m_Interactions)
    {
      ezTempHashedString s(i.m_sInteractionType.GetData());
      auto& item = m_Interactions.ExpandAndGetRef();
      item.m_uiInteractionTypeHash = s.GetHash();
      item.m_pInteraction = &i;
    }

    m_Interactions.Sort([](const SurfInt& lhs, const SurfInt& rhs) -> bool
      {
      if (lhs.m_uiInteractionTypeHash != rhs.m_uiInteractionTypeHash)
        return lhs.m_uiInteractionTypeHash < rhs.m_uiInteractionTypeHash;

      return lhs.m_pInteraction->m_fImpulseThreshold > rhs.m_pInteraction->m_fImpulseThreshold; });
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezSurfaceResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezSurfaceResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezSurfaceResource, ezSurfaceResourceDescriptor)
{
  m_Descriptor = descriptor;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  ezSurfaceResourceEvent e;
  e.m_pSurface = this;
  e.m_Type = ezSurfaceResourceEvent::Type::Created;
  s_Events.Broadcast(e);

  return res;
}

const ezSurfaceInteraction* ezSurfaceResource::FindInteraction(const ezSurfaceResource* pCurSurf, ezUInt64 uiHash, float fImpulseSqr, float& out_fImpulseParamValue)
{
  while (true)
  {
    bool bFoundAny = false;

    // try to find a matching interaction
    for (const auto& interaction : pCurSurf->m_Interactions)
    {
      if (interaction.m_uiInteractionTypeHash > uiHash)
        break;

      if (interaction.m_uiInteractionTypeHash == uiHash)
      {
        bFoundAny = true;

        // only use it if the threshold is large enough
        if (fImpulseSqr >= ezMath::Square(interaction.m_pInteraction->m_fImpulseThreshold))
        {
          const float fImpulse = ezMath::Sqrt(fImpulseSqr);
          out_fImpulseParamValue = (fImpulse - interaction.m_pInteraction->m_fImpulseThreshold) * interaction.m_pInteraction->m_fImpulseScale;

          return interaction.m_pInteraction;
        }
      }
    }

    // if we did find something, we just never exceeded the threshold, then do not search in the base surface
    if (bFoundAny)
      break;

    if (pCurSurf->m_Descriptor.m_hBaseSurface.IsValid())
    {
      ezResourceLock<ezSurfaceResource> pBase(pCurSurf->m_Descriptor.m_hBaseSurface, ezResourceAcquireMode::BlockTillLoaded);
      pCurSurf = pBase.GetPointer();
    }
    else
    {
      break;
    }
  }

  return nullptr;
}

bool ezSurfaceResource::InteractWithSurface(ezWorld* pWorld, ezGameObjectHandle hObject, const ezVec3& vPosition, const ezVec3& vSurfaceNormal, const ezVec3& vIncomingDirection, const ezTempHashedString& sInteraction, const ezUInt16* pOverrideTeamID, float fImpulseSqr /*= 0.0f*/) const
{
  float fImpulseParam = 0;
  const ezSurfaceInteraction* pIA = FindInteraction(this, sInteraction.GetHash(), fImpulseSqr, fImpulseParam);

  if (pIA == nullptr)
    return false;

  // defined, but set to be empty
  if (!pIA->m_hPrefab.IsValid())
    return false;

  ezResourceLock<ezPrefabResource> pPrefab(pIA->m_hPrefab, ezResourceAcquireMode::BlockTillLoaded);

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
    double randomAngle = pWorld->GetRandomNumberGenerator().DoubleMinMax(0.0, ezMath::Pi<double>() * 2.0);

    ezMat3 rotMat = ezMat3::MakeAxisRotation(vDir, ezAngle::MakeFromRadian((float)randomAngle));

    vTangent = rotMat * vTangent;
  }

  if (pIA->m_Deviation > ezAngle::MakeFromRadian(0.0f))
  {
    ezAngle maxDeviation;

    /// \todo do random deviation, make sure to clamp max deviation angle
    switch (pIA->m_Alignment)
    {
      case ezSurfaceInteractionAlignment::IncidentDirection:
      case ezSurfaceInteractionAlignment::ReverseReflectedDirection:
      {
        const float fCosAngle = vDir.Dot(-vSurfaceNormal);
        const float fMaxDeviation = ezMath::Pi<float>() - ezMath::ACos(fCosAngle).GetRadian();

        maxDeviation = ezMath::Min(pIA->m_Deviation, ezAngle::MakeFromRadian(fMaxDeviation));
      }
      break;

      case ezSurfaceInteractionAlignment::ReflectedDirection:
      case ezSurfaceInteractionAlignment::ReverseIncidentDirection:
      {
        const float fCosAngle = vDir.Dot(vSurfaceNormal);
        const float fMaxDeviation = ezMath::Pi<float>() - ezMath::ACos(fCosAngle).GetRadian();

        maxDeviation = ezMath::Min(pIA->m_Deviation, ezAngle::MakeFromRadian(fMaxDeviation));
      }
      break;

      default:
        maxDeviation = pIA->m_Deviation;
        break;
    }

    const ezAngle deviation = ezAngle::MakeFromRadian((float)pWorld->GetRandomNumberGenerator().DoubleMinMax(-maxDeviation.GetRadian(), maxDeviation.GetRadian()));

    // tilt around the tangent (we don't want to compute another random rotation here)
    ezMat3 matTilt = ezMat3::MakeAxisRotation(vTangent, deviation);

    vDir = matTilt * vDir;
  }


  // finally compute the bi-tangent
  const ezVec3 vBiTangent = vDir.CrossRH(vTangent);

  ezMat3 mRot;
  mRot.SetColumn(0, vDir); // we always use X as the main axis, so align X with the direction
  mRot.SetColumn(1, vTangent);
  mRot.SetColumn(2, vBiTangent);

  ezTransform t;
  t.m_vPosition = vPosition;
  t.m_qRotation = ezQuat::MakeFromMat3(mRot);
  t.m_vScale.Set(1.0f);

  // attach to dynamic objects
  ezGameObjectHandle hParent;

  ezGameObject* pObject = nullptr;
  if (pWorld->TryGetObject(hObject, pObject) && pObject->IsDynamic())
  {
    hParent = hObject;
    t = ezTransform::MakeLocalTransform(pObject->GetGlobalTransform(), t);
  }

  ezHybridArray<ezGameObject*, 8> rootObjects;

  ezPrefabInstantiationOptions options;
  options.m_hParent = hParent;
  options.m_pCreatedRootObjectsOut = &rootObjects;
  options.m_pOverrideTeamID = pOverrideTeamID;

  pPrefab->InstantiatePrefab(*pWorld, t, options, &pIA->m_Parameters);

  {
    ezMsgSetFloatParameter msgSetFloat;
    msgSetFloat.m_sParameterName = "Impulse";
    msgSetFloat.m_fValue = fImpulseParam;

    for (auto pRootObject : rootObjects)
    {
      pRootObject->PostMessageRecursive(msgSetFloat, ezTime::MakeZero(), ezObjectMsgQueueType::AfterInitialized);
    }
  }

  if (pObject != nullptr && pObject->IsDynamic())
  {
    ezMsgOnlyApplyToObject msg;
    msg.m_hObject = hParent;

    for (auto pRootObject : rootObjects)
    {
      pRootObject->PostMessageRecursive(msg, ezTime::MakeZero(), ezObjectMsgQueueType::AfterInitialized);
    }
  }

  return true;
}

bool ezSurfaceResource::IsBasedOn(const ezSurfaceResource* pThisOrBaseSurface) const
{
  if (pThisOrBaseSurface == this)
    return true;

  if (m_Descriptor.m_hBaseSurface.IsValid())
  {
    ezResourceLock<ezSurfaceResource> pBase(m_Descriptor.m_hBaseSurface, ezResourceAcquireMode::BlockTillLoaded);

    return pBase->IsBasedOn(pThisOrBaseSurface);
  }

  return false;
}

bool ezSurfaceResource::IsBasedOn(const ezSurfaceResourceHandle hThisOrBaseSurface) const
{
  ezResourceLock<ezSurfaceResource> pThisOrBaseSurface(hThisOrBaseSurface, ezResourceAcquireMode::BlockTillLoaded);

  return IsBasedOn(pThisOrBaseSurface.GetPointer());
}


EZ_STATICLINK_FILE(Core, Core_Physics_Implementation_SurfaceResource);
