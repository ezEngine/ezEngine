#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectDescriptor, 2, ezRTTIDefaultAllocator<ezParticleEffectDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("WhenInvisible", ezEffectInvisibleUpdateRate, m_InvisibleUpdateRate),
    EZ_MEMBER_PROPERTY("AlwaysShared", m_bAlwaysShared),
    EZ_MEMBER_PROPERTY("SimulateInLocalSpace", m_bSimulateInLocalSpace),
    EZ_MEMBER_PROPERTY("ApplyOwnerVelocity", m_fApplyInstanceVelocity)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("PreSimulateDuration", m_PreSimulateDuration),
    EZ_MEMBER_PROPERTY("NumWindSamples", m_vNumWindSamples)->AddAttributes(new ezDefaultValueAttribute(ezVec3U32(1)), new ezClampValueAttribute(ezVec3U32(1), ezVec3U32(8))),
    EZ_MAP_MEMBER_PROPERTY("FloatParameters", m_FloatParameters),
    EZ_MAP_MEMBER_PROPERTY("ColorParameters", m_ColorParameters)->AddAttributes(new ezExposeColorAlphaAttribute),
    EZ_SET_ACCESSOR_PROPERTY("ParticleSystems", GetParticleSystems, AddParticleSystem, RemoveParticleSystem)->AddFlags(ezPropertyFlags::PointerOwner),
    EZ_SET_ACCESSOR_PROPERTY("EventReactions", GetEventReactions, AddEventReaction, RemoveEventReaction)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEffectDescriptor::ezParticleEffectDescriptor() = default;

ezParticleEffectDescriptor::~ezParticleEffectDescriptor()
{
  ClearSystems();
  ClearEventReactions();
}

void ezParticleEffectDescriptor::ClearSystems()
{
  for (auto pSystem : m_ParticleSystems)
  {
    pSystem->GetDynamicRTTI()->GetAllocator()->Deallocate(pSystem);
  }

  m_ParticleSystems.Clear();
}


void ezParticleEffectDescriptor::ClearEventReactions()
{
  for (auto pReaction : m_EventReactions)
  {
    pReaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pReaction);
  }

  m_EventReactions.Clear();
}

enum class ParticleEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4,
  Version_5,  // m_bAlwaysShared
  Version_6,  // added parameters
  Version_7,  // added instance velocity
  Version_8,  // added event reactions
  Version_9,  // breaking change
  Version_10, // added wind samples

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleEffectDescriptor::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)ParticleEffectVersion::Version_Current;

  inout_stream << uiVersion;

  const ezUInt32 uiNumSystems = m_ParticleSystems.GetCount();

  inout_stream << uiNumSystems;

  // Version 3
  inout_stream << m_bSimulateInLocalSpace;
  inout_stream << m_PreSimulateDuration;
  // Version 4
  inout_stream << m_InvisibleUpdateRate;
  // Version 5
  inout_stream << m_bAlwaysShared;

  // Version 3
  for (auto pSystem : m_ParticleSystems)
  {
    inout_stream << pSystem->GetDynamicRTTI()->GetTypeName();

    pSystem->Save(inout_stream);
  }

  // Version 6
  {
    ezUInt8 paramCol = static_cast<ezUInt8>(m_ColorParameters.GetCount());
    inout_stream << paramCol;
    for (auto it = m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      inout_stream << it.Key();
      inout_stream << it.Value();
    }

    ezUInt8 paramFloat = static_cast<ezUInt8>(m_FloatParameters.GetCount());
    inout_stream << paramFloat;
    for (auto it = m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      inout_stream << it.Key();
      inout_stream << it.Value();
    }
  }

  // Version 7
  inout_stream << m_fApplyInstanceVelocity;

  // Version 8
  {
    const ezUInt32 uiNumReactions = m_EventReactions.GetCount();
    inout_stream << uiNumReactions;

    for (auto pReaction : m_EventReactions)
    {
      inout_stream << pReaction->GetDynamicRTTI()->GetTypeName();

      pReaction->Save(inout_stream);
    }
  }

  // Version 10
  inout_stream << m_vNumWindSamples;
}


void ezParticleEffectDescriptor::Load(ezStreamReader& inout_stream)
{
  ClearSystems();
  ClearEventReactions();

  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= (int)ParticleEffectVersion::Version_Current, "Unknown particle effect template version {0}", uiVersion);

  if (uiVersion < (int)ParticleEffectVersion::Version_9)
  {
    ezLog::SeriousWarning("Unsupported old particle effect version");
    return;
  }

  ezUInt32 uiNumSystems = 0;
  inout_stream >> uiNumSystems;

  inout_stream >> m_bSimulateInLocalSpace;
  inout_stream >> m_PreSimulateDuration;
  inout_stream >> m_InvisibleUpdateRate;
  inout_stream >> m_bAlwaysShared;

  m_ParticleSystems.SetCountUninitialized(uiNumSystems);

  ezStringBuilder sType;

  for (auto& pSystem : m_ParticleSystems)
  {
    inout_stream >> sType;

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect type '{0}'", sType);

    pSystem = pRtti->GetAllocator()->Allocate<ezParticleSystemDescriptor>();

    pSystem->Load(inout_stream);
  }

  ezStringBuilder key;
  m_ColorParameters.Clear();
  m_FloatParameters.Clear();

  ezUInt8 paramCol;
  inout_stream >> paramCol;
  for (ezUInt32 i = 0; i < paramCol; ++i)
  {
    ezColor val;
    inout_stream >> key;
    inout_stream >> val;
    m_ColorParameters[key] = val;
  }

  ezUInt8 paramFloat;
  inout_stream >> paramFloat;
  for (ezUInt32 i = 0; i < paramFloat; ++i)
  {
    float val;
    inout_stream >> key;
    inout_stream >> val;
    m_FloatParameters[key] = val;
  }

  inout_stream >> m_fApplyInstanceVelocity;

  ezUInt32 uiNumReactions = 0;
  inout_stream >> uiNumReactions;

  m_EventReactions.SetCountUninitialized(uiNumReactions);

  for (auto& pReaction : m_EventReactions)
  {
    inout_stream >> sType;

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect event reaction type '{0}'", sType);

    pReaction = pRtti->GetAllocator()->Allocate<ezParticleEventReactionFactory>();

    pReaction->Load(inout_stream);
  }

  if (uiVersion >= (int)ParticleEffectVersion::Version_10)
  {
    inout_stream >> m_vNumWindSamples;
  }
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectDescriptor);
