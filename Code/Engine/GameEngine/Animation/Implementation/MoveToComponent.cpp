#include <GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/MoveToComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezMoveToComponent, 3, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning),
    EZ_MEMBER_PROPERTY("TranslationSpeed", m_fMaxTranslationSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("TranslationAcceleration", m_fTranslationAcceleration),
    EZ_MEMBER_PROPERTY("TranslationDeceleration", m_fTranslationDeceleration),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Transform"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetTargetPosition, In, "position"),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezMoveToComponent::ezMoveToComponent() = default;
ezMoveToComponent::~ezMoveToComponent() = default;

void ezMoveToComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_Flags.GetValue();
  s << m_fCurTranslationSpeed;
  s << m_fMaxTranslationSpeed;
  s << m_fTranslationAcceleration;
  s << m_fTranslationDeceleration;
  s << m_vTargetPosition;
}


void ezMoveToComponent::DeserializeComponent(ezWorldReader& stream)
{
  auto& s = stream.GetStream();

  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_Flags;
  s >> m_fCurTranslationSpeed;
  s >> m_fMaxTranslationSpeed;
  s >> m_fTranslationAcceleration;
  s >> m_fTranslationDeceleration;
  s >> m_vTargetPosition;
}

void ezMoveToComponent::SetRunning(bool bRunning)
{
  m_Flags.AddOrRemove(ezMoveToComponentFlags::Running, bRunning);
}

bool ezMoveToComponent::IsRunning() const
{
  return m_Flags.IsSet(ezMoveToComponentFlags::Running);
}

void ezMoveToComponent::SetTargetPosition(const ezVec3& pos)
{
  m_vTargetPosition = pos;
}

static float CalculateNewSpeed(float fRemainingDistance, float fCurSpeed, float fMaxSpeed, float fAcceleration, float fDeceleration, float fTimeStep)
{
  float fMaxAllowedSpeed = fMaxSpeed;

  if (fDeceleration > 0)
  {
    const float fMaxDecelerationTime = (fMaxSpeed / fDeceleration);
    const float fMaxDecelerationDistance = fMaxDecelerationTime * fMaxSpeed;

    if (fRemainingDistance <= fMaxDecelerationDistance)
    {
      fMaxAllowedSpeed = fMaxSpeed * (fRemainingDistance / fMaxDecelerationDistance);
    }
  }

  float fMaxNewSpeed = fMaxSpeed;

  if (fAcceleration > 0)
  {
    fMaxNewSpeed = fCurSpeed + fTimeStep * fAcceleration;
  }

  return ezMath::Clamp(fMaxNewSpeed, 0.0f, fMaxAllowedSpeed);
}

void ezMoveToComponent::Update()
{
  if (!m_Flags.IsAnySet(ezMoveToComponentFlags::Running))
    return;

  ezGameObject* pOwner = GetOwner();

  const ezVec3 vCurPos = pOwner->GetGlobalPosition();

  ezVec3 vDiff = m_vTargetPosition - vCurPos;
  const float fRemainingLength = vDiff.GetLength();

  if (ezMath::IsZero(fRemainingLength, 0.002f))
  {
    SetRunning(false);
    pOwner->SetGlobalPosition(m_vTargetPosition);

    ezMsgAnimationReachedEnd msg;
    m_ReachedEndMsgSender.SendMessage(msg, this, GetOwner());

    return;
  }

  const ezVec3 vDir = vDiff / fRemainingLength;

  m_fCurTranslationSpeed = CalculateNewSpeed(fRemainingLength, m_fCurTranslationSpeed, m_fMaxTranslationSpeed, m_fTranslationAcceleration, m_fTranslationDeceleration, GetWorld()->GetClock().GetTimeDiff().GetSeconds());

  const float fTravelDist = ezMath::Min<float>(fRemainingLength, m_fCurTranslationSpeed * GetWorld()->GetClock().GetTimeDiff().GetSeconds());

  pOwner->SetGlobalPosition(vCurPos + vDir * fTravelDist);
}
