#include <PCH.h>
#include <RtsGamePlugin/Components/ShipSteeringComponent.h>

EZ_BEGIN_COMPONENT_TYPE(RtsShipSteeringComponent, 1, ezComponentMode::Dynamic)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //}
  //EZ_END_PROPERTIES

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(RtsMsgNavigateTo, OnMsgNavigateTo)
  }
  EZ_END_MESSAGEHANDLERS

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("RTS Sample"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

RtsShipSteeringComponent::RtsShipSteeringComponent() = default;
RtsShipSteeringComponent::~RtsShipSteeringComponent() = default;

void RtsShipSteeringComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  //auto& s = stream.GetStream();

}

void RtsShipSteeringComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  //auto& s = stream.GetStream();

}

void RtsShipSteeringComponent::OnMsgNavigateTo(RtsMsgNavigateTo& msg)
{
  m_vTargetPosition = msg.m_vTargetPosition.GetAsVec3(0);
  m_Mode = RtsShipSteeringComponent::Mode::Steering;
}

void RtsShipSteeringComponent::UpdateSteering()
{
  const ezTransform t = GetOwner()->GetGlobalTransform();

  const ezVec3 vCurPos = t.m_vPosition;
  const ezVec3 vDiff = m_vTargetPosition - vCurPos;

  const float fLength = vDiff.GetLength();

  if (fLength <= 0.001f)
  {
    m_Mode = RtsShipSteeringComponent::Mode::None;
    return;
  }

  const ezVec3 vDir = vDiff / fLength;

  const float fMaxSpeed = 3.0f;

  const float fMaxDistance = ezMath::Min((float)GetWorld()->GetClock().GetTimeDiff().GetSeconds() * fMaxSpeed, fLength);

  const ezVec3 vNewPos = vCurPos + vDir * fMaxDistance;

  GetOwner()->SetGlobalPosition(vNewPos);
}

//////////////////////////////////////////////////////////////////////////

RtsShipSteeringComponentManager::RtsShipSteeringComponentManager(ezWorld* pWorld)
  : ezComponentManager<class RtsShipSteeringComponent, ezBlockStorageType::Compact>(pWorld)
{

}

void RtsShipSteeringComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(RtsShipSteeringComponentManager::SteeringUpdate, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::Async;
  desc.m_uiGranularity = 8;

  RegisterUpdateFunction(desc);
}

void RtsShipSteeringComponentManager::SteeringUpdate(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive() && it->m_Mode != RtsShipSteeringComponent::Mode::None)
    {
      it->UpdateSteering();
    }
  }
}
