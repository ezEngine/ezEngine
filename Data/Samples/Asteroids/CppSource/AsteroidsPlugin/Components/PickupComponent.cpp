#include <AsteroidsPlugin/AsteroidsPluginPCH.h>

#include <AsteroidsPlugin/Components/AsteroidsGameComponent.h>
#include <AsteroidsPlugin/Components/PickupComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

namespace
{
  constexpr float s_fBlinkBeforeExpiry = 3.0f;
  constexpr float s_fBlinkPeriod = 0.3f;
  constexpr float s_fSpinDegreesPerSecond = 120.0f;
} // namespace

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(PickupType, 1)
  EZ_ENUM_CONSTANTS(PickupType::Shield, PickupType::TripleShot)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(PickupComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Type", PickupType, m_Type),
    EZ_MEMBER_PROPERTY("CollectSound", m_sCollectSound)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_MiniAudio_Sound", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(1.8f)),
    EZ_MEMBER_PROPERTY("LifeTime", m_LifeTime)->AddAttributes(new ezDefaultValueAttribute(ezTime::MakeFromSeconds(10))),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Asteroids"),
    new ezSphereVisualizerAttribute("Radius", ezColor::Yellow),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

PickupComponent::PickupComponent() = default;
PickupComponent::~PickupComponent() = default;

void PickupComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Type;
  s << m_sCollectSound;
  s << m_fRadius;
  s << m_LifeTime;
}

void PickupComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_Type;
  s >> m_sCollectSound;
  s >> m_fRadius;
  s >> m_LifeTime;
}

void PickupComponent::Update()
{
  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  const float fTimeDiff = tDiff.AsFloatInSeconds();

  m_LifeTime -= tDiff;
  if (m_LifeTime.IsZeroOrNegative())
  {
    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
    return;
  }

  ezGameObject* pOwner = GetOwner();

  // blink during the last seconds, to let the player know that it is about to go away
  const float fTimeLeft = m_LifeTime.AsFloatInSeconds();
  const bool bBlinkedOff = fTimeLeft < s_fBlinkBeforeExpiry && ezMath::Mod(fTimeLeft, s_fBlinkPeriod) < s_fBlinkPeriod * 0.5f;
  for (auto it = pOwner->GetChildren(); it.IsValid(); ++it)
    it->SetActiveFlag(!bBlinkedOff);

  const ezQuat qSpin = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(s_fSpinDegreesPerSecond * fTimeDiff));
  pOwner->SetLocalRotation(qSpin * pOwner->GetLocalRotation());

  AsteroidsGameComponent::MoveAndWrap(pOwner, m_vVelocity, m_fRadius);
}
