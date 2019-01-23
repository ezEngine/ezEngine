#include <PCH.h>

#include <Foundation/Utilities/Stats.h>
#include <RtsGamePlugin/Components/ShipSteeringComponent.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(RtsShipSteeringComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Speed", m_fMaxSpeed)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.1f, 100.0f)),
    EZ_MEMBER_PROPERTY("Acceleration", m_fMaxAcceleration)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.1f, 100.0f)),
    EZ_MEMBER_PROPERTY("Deceleration", m_fMaxDeceleration)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.1f, 100.0f)),
    EZ_MEMBER_PROPERTY("TurnSpeed", m_MaxTurnSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90.0f))),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(RtsMsgNavigateTo, OnMsgNavigateTo),
    EZ_MESSAGE_HANDLER(RtsMsgStopNavigation, OnMsgStopNavigation),
  }
  EZ_END_MESSAGEHANDLERS;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("RTS Sample"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

RtsShipSteeringComponent::RtsShipSteeringComponent() = default;
RtsShipSteeringComponent::~RtsShipSteeringComponent() = default;

void RtsShipSteeringComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fMaxSpeed;
  s << m_fMaxAcceleration;
  s << m_MaxTurnSpeed;
  s << m_fMaxDeceleration;
}

void RtsShipSteeringComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fMaxSpeed;
  s >> m_fMaxAcceleration;
  s >> m_MaxTurnSpeed;
  s >> m_fMaxDeceleration;
}

void RtsShipSteeringComponent::OnMsgNavigateTo(RtsMsgNavigateTo& msg)
{
  m_vTargetPosition = msg.m_vTargetPosition;
  m_Mode = RtsShipSteeringComponent::Mode::Steering;
}

void RtsShipSteeringComponent::OnMsgStopNavigation(RtsMsgStopNavigation& msg)
{
  if (m_Mode == RtsShipSteeringComponent::Mode::Steering)
  {
    m_Mode = RtsShipSteeringComponent::Mode::Stop;

    RtsMsgArrivedAtLocation msg;
    GetOwner()->SendMessage(msg);
  }
}

void RtsShipSteeringComponent::UpdateSteering()
{
  if (m_Mode == RtsShipSteeringComponent::Mode::None)
    return;

  const float tDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

  ezTransform transform = GetOwner()->GetGlobalTransform();

  const ezVec2 vOwnerPos = transform.m_vPosition.GetAsVec2();
  const ezVec2 vOwnerDir = GetOwner()->GetGlobalDirForwards().GetAsVec2();
  const ezVec2 vOwnerRight(-vOwnerDir.y, vOwnerDir.x);

  const float fArriveDistance = m_fCurrentSpeed * m_fCurrentSpeed / m_fMaxDeceleration;

  const ezVec2 vDistToTarget = m_vTargetPosition - vOwnerPos;
  float fDistToTarget = vDistToTarget.GetLength();
  const ezVec2 vDirToTarget = (fDistToTarget <= 0) ? vOwnerDir : (vDistToTarget / fDistToTarget);

  const bool bTargetIsInFront = vOwnerDir.Dot(vDirToTarget) > ezMath::Cos(ezAngle::Degree(60));
  const bool bTargetIsRight = (vOwnerRight.Dot(m_vTargetPosition) - vOwnerRight.Dot(vOwnerPos)) > 0;

  if (m_Mode == RtsShipSteeringComponent::Mode::Stop)
  {
    fDistToTarget = 0;
  }

  if (fDistToTarget <= fArriveDistance)
  {
    m_fCurrentSpeed -= m_fMaxDeceleration * tDiff;
  }
  else
  {
    if (bTargetIsInFront)
    {
      m_fCurrentSpeed = ezMath::Min(m_fCurrentSpeed + m_fMaxAcceleration * tDiff, m_fMaxSpeed);
    }
    else
    {
      m_fCurrentSpeed = ezMath::Max(m_fCurrentSpeed - m_fMaxDeceleration * tDiff, 0.0f);
    }
  }

  if (fDistToTarget > 0.5f)
  {
    const ezAngle angleToTurn = vDirToTarget.GetAngleBetween(vOwnerDir);

    if (angleToTurn != ezAngle())
    {
      const ezAngle toTurnNow = (bTargetIsRight ? 1.0f : -1.0f) * ezMath::Min(angleToTurn, m_MaxTurnSpeed * tDiff);

      ezQuat qRot;
      qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), toTurnNow);

      transform.m_qRotation = qRot * transform.m_qRotation;
    }
  }
  else
  {
    if (m_Mode == RtsShipSteeringComponent::Mode::Steering)
    {
      m_Mode = RtsShipSteeringComponent::Mode::Stop;

      RtsMsgArrivedAtLocation msg;
      GetOwner()->SendMessage(msg);
    }

    if (m_fCurrentSpeed <= 0)
    {
      m_fCurrentSpeed = 0;
      m_Mode = RtsShipSteeringComponent::Mode::None;
    }
  }

  {
    const ezVec2 vMove = vOwnerDir * m_fCurrentSpeed * tDiff;
    transform.m_vPosition += vMove.GetAsVec3(0);
    transform.m_vPosition.z = 0;
  }

  GetOwner()->SetGlobalTransform(transform);
}

//////////////////////////////////////////////////////////////////////////

RtsShipSteeringComponentManager::RtsShipSteeringComponentManager(ezWorld* pWorld)
    : ezComponentManager<class RtsShipSteeringComponent, ezBlockStorageType::Compact>(pWorld)
{
}

void RtsShipSteeringComponentManager::Initialize()
{
  // configure this system to update all components multi-threaded (async phase)

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(RtsShipSteeringComponentManager::SteeringUpdate, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::Async;
  desc.m_uiGranularity = 8;

  RegisterUpdateFunction(desc);
}

void RtsShipSteeringComponentManager::SteeringUpdate(const ezWorldModule::UpdateContext& context)
{
  if (RtsGameState::GetSingleton() == nullptr || RtsGameState::GetSingleton()->GetActiveGameMode() != RtsActiveGameMode::BattleMode)
    return;

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
    {
      it->UpdateSteering();
    }
  }
}
