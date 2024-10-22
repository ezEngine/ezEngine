#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Intersection.h>
#include <GameComponentsPlugin/Effects/Shake/CameraShakeVolumeComponent.h>

ezSpatialData::Category ezCameraShakeVolumeComponent::SpatialDataCategory = ezSpatialData::RegisterCategory("CameraShakeVolumes", ezSpatialData::Flags::None);

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezCameraShakeVolumeComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Strength", m_fStrength),
    EZ_MEMBER_PROPERTY("BurstDuration", m_BurstDuration),
    EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction, m_OnFinishedAction),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgComponentInternalTrigger, OnTriggered),
    EZ_MESSAGE_HANDLER(ezMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects/CameraShake"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezCameraShakeVolumeComponent::ezCameraShakeVolumeComponent() = default;
ezCameraShakeVolumeComponent::~ezCameraShakeVolumeComponent() = default;

void ezCameraShakeVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void ezCameraShakeVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void ezCameraShakeVolumeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_BurstDuration.IsPositive())
  {
    ezMsgComponentInternalTrigger msg;
    msg.m_sMessage.Assign("Suicide");

    PostMessage(msg, m_BurstDuration);
  }
}

void ezCameraShakeVolumeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_BurstDuration;
  s << m_OnFinishedAction;
  s << m_fStrength;
}

void ezCameraShakeVolumeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_BurstDuration;
  s >> m_OnFinishedAction;
  s >> m_fStrength;
}

float ezCameraShakeVolumeComponent::ComputeForceAtGlobalPosition(const ezSimdVec4f& vGlobalPos) const
{
  const ezSimdTransform t = GetOwner()->GetGlobalTransformSimd();
  const ezSimdTransform tInv = t.GetInverse();
  const ezSimdVec4f localPos = tInv.TransformPosition(vGlobalPos);

  return ComputeForceAtLocalPosition(localPos);
}

void ezCameraShakeVolumeComponent::OnTriggered(ezMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != ezTempHashedString("Suicide"))
    return;

  ezOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);

  SetActiveFlag(false);
}

void ezCameraShakeVolumeComponent::OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg)
{
  if (m_BurstDuration.IsPositive())
  {
    ezOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezCameraShakeVolumeSphereComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereVisualizerAttribute("Radius", ezColor::SaddleBrown),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezCameraShakeVolumeSphereComponent::ezCameraShakeVolumeSphereComponent() = default;
ezCameraShakeVolumeSphereComponent::~ezCameraShakeVolumeSphereComponent() = default;

void ezCameraShakeVolumeSphereComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
}

void ezCameraShakeVolumeSphereComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;
}

float ezCameraShakeVolumeSphereComponent::ComputeForceAtLocalPosition(const ezSimdVec4f& vLocalPos) const
{
  ezSimdFloat lenScaled = vLocalPos.GetLength<3>() * m_fOneDivRadius;

  // inverse quadratic falloff to have sharper edges
  ezSimdFloat forceFactor = ezSimdFloat(1.0f) - lenScaled;

  const ezSimdFloat force = forceFactor.Max(0.0f);

  return m_fStrength * force;
}

void ezCameraShakeVolumeSphereComponent::SetRadius(float fVal)
{
  m_fRadius = ezMath::Max(fVal, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezCameraShakeVolumeSphereComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), m_fRadius), ezCameraShakeVolumeComponent::SpatialDataCategory);
}


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Effects_Shake_Implementation_CameraShakeVolumeComponent);

