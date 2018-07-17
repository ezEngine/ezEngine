#include <PCH.h>

#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Events/ParticleEventReaction_Effect.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Components/ParticleComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEventReactionFactory_Effect, 1, ezRTTIDefaultAllocator<ezParticleEventReactionFactory_Effect>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Effect", m_sEffect)->AddAttributes(new ezAssetBrowserAttribute("Particle Effect")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEventReaction_Effect, 1, ezRTTIDefaultAllocator<ezParticleEventReaction_Effect>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEventReactionFactory_Effect::ezParticleEventReactionFactory_Effect() {}

enum class ReactionEffectVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleEventReactionFactory_Effect::Save(ezStreamWriter& stream) const
{
  SUPER::Save(stream);

  const ezUInt8 uiVersion = (int)ReactionEffectVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_sEffect;
}

void ezParticleEventReactionFactory_Effect::Load(ezStreamReader& stream)
{
  SUPER::Load(stream);

  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)ReactionEffectVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  stream >> m_sEffect;
}


const ezRTTI* ezParticleEventReactionFactory_Effect::GetEventReactionType() const
{
  return ezGetStaticRTTI<ezParticleEventReaction_Effect>();
}


void ezParticleEventReactionFactory_Effect::CopyReactionProperties(ezParticleEventReaction* pObject) const
{
  ezParticleEventReaction_Effect* pReaction = static_cast<ezParticleEventReaction_Effect*>(pObject);

  pReaction->m_hEffect.Invalidate();

  if (!m_sEffect.IsEmpty())
    pReaction->m_hEffect = ezResourceManager::LoadResource<ezParticleEffectResource>(m_sEffect);
}

//////////////////////////////////////////////////////////////////////////

ezParticleEventReaction_Effect::ezParticleEventReaction_Effect() = default;
ezParticleEventReaction_Effect::~ezParticleEventReaction_Effect() = default;

void ezParticleEventReaction_Effect::ProcessEventQueue(const ezParticleEventQueue* pQueue)
{
  if (pQueue->GetEventTypeHash() != m_sEventName.GetHash())
    return;

  ezGameObjectDesc god;
  god.m_bDynamic = true;

  for (const auto& e : pQueue->GetAllEvents())
  {
    god.m_LocalPosition = e.m_vPosition;

    // TODO: modes how to align the spawned effect (direction, normal, reflected dir, etc.)
    god.m_LocalRotation.SetShortestRotation(ezVec3(0, 0, 1), e.m_vDirection);

    ezGameObject* pObject = nullptr;
    m_pOwnerEffect->GetWorld()->CreateObject(god, pObject);

    ezParticleComponent* pComponent = nullptr;
    ezParticleComponent::CreateComponent(pObject, pComponent);

    pComponent->m_bIfContinuousStopRightAway = true;
    pComponent->m_OnFinishedAction = ezOnComponentFinishedAction2::DeleteGameObject;
    pComponent->SetParticleEffect(m_hEffect);
  }
}
