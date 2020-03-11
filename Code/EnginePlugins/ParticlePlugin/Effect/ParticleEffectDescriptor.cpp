#include <ParticlePluginPCH.h>

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
    EZ_MAP_MEMBER_PROPERTY("FloatParameters", m_FloatParameters),
    EZ_MAP_MEMBER_PROPERTY("ColorParameters", m_ColorParameters)->AddAttributes(new ezExposeColorAlphaAttribute),
    EZ_SET_ACCESSOR_PROPERTY("ParticleSystems", GetParticleSystems, AddParticleSystem, RemoveParticleSystem)->AddFlags(ezPropertyFlags::PointerOwner),
    EZ_SET_ACCESSOR_PROPERTY("EventReactions", GetEventReactions, AddEventReaction, RemoveEventReaction)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEffectDescriptor::ezParticleEffectDescriptor() {}

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
  Version_5, // m_bAlwaysShared
  Version_6, // added parameters
  Version_7, // added instance velocity
  Version_8, // added event reactions
  Version_9, // breaking change

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleEffectDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)ParticleEffectVersion::Version_Current;

  stream << uiVersion;

  const ezUInt32 uiNumSystems = m_ParticleSystems.GetCount();

  stream << uiNumSystems;

  // Version 3
  stream << m_bSimulateInLocalSpace;
  stream << m_PreSimulateDuration;
  // Version 4
  stream << m_InvisibleUpdateRate;
  // Version 5
  stream << m_bAlwaysShared;

  // Version 3
  for (auto pSystem : m_ParticleSystems)
  {
    stream << pSystem->GetDynamicRTTI()->GetTypeName();

    pSystem->Save(stream);
  }

  // Version 6
  {
    ezUInt8 paramCol = m_ColorParameters.GetCount();
    stream << paramCol;
    for (auto it = m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      stream << it.Key();
      stream << it.Value();
    }

    ezUInt8 paramFloat = m_FloatParameters.GetCount();
    stream << paramFloat;
    for (auto it = m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      stream << it.Key();
      stream << it.Value();
    }
  }

  // Version 7
  stream << m_fApplyInstanceVelocity;

  // Version 8
  {
    const ezUInt32 uiNumReactions = m_EventReactions.GetCount();
    stream << uiNumReactions;

    for (auto pReaction : m_EventReactions)
    {
      stream << pReaction->GetDynamicRTTI()->GetTypeName();

      pReaction->Save(stream);
    }
  }
}


void ezParticleEffectDescriptor::Load(ezStreamReader& stream)
{
  ClearSystems();
  ClearEventReactions();

  ezUInt8 uiVersion = 0;
  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= (int)ParticleEffectVersion::Version_Current, "Unknown particle effect template version {0}", uiVersion);

  if (uiVersion < (int)ParticleEffectVersion::Version_9)
  {
    ezLog::SeriousWarning("Unsupported old particle effect version");
    return;
  }

  ezUInt32 uiNumSystems = 0;
  stream >> uiNumSystems;

  stream >> m_bSimulateInLocalSpace;
  stream >> m_PreSimulateDuration;
  stream >> m_InvisibleUpdateRate;
  stream >> m_bAlwaysShared;

  m_ParticleSystems.SetCountUninitialized(uiNumSystems);

  ezStringBuilder sType;

  for (auto& pSystem : m_ParticleSystems)
  {
    stream >> sType;

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect type '{0}'", sType);

    pSystem = pRtti->GetAllocator()->Allocate<ezParticleSystemDescriptor>();

    pSystem->Load(stream);
  }

  ezStringBuilder key;
  m_ColorParameters.Clear();
  m_FloatParameters.Clear();

  ezUInt8 paramCol;
  stream >> paramCol;
  for (ezUInt32 i = 0; i < paramCol; ++i)
  {
    ezColor val;
    stream >> key;
    stream >> val;
    m_ColorParameters[key] = val;
  }

  ezUInt8 paramFloat;
  stream >> paramFloat;
  for (ezUInt32 i = 0; i < paramFloat; ++i)
  {
    float val;
    stream >> key;
    stream >> val;
    m_FloatParameters[key] = val;
  }

  stream >> m_fApplyInstanceVelocity;

  ezUInt32 uiNumReactions = 0;
  stream >> uiNumReactions;

  m_EventReactions.SetCountUninitialized(uiNumReactions);

  for (auto& pReaction : m_EventReactions)
  {
    stream >> sType;

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect event reaction type '{0}'", sType);

    pReaction = pRtti->GetAllocator()->Allocate<ezParticleEventReactionFactory>();

    pReaction->Load(stream);
  }
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectDescriptor);
