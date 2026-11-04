#include <AsteroidsPlugin/AsteroidsPluginPCH.h>

#include <AsteroidsPlugin/Components/AsteroidsGameComponent.h>
#include <AsteroidsPlugin/Components/ProjectileComponent.h>
#include <AsteroidsPlugin/Components/ShipComponent.h>
#include <Core/Input/InputManager.h>
#include <Core/Interfaces/SoundInterface.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Random.h>
#include <MiniAudioPlugin/Components/MiniAudioSoundComponent.h>

namespace
{
  constexpr double s_fShieldHitInvulnerability = 1.5;
  constexpr float s_fTripleShotSpreadDegrees = 12.0f;
  constexpr float s_fMaxBankDegrees = 35.0f;
  constexpr float s_fBankSpeed = 8.0f;
  constexpr float s_fBlinkPeriod = 0.2f;
  constexpr float s_fThrustVolumeFadeIn = 8.0f;
  constexpr float s_fThrustVolumeFadeOut = 5.0f;
} // namespace

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ShipComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("ProjectilePrefab", m_hProjectilePrefab)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_RESOURCE_MEMBER_PROPERTY("ExplosionPrefab", m_hExplosionPrefab)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("FireSound", m_sFireSound)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_MiniAudio_Sound", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("ShieldHitSound", m_sShieldHitSound)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_MiniAudio_Sound", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(1.5f)),
    EZ_MEMBER_PROPERTY("Thrust", m_fThrust)->AddAttributes(new ezDefaultValueAttribute(45.0f)),
    EZ_MEMBER_PROPERTY("MaxSpeed", m_fMaxSpeed)->AddAttributes(new ezDefaultValueAttribute(38.0f)),
    EZ_MEMBER_PROPERTY("Drag", m_fDrag)->AddAttributes(new ezDefaultValueAttribute(0.45f), new ezClampValueAttribute(0.0f, 0.99f)),
    EZ_MEMBER_PROPERTY("TurnSpeed", m_TurnSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(250.0f))),
    EZ_MEMBER_PROPERTY("FireInterval", m_FireInterval)->AddAttributes(new ezDefaultValueAttribute(ezTime::MakeFromSeconds(0.16))),
    EZ_MEMBER_PROPERTY("TripleShotDuration", m_TripleShotDuration)->AddAttributes(new ezDefaultValueAttribute(ezTime::MakeFromSeconds(10))),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Asteroids"),
    new ezSphereVisualizerAttribute("Radius", ezColor::LimeGreen),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ShipComponent::ShipComponent() = default;
ShipComponent::~ShipComponent() = default;

void ShipComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hProjectilePrefab;
  s << m_hExplosionPrefab;
  s << m_sFireSound;
  s << m_sShieldHitSound;
  s << m_fRadius;
  s << m_fThrust;
  s << m_fMaxSpeed;
  s << m_fDrag;
  s << m_TurnSpeed;
  s << m_FireInterval;
  s << m_TripleShotDuration;
}

void ShipComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_hProjectilePrefab;
  s >> m_hExplosionPrefab;
  s >> m_sFireSound;
  s >> m_sShieldHitSound;
  s >> m_fRadius;
  s >> m_fThrust;
  s >> m_fMaxSpeed;
  s >> m_fDrag;
  s >> m_TurnSpeed;
  s >> m_FireInterval;
  s >> m_TripleShotDuration;
}

void ShipComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (ezGameObject* pVisual = GetOwner()->FindChildByName(ezTempHashedString("Visual")))
  {
    m_hVisual = pVisual->GetHandle();
    m_qVisualBaseRotation = pVisual->GetLocalRotation();
  }

  if (ezGameObject* pThrust = GetOwner()->FindChildByName(ezTempHashedString("Thrust")))
  {
    m_hThrust = pThrust->GetHandle();
  }

  if (ezGameObject* pShield = GetOwner()->FindChildByName(ezTempHashedString("Shield")))
  {
    m_hShield = pShield->GetHandle();
  }

  // make sure firing the first shot or exploding doesn't cause a hiccup
  if (m_hProjectilePrefab.IsValid())
    ezResourceManager::PreloadResource(m_hProjectilePrefab);

  if (m_hExplosionPrefab.IsValid())
    ezResourceManager::PreloadResource(m_hExplosionPrefab);
}

bool ShipComponent::Hit()
{
  if (!m_bHasShield)
    return true;

  // the shield absorbs the hit and also gives a moment to fly to safety
  m_bHasShield = false;
  m_InvulnerableTime = ezTime::MakeFromSeconds(s_fShieldHitInvulnerability);

  ezGameObject* pShield = nullptr;
  if (GetWorld()->TryGetObject(m_hShield, pShield))
    pShield->SetActiveFlag(false);

  PlaySound(m_sShieldHitSound);
  return false;
}

void ShipComponent::GiveShield()
{
  m_bHasShield = true;

  ezGameObject* pShield = nullptr;
  if (GetWorld()->TryGetObject(m_hShield, pShield))
    pShield->SetActiveFlag(true);
}

void ShipComponent::GiveTripleShot()
{
  m_TripleShotTime = m_TripleShotDuration;
}

void ShipComponent::Update()
{
  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  const float fTimeDiff = tDiff.AsFloatInSeconds();

  m_FireCooldown = ezMath::Max(m_FireCooldown - tDiff, ezTime::MakeZero());
  m_InvulnerableTime = ezMath::Max(m_InvulnerableTime - tDiff, ezTime::MakeZero());
  m_TripleShotTime = ezMath::Max(m_TripleShotTime - tDiff, ezTime::MakeZero());

  // the actions are bound to keys and controller sticks in RuntimeConfigs/InputConfig.ddl
  // the input value is proportional for sticks and triggers, and either 0 or 1 for keys
  float fTurnLeft = 0.0f;
  float fTurnRight = 0.0f;
  float fThrust = 0.0f;
  ezInputManager::GetInputActionState("Game", "TurnLeft", &fTurnLeft);
  ezInputManager::GetInputActionState("Game", "TurnRight", &fTurnRight);
  ezInputManager::GetInputActionState("Game", "Thrust", &fThrust);

  // seen from the camera above the playfield, a positive rotation around Z is clockwise
  const float fTurn = ezMath::Clamp(fTurnRight - fTurnLeft, -1.0f, 1.0f);

  Fly(fTimeDiff, fTurn, ezMath::Clamp(fThrust, 0.0f, 1.0f));
  UpdateVisuals(fTimeDiff, fTurn);
  UpdateThrustEffect(fTimeDiff, fThrust > 0.1f);

  if (ezInputManager::GetInputActionState("Game", "Fire") != ezKeyState::Up && m_FireCooldown.IsZeroOrNegative())
  {
    FireProjectiles();
    m_FireCooldown = m_FireInterval;
  }
}

void ShipComponent::Fly(float fTimeDiff, float fTurn, float fThrust)
{
  ezGameObject* pOwner = GetOwner();

  const ezQuat qTurn = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), m_TurnSpeed * fTurn * fTimeDiff);
  pOwner->SetLocalRotation(qTurn * pOwner->GetLocalRotation());

  // the ship's local X axis is its forward direction
  const ezVec3 vForward = pOwner->GetLocalRotation() * ezVec3(1, 0, 0);

  m_vVelocity += vForward * m_fThrust * fThrust * fTimeDiff;

  // a little bit of drag keeps the ship controllable, without losing the feeling of inertia
  m_vVelocity *= ezMath::Pow(1.0f - m_fDrag, fTimeDiff);

  const float fSpeed = m_vVelocity.GetLength();
  if (fSpeed > m_fMaxSpeed)
    m_vVelocity *= m_fMaxSpeed / fSpeed;

  AsteroidsGameComponent::MoveAndWrap(pOwner, m_vVelocity, m_fRadius);
}

void ShipComponent::UpdateVisuals(float fTimeDiff, float fTurn)
{
  ezGameObject* pVisual = nullptr;
  if (!GetWorld()->TryGetObject(m_hVisual, pVisual))
    return;

  // lean into the turn, this is only a visual effect
  const float fTargetBank = fTurn * -s_fMaxBankDegrees;
  m_fBank = ezMath::Lerp(m_fBank, fTargetBank, ezMath::Min(1.0f, fTimeDiff * s_fBankSpeed));

  const ezQuat qBank = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(m_fBank));
  pVisual->SetLocalRotation(qBank * m_qVisualBaseRotation);

  // blink while invulnerable
  const bool bBlinkedOff = IsInvulnerable() && ezMath::Mod(m_InvulnerableTime.AsFloatInSeconds(), s_fBlinkPeriod) < s_fBlinkPeriod * 0.5f;
  pVisual->SetActiveFlag(!bBlinkedOff);
}

void ShipComponent::UpdateThrustEffect(float fTimeDiff, bool bThrusting)
{
  ezGameObject* pThrust = nullptr;
  if (!GetWorld()->TryGetObject(m_hThrust, pThrust))
    return;

  if (m_bThrusting != bThrusting)
  {
    m_bThrusting = bThrusting;

    // the particle component reacts to this message, so the ship doesn't need to know what the effect is made of
    ezMsgSetPlaying msg;
    msg.m_bPlay = bThrusting;
    pThrust->SendMessage(msg);
  }

  // the engine sound loops all the time, it is just faded in and out
  m_fThrustVolume = ezMath::Clamp(m_fThrustVolume + (bThrusting ? s_fThrustVolumeFadeIn : -s_fThrustVolumeFadeOut) * fTimeDiff, 0.0f, 1.0f);

  ezMiniAudioSoundComponent* pSound = nullptr;
  if (pThrust->TryGetComponentOfBaseType(pSound))
    pSound->SetVolume(m_fThrustVolume);
}

void ShipComponent::FireProjectiles()
{
  SpawnProjectile(ezAngle::MakeZero());

  if (m_TripleShotTime.IsPositive())
  {
    SpawnProjectile(ezAngle::MakeFromDegree(s_fTripleShotSpreadDegrees));
    SpawnProjectile(ezAngle::MakeFromDegree(-s_fTripleShotSpreadDegrees));
  }

  PlaySound(m_sFireSound, (float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.95, 1.05));
}

void ShipComponent::SpawnProjectile(ezAngle yaw)
{
  const ezQuat qRotation = GetOwner()->GetGlobalRotation() * ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), yaw);

  const ezVec3 vPosition = GetOwner()->GetGlobalPosition() + qRotation * ezVec3(m_fRadius, 0, 0);

  ezGameObject* pObject = AsteroidsGameComponent::SpawnPrefab(GetWorld(), m_hProjectilePrefab, ezTransform(vPosition, qRotation));

  ProjectileComponent* pProjectile = nullptr;
  if (pObject != nullptr && pObject->TryGetComponentOfBaseType(pProjectile))
  {
    // bullets inherit the ship's momentum, otherwise they would look slow when flying forward at full speed
    pProjectile->m_vVelocity = qRotation * ezVec3(pProjectile->m_fSpeed, 0, 0) + m_vVelocity;
  }
}

void ShipComponent::PlaySound(const ezString& sSound, float fPitch)
{
  if (!sSound.IsEmpty())
  {
    ezSoundInterface::PlaySound(GetWorld(), sSound, GetOwner()->GetGlobalTransform(), fPitch, 1.0f, false).IgnoreResult();
  }
}
