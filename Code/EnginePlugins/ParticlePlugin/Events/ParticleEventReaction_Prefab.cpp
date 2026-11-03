#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Events/ParticleEventReaction_Prefab.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEventReaction_Prefab, 1, ezRTTIDefaultAllocator<ezParticleEventReaction_Prefab>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Prefab", m_sPrefab)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab"), new ezRequiredAttribute()),
    EZ_ENUM_MEMBER_PROPERTY("Alignment", ezSurfaceInteractionAlignment, m_Alignment),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEventReaction_Prefab::ezParticleEventReaction_Prefab() = default;

enum class ReactionPrefabVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleEventReaction_Prefab::Save(ezStreamWriter& inout_stream) const
{
  SUPER::Save(inout_stream);

  const ezUInt8 uiVersion = (int)ReactionPrefabVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_sPrefab;

  // Version 2
  inout_stream << m_Alignment;
}

void ezParticleEventReaction_Prefab::Load(ezStreamReader& inout_stream)
{
  SUPER::Load(inout_stream);

  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)ReactionPrefabVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  inout_stream >> m_sPrefab;

  if (uiVersion >= 2)
  {
    inout_stream >> m_Alignment;
  }

  if (!m_sPrefab.IsEmpty())
  {
    m_hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(m_sPrefab);
  }
}


//////////////////////////////////////////////////////////////////////////

void ezParticleEventReaction_Prefab::ProcessEvent(ezParticleEffectInstance* pOwner, const ezParticleEvent& e) const
{
  if (!m_hPrefab.IsValid())
    return;

  ezTransform trans;
  trans.m_vScale.Set(1.0f);
  trans.m_vPosition = e.m_vPosition;

  ezVec3 vAlignDir = e.m_vNormal;

  switch (m_Alignment)
  {
    case ezSurfaceInteractionAlignment::IncidentDirection:
      vAlignDir = -e.m_vDirection;
      break;

    case ezSurfaceInteractionAlignment::ReflectedDirection:
      vAlignDir = e.m_vDirection.GetReflectedVector(e.m_vNormal);
      break;

    case ezSurfaceInteractionAlignment::ReverseSurfaceNormal:
      vAlignDir = -e.m_vNormal;
      break;

    case ezSurfaceInteractionAlignment::ReverseIncidentDirection:
      vAlignDir = e.m_vDirection;
      ;
      break;

    case ezSurfaceInteractionAlignment::ReverseReflectedDirection:
      vAlignDir = -e.m_vDirection.GetReflectedVector(e.m_vNormal);
      break;

    case ezSurfaceInteractionAlignment::SurfaceNormal:
      break;
  }

  // rotate the prefab randomly along its main axis (the X axis)
  ezQuat qRot = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromRadian((float)pOwner->GetRNG().DoubleZeroToOneInclusive() * ezMath::Pi<float>() * 2.0f));

  vAlignDir.NormalizeIfNotZero(ezVec3::MakeAxisX()).IgnoreResult();

  trans.m_qRotation = ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), vAlignDir);
  trans.m_qRotation = trans.m_qRotation * qRot;

  ezResourceLock<ezPrefabResource> pPrefab(m_hPrefab, ezResourceAcquireMode::BlockTillLoaded);

  ezPrefabInstantiationOptions options;

  pPrefab->InstantiatePrefab(*pOwner->GetWorld(), trans, options);
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Events_ParticleEventReaction_Prefab);
