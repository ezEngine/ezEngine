#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Effects/Shake/CameraShakeComponent.h>
#include <GameComponentsPlugin/Effects/Shake/CameraShakeVolumeComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezCameraShakeComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MinShake", m_MinShake),
    EZ_MEMBER_PROPERTY("MaxShake", m_MaxShake)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(5))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects/CameraShake"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCameraShakeComponent::ezCameraShakeComponent() = default;
ezCameraShakeComponent::~ezCameraShakeComponent() = default;

void ezCameraShakeComponent::Update()
{
  const ezTime tDuration = ezTime::MakeFromSeconds(1.0 / 30.0); // 30 Hz vibration seems to work well

  // Use the world clock, so that the shake slows down, speeds up and pauses together with the rest of the world.
  // The shake volumes also use the world clock for their burst duration.
  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (tNow >= m_ReferenceTime + tDuration)
  {
    GetOwner()->SetLocalRotation(m_qNextTarget);
    GenerateKeyframe(tNow);
  }
  else
  {
    const float fLerp = ezMath::Clamp((tNow - m_ReferenceTime).AsFloatInSeconds() / tDuration.AsFloatInSeconds(), 0.0f, 1.0f);

    ezQuat q = ezQuat::MakeSlerp(m_qPrevTarget, m_qNextTarget, fLerp);

    GetOwner()->SetLocalRotation(q);
  }
}

void ezCameraShakeComponent::GenerateKeyframe(ezTime tNow)
{
  m_qPrevTarget = m_qNextTarget;

  // fade out shaking over a second, if the vibration stopped
  // use the time since the previous keyframe, not the last frame's time diff, so that the fade does not depend on the frame rate
  m_fLastStrength -= (tNow - m_ReferenceTime).AsFloatInSeconds();

  m_ReferenceTime = tNow;

  ezWorld* pWorld = GetWorld();

  const float fShake = ezMath::Clamp(GetStrengthAtPosition(), 0.0f, 1.0f);

  m_fLastStrength = ezMath::Max(m_fLastStrength, fShake);

  ezAngle deviation;
  deviation = ezMath::Lerp(m_MinShake, m_MaxShake, m_fLastStrength);

  if (deviation > ezAngle())
  {
    m_Rotation += ezAngle::MakeFromRadian(pWorld->GetRandomNumberGenerator().FloatMinMax(ezAngle::MakeFromDegree(120).GetRadian(), ezAngle::MakeFromDegree(240).GetRadian()));
    m_Rotation.NormalizeRange();

    ezQuat qRot;
    qRot = ezQuat::MakeFromAxisAndAngle(ezVec3::MakeAxisX(), m_Rotation);

    const ezVec3 tiltAxis = qRot * ezVec3::MakeAxisZ();

    m_qNextTarget = ezQuat::MakeFromAxisAndAngle(tiltAxis, deviation);
  }
  else
  {
    m_qNextTarget.SetIdentity();
  }
}

float ezCameraShakeComponent::GetStrengthAtPosition() const
{
  float force = 0;

  if (auto pSpatial = GetWorld()->GetSpatialSystem())
  {
    const ezVec3 vPosition = GetOwner()->GetGlobalPosition();

    ezTempHybridArray<ezGameObject*, 16> volumes;

    ezSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = ezCameraShakeVolumeComponent::SpatialDataCategory.GetBitmask();

    pSpatial->FindObjectsInSphere(ezBoundingSphere::MakeFromCenterAndRadius(vPosition, 0.5f), queryParams, volumes);

    const ezSimdVec4f pos = ezSimdConversion::ToVec3(vPosition);

    for (ezGameObject* pObj : volumes)
    {
      ezCameraShakeVolumeComponent* pVol;
      if (pObj->TryGetComponentOfBaseType(pVol))
      {
        force = ezMath::Max(force, pVol->ComputeForceAtGlobalPosition(pos));
      }
    }
  }

  return force;
}

void ezCameraShakeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_MinShake;
  s << m_MaxShake;
}

void ezCameraShakeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_MinShake;
  s >> m_MaxShake;
}


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Effects_Shake_Implementation_CameraShakeComponent);
