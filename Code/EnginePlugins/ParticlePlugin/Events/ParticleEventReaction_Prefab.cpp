#include <ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <GameEngine/Physics/SurfaceResourceDescriptor.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Events/ParticleEventReaction_Prefab.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEventReactionFactory_Prefab, 1, ezRTTIDefaultAllocator<ezParticleEventReactionFactory_Prefab>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Prefab", m_sPrefab)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_ENUM_MEMBER_PROPERTY("Alignment", ezSurfaceInteractionAlignment, m_Alignment),
    //EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Prefab"), new ezExposeColorAlphaAttribute),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEventReaction_Prefab, 1, ezRTTIDefaultAllocator<ezParticleEventReaction_Prefab>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEventReactionFactory_Prefab::ezParticleEventReactionFactory_Prefab()
{
  // m_Parameters = EZ_DEFAULT_NEW(ezParticlePrefabParameters);
}

enum class ReactionPrefabVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  // Version_3, // added Prefab parameters

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleEventReactionFactory_Prefab::Save(ezStreamWriter& stream) const
{
  SUPER::Save(stream);

  const ezUInt8 uiVersion = (int)ReactionPrefabVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_sPrefab;

  // Version 2
  stream << m_Alignment;

  // Version 3
  // stream << m_Parameters->m_FloatParams.GetCount();
  // for (ezUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
  //{
  //  stream << m_Parameters->m_FloatParams[i].m_sName;
  //  stream << m_Parameters->m_FloatParams[i].m_Value;
  //}
  // stream << m_Parameters->m_ColorParams.GetCount();
  // for (ezUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
  //{
  //  stream << m_Parameters->m_ColorParams[i].m_sName;
  //  stream << m_Parameters->m_ColorParams[i].m_Value;
  //}
}

void ezParticleEventReactionFactory_Prefab::Load(ezStreamReader& stream)
{
  SUPER::Load(stream);

  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)ReactionPrefabVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  stream >> m_sPrefab;

  if (uiVersion >= 2)
  {
    stream >> m_Alignment;
  }

  // if (uiVersion >= 3)
  //{
  //  ezUInt32 numFloats, numColors;

  //  stream >> numFloats;
  //  m_Parameters->m_FloatParams.SetCountUninitialized(numFloats);

  //  for (ezUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
  //  {
  //    stream >> m_Parameters->m_FloatParams[i].m_sName;
  //    stream >> m_Parameters->m_FloatParams[i].m_Value;
  //  }

  //  stream >> numColors;
  //  m_Parameters->m_ColorParams.SetCountUninitialized(numColors);

  //  for (ezUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
  //  {
  //    stream >> m_Parameters->m_ColorParams[i].m_sName;
  //    stream >> m_Parameters->m_ColorParams[i].m_Value;
  //  }
  //}
}


const ezRTTI* ezParticleEventReactionFactory_Prefab::GetEventReactionType() const
{
  return ezGetStaticRTTI<ezParticleEventReaction_Prefab>();
}


void ezParticleEventReactionFactory_Prefab::CopyReactionProperties(ezParticleEventReaction* pObject, bool bFirstTime) const
{
  ezParticleEventReaction_Prefab* pReaction = static_cast<ezParticleEventReaction_Prefab*>(pObject);

  pReaction->m_hPrefab.Invalidate();
  pReaction->m_Alignment = m_Alignment;

  if (!m_sPrefab.IsEmpty())
    pReaction->m_hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(m_sPrefab);

  // pReaction->m_Parameters = m_Parameters;
}
//
// const ezRangeView<const char*, ezUInt32> ezParticleEventReactionFactory_Prefab::GetParameters() const
//{
//  return ezRangeView<const char*, ezUInt32>(
//      [this]() -> ezUInt32 { return 0; },
//      [this]() -> ezUInt32 { return m_Parameters->m_FloatParams.GetCount() + m_Parameters->m_ColorParams.GetCount(); },
//      [this](ezUInt32& it) { ++it; },
//      [this](const ezUInt32& it) -> const char* {
//        if (it < m_Parameters->m_FloatParams.GetCount())
//          return m_Parameters->m_FloatParams[it].m_sName.GetData();
//        else
//          return m_Parameters->m_ColorParams[it - m_Parameters->m_FloatParams.GetCount()].m_sName.GetData();
//      });
//}
//
// void ezParticleEventReactionFactory_Prefab::SetParameter(const char* szKey, const ezVariant& var)
//{
//  const ezTempHashedString th(szKey);
//  if (var.CanConvertTo<float>())
//  {
//    float value = var.ConvertTo<float>();
//
//    for (ezUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
//    {
//      if (m_Parameters->m_FloatParams[i].m_sName == th)
//      {
//        if (m_Parameters->m_FloatParams[i].m_Value != value)
//        {
//          m_Parameters->m_FloatParams[i].m_Value = value;
//        }
//        return;
//      }
//    }
//
//    auto& e = m_Parameters->m_FloatParams.ExpandAndGetRef();
//    e.m_sName.Assign(szKey);
//    e.m_Value = value;
//
//    return;
//  }
//
//  if (var.CanConvertTo<ezColor>())
//  {
//    ezColor value = var.ConvertTo<ezColor>();
//
//    for (ezUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
//    {
//      if (m_Parameters->m_ColorParams[i].m_sName == th)
//      {
//        if (m_Parameters->m_ColorParams[i].m_Value != value)
//        {
//          m_Parameters->m_ColorParams[i].m_Value = value;
//        }
//        return;
//      }
//    }
//
//    auto& e = m_Parameters->m_ColorParams.ExpandAndGetRef();
//    e.m_sName.Assign(szKey);
//    e.m_Value = value;
//
//    return;
//  }
//}
//
// void ezParticleEventReactionFactory_Prefab::RemoveParameter(const char* szKey)
//{
//  const ezTempHashedString th(szKey);
//
//  for (ezUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
//  {
//    if (m_Parameters->m_FloatParams[i].m_sName == th)
//    {
//      m_Parameters->m_FloatParams.RemoveAtAndSwap(i);
//      return;
//    }
//  }
//
//  for (ezUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
//  {
//    if (m_Parameters->m_ColorParams[i].m_sName == th)
//    {
//      m_Parameters->m_ColorParams.RemoveAtAndSwap(i);
//      return;
//    }
//  }
//}
//
// bool ezParticleEventReactionFactory_Prefab::GetParameter(const char* szKey, ezVariant& out_value) const
//{
//  const ezTempHashedString th(szKey);
//
//  for (const auto& e : m_Parameters->m_FloatParams)
//  {
//    if (e.m_sName == th)
//    {
//      out_value = e.m_Value;
//      return true;
//    }
//  }
//  for (const auto& e : m_Parameters->m_ColorParams)
//  {
//    if (e.m_sName == th)
//    {
//      out_value = e.m_Value;
//      return true;
//    }
//  }
//  return false;
//}

//////////////////////////////////////////////////////////////////////////

ezParticleEventReaction_Prefab::ezParticleEventReaction_Prefab() = default;
ezParticleEventReaction_Prefab::~ezParticleEventReaction_Prefab() = default;

void ezParticleEventReaction_Prefab::ProcessEvent(const ezParticleEvent& e)
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
  }

  // rotate the prefab randomly along its main axis (the X axis)
  ezQuat qRot;
  qRot.SetFromAxisAndAngle(
    ezVec3(1, 0, 0), ezAngle::Radian((float)m_pOwnerEffect->GetRNG().DoubleZeroToOneInclusive() * ezMath::BasicType<float>::Pi() * 2.0f));

  trans.m_qRotation.SetShortestRotation(ezVec3(1, 0, 0), vAlignDir);
  trans.m_qRotation = trans.m_qRotation * qRot;

  ezResourceLock<ezPrefabResource> pPrefab(m_hPrefab, ezResourceAcquireMode::BlockTillLoaded);
  pPrefab->InstantiatePrefab(*m_pOwnerEffect->GetWorld(), trans, ezGameObjectHandle(), nullptr, nullptr, nullptr, false);
}
