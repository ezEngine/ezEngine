#include <PCH.h>

#include <ParticlePlugin/Events/ParticleEventReaction.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEventReactionFactory, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("EventType", m_sEventType)
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

  CopyReactionProperties(pReaction);
  pReaction->AfterPropertiesConfigured(true);

  return pReaction;
}

enum class ReactionVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleEventReactionFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)ReactionVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_sEventType;
}


void ezParticleEventReactionFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)ReactionVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  stream >> m_sEventType;
}

//////////////////////////////////////////////////////////////////////////

ezParticleEventReaction::ezParticleEventReaction() = default;
ezParticleEventReaction::~ezParticleEventReaction() = default;

void ezParticleEventReaction::ProcessEventQueue(const ezParticleEventQueue* pQueue) {}
void ezParticleEventReaction::AfterPropertiesConfigured(bool bFirstTime) {}

void ezParticleEventReaction::Reset(ezParticleEffectInstance* pOwner)
{
  m_pOwnerEffect = pOwner;
}
