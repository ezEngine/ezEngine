#include <PCH.h>
#include <RtsGamePlugin/Components/TorpedoComponent.h>
#include <RtsGamePlugin/Components/ComponentMessages.h>

EZ_BEGIN_COMPONENT_TYPE(RtsTorpedoComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(10.0f)),
  }
  EZ_END_PROPERTIES

    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(RtsMsgSetTarget, OnMsgSetTarget),
  }
  EZ_END_MESSAGEHANDLERS

    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("RTS Sample"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

RtsTorpedoComponent::RtsTorpedoComponent() = default;
RtsTorpedoComponent::~RtsTorpedoComponent() = default;

void RtsTorpedoComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fSpeed;
}

void RtsTorpedoComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fSpeed;
}


void RtsTorpedoComponent::OnMsgSetTarget(RtsMsgSetTarget& msg)
{
  m_vTargetPosition = msg.m_vPosition;
  m_hTargetObject = msg.m_hObject;
}

void RtsTorpedoComponent::Update()
{
  // TODO: do this check in the component manager
  if (RtsGameState::GetSingleton()->GetActiveGameMode() != RtsActiveGameMode::BattleMode)
    return;

  ezGameObject* pObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pObject))
  {
    m_vTargetPosition = pObject->GetGlobalTransform().m_vPosition.GetAsVec2();
  }

  const ezTransform transform = GetOwner()->GetGlobalTransform();
  const ezVec2 vCurPos = transform.m_vPosition.GetAsVec2();
  const ezVec2 vDir = m_vTargetPosition - vCurPos;
  const float fDist = vDir.GetLength();

  const float tDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

  bool bExplode = false;

  if (fDist > 0)
  {
    const ezVec2 vDirNorm = vDir / fDist;
    float fTravelDist = m_fSpeed * tDiff;

    if (fTravelDist >= fDist)
    {
      fTravelDist = fDist;
      bExplode = true;
    }

    const ezVec3 vNewPos = transform.m_vPosition + vDirNorm.GetAsVec3(0) * fTravelDist;

    GetOwner()->SetGlobalPosition(vNewPos);
  }
  else
  {
    bExplode = true;
  }

  if (bExplode)
  {
    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
  }
}

