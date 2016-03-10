#include <GameUtils/PCH.h>
#include <GameUtils/Components/TimedDeathComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezTimedDeathComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Min Delay", m_MinDelay)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant()), new ezDefaultValueAttribute(ezTime::Seconds(1.0))),
    EZ_MEMBER_PROPERTY("Delay Range", m_DelayRange)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant())),
  EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(ezComponentTriggerMessage, OnTriggered),
  EZ_END_MESSAGEHANDLERS
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Gameplay"),
  EZ_END_ATTRIBUTES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTimedDeathComponent::ezTimedDeathComponent()
{
  m_MinDelay = ezTime::Seconds(1.0);
  m_DelayRange = ezTime::Seconds(0.0);
}

void ezTimedDeathComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_MinDelay;
  s << m_DelayRange;
}

void ezTimedDeathComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_MinDelay;
  s >> m_DelayRange;

}

ezComponent::Initialization ezTimedDeathComponent::Initialize()
{
  ezComponentTriggerMessage msg;
  msg.m_hTargetComponent = GetHandle();

  ezWorld* pWorld = GetWorld();

  const ezTime tKill = ezTime::Seconds(pWorld->GetRandomNumberGenerator().DoubleInRange(m_MinDelay.GetSeconds(), m_DelayRange.GetSeconds()));

  pWorld->PostMessage(GetOwner()->GetHandle(), msg, ezObjectMsgQueueType::NextFrame, tKill, ezObjectMsgRouting::ToComponents);

  return ezComponent::Initialization::Done;
}

void ezTimedDeathComponent::OnTriggered(ezComponentTriggerMessage& msg)
{
  if (msg.m_hTargetComponent != GetHandle())
    return;

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}
