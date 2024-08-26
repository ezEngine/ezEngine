#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Events/ParticleEventReaction.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEventReactionFactory, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("EventType", m_sEventType),
    EZ_MEMBER_PROPERTY("Probability", m_uiProbability)->AddAttributes(new ezDefaultValueAttribute(100), new ezClampValueAttribute(1, 100)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEventReaction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

ezParticleEventReaction* ezParticleEventReactionFactory::CreateEventReaction(ezParticleEffectInstance* pOwner) const
{
  const ezRTTI* pRtti = GetEventReactionType();

  ezParticleEventReaction* pReaction = pRtti->GetAllocator()->Allocate<ezParticleEventReaction>();
  pReaction->Reset(pOwner);
  pReaction->m_sEventName = ezTempHashedString(m_sEventType.GetData());
  pReaction->m_uiProbability = m_uiProbability;

  CopyReactionProperties(pReaction, true);

  return pReaction;
}

enum class ReactionVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added probability

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleEventReactionFactory::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)ReactionVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_sEventType;

  // Version 2
  inout_stream << m_uiProbability;
}


void ezParticleEventReactionFactory::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)ReactionVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  inout_stream >> m_sEventType;

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiProbability;
  }
}

//////////////////////////////////////////////////////////////////////////

ezParticleEventReaction::ezParticleEventReaction() = default;
ezParticleEventReaction::~ezParticleEventReaction() = default;

void ezParticleEventReaction::Reset(ezParticleEffectInstance* pOwner)
{
  m_pOwnerEffect = pOwner;
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Events_ParticleEventReaction);
