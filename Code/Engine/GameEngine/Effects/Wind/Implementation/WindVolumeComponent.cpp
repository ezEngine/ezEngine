#include <GameEnginePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Intersection.h>
#include <GameEngine/Effects/Wind/WindVolumeComponent.h>

ezSpatialData::Category ezWindVolumeComponent::SpatialDataCategory = ezSpatialData::RegisterCategory("WindVolumes");

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezWindVolumeComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("BurstDuration", m_BurstDuration),
    EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction, m_OnFinishedAction),
    EZ_ENUM_MEMBER_PROPERTY("Strength", ezWindStrength, m_Strength),
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
    new ezCategoryAttribute("Effects"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezWindVolumeComponent::ezWindVolumeComponent() = default;
ezWindVolumeComponent::~ezWindVolumeComponent() = default;

void ezWindVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void ezWindVolumeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_BurstDuration.IsPositive())
  {
    ezMsgComponentInternalTrigger msg;
    msg.m_sMessage.Assign("Suicide");

    PostMessage(msg, m_BurstDuration);
  }
}

void ezWindVolumeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_BurstDuration;
  s << m_OnFinishedAction;
  s << m_Strength;
}

void ezWindVolumeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_BurstDuration;
  s >> m_OnFinishedAction;
  s >> m_Strength;
}

ezSimdVec4f ezWindVolumeComponent::ComputeForceAtGlobalPosition(const ezSimdVec4f& globalPos) const
{
  const ezSimdTransform t = GetOwner()->GetGlobalTransformSimd();
  const ezSimdTransform tInv = t.GetInverse();
  const ezSimdVec4f localPos = tInv.TransformPosition(globalPos);

  const ezSimdVec4f force = ComputeForceAtLocalPosition(localPos);

  return t.TransformDirection(force);
}

void ezWindVolumeComponent::OnTriggered(ezMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != ezTempHashedString("Suicide"))
    return;

  ezOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);
}

void ezWindVolumeComponent::OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg)
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
EZ_BEGIN_COMPONENT_TYPE(ezWindVolumeSphereComponent, 1, ezComponentMode::Static)
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
    new ezSphereVisualizerAttribute("Radius", ezColor::CornflowerBlue),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezWindVolumeSphereComponent::ezWindVolumeSphereComponent() = default;
ezWindVolumeSphereComponent::~ezWindVolumeSphereComponent() = default;

void ezWindVolumeSphereComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fRadius;
}

void ezWindVolumeSphereComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fRadius;
  m_OneDivRadius = 1.0f / m_fRadius;
}

ezSimdVec4f ezWindVolumeSphereComponent::ComputeForceAtLocalPosition(const ezSimdVec4f& localPos) const
{
  // TODO: could do this computation in global space

  ezSimdFloat lenScaled = localPos.GetLength<3>() * m_OneDivRadius;

  // inverse quadratic falloff to have sharper edges
  ezSimdFloat forceFactor = ezSimdFloat(1.0f) - (lenScaled * lenScaled);

  const ezSimdFloat force = ezWindStrength::GetInMetersPerSecond(m_Strength) * forceFactor.Max(0.0f);

  ezSimdVec4f dir = localPos;
  dir.NormalizeIfNotZero<3>();

  return dir * force;
}

void ezWindVolumeSphereComponent::SetRadius(float val)
{
  m_fRadius = ezMath::Max(val, 0.1f);
  m_OneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezWindVolumeSphereComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(ezBoundingSphere(ezVec3::ZeroVector(), m_fRadius), ezWindVolumeComponent::SpatialDataCategory);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezWindVolumeCylinderMode, 1)
  EZ_ENUM_CONSTANTS(ezWindVolumeCylinderMode::Directional, ezWindVolumeCylinderMode::Vortex)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezWindVolumeCylinderComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.1f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, ezVariant())),
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezWindVolumeCylinderMode, m_Mode),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCylinderVisualizerAttribute(ezBasisAxis::PositiveX, "Length", "Radius", ezColor::CornflowerBlue),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 1.0f, ezColor::DeepSkyBlue),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezWindVolumeCylinderComponent::ezWindVolumeCylinderComponent() = default;
ezWindVolumeCylinderComponent::~ezWindVolumeCylinderComponent() = default;

void ezWindVolumeCylinderComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fRadius;
  s << m_fLength;
  s << m_Mode;
}

void ezWindVolumeCylinderComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fRadius;
  m_OneDivRadius = 1.0f / m_fRadius;

  s >> m_fLength;
  s >> m_Mode;
}

ezSimdVec4f ezWindVolumeCylinderComponent::ComputeForceAtLocalPosition(const ezSimdVec4f& localPos) const
{
  const ezSimdFloat fCylDist = localPos.x();

  if (fCylDist <= -m_fLength * 0.5f || fCylDist >= m_fLength * 0.5f)
    return ezSimdVec4f::ZeroVector();

  ezSimdVec4f orthoDir = localPos;
  orthoDir.SetX(0.0f);

  if (orthoDir.GetLengthSquared<3>() >= ezMath::Square(m_fRadius))
    return ezSimdVec4f::ZeroVector();

  if (m_Mode == ezWindVolumeCylinderMode::Vortex)
  {
    ezSimdVec4f forceDir = ezSimdVec4f(1, 0, 0, 0).CrossRH(orthoDir);
    forceDir.NormalizeIfNotZero<3>();
    return forceDir * ezWindStrength::GetInMetersPerSecond(m_Strength);
  }

  return ezSimdVec4f(ezWindStrength::GetInMetersPerSecond(m_Strength), 0, 0);
}

void ezWindVolumeCylinderComponent::SetRadius(float val)
{
  m_fRadius = ezMath::Max(val, 0.1f);
  m_OneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezWindVolumeCylinderComponent::SetLength(float val)
{
  m_fLength = ezMath::Max(val, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezWindVolumeCylinderComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  const ezVec3 corner(m_fLength * 0.5f, m_fRadius, m_fRadius);

  msg.AddBounds(ezBoundingBox(-corner, corner), ezWindVolumeComponent::SpatialDataCategory);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezWindVolumeConeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Angle", GetAngle, SetAngle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(45)), new ezClampValueAttribute(ezAngle::Degree(1), ezAngle::Degree(179))),
    EZ_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezConeVisualizerAttribute(ezBasisAxis::PositiveX, "Angle", 1.0f, "Length", ezColor::CornflowerBlue),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezWindVolumeConeComponent::ezWindVolumeConeComponent() = default;
ezWindVolumeConeComponent::~ezWindVolumeConeComponent() = default;

void ezWindVolumeConeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fLength;
  s << m_Angle;
}

void ezWindVolumeConeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fLength;
  s >> m_Angle;
}

ezSimdVec4f ezWindVolumeConeComponent::ComputeForceAtLocalPosition(const ezSimdVec4f& localPos) const
{
  const ezSimdFloat fConeDist = localPos.x();

  if (fConeDist <= ezSimdFloat::Zero() || fConeDist >= m_fLength)
    return ezSimdVec4f::ZeroVector();

  // TODO: precompute base radius
  const float fBaseRadius = ezMath::Tan(m_Angle * 0.5f) * m_fLength;

  // TODO: precompute 1/length
  const ezSimdFloat fConeRadius = (fConeDist / ezSimdFloat(m_fLength)) * ezSimdFloat(fBaseRadius);

  ezSimdVec4f orthoDir = localPos;
  orthoDir.SetX(0.0f);

  if (orthoDir.GetLengthSquared<3>() >= fConeRadius * fConeRadius)
    return ezSimdVec4f::ZeroVector();

  return localPos.GetNormalized<3>() * ezWindStrength::GetInMetersPerSecond(m_Strength);
}

void ezWindVolumeConeComponent::SetLength(float val)
{
  m_fLength = ezMath::Max(val, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezWindVolumeConeComponent::SetAngle(ezAngle val)
{
  m_Angle = ezMath::Max(val, ezAngle::Degree(1.0f));

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezWindVolumeConeComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  ezVec3 c0, c1;
  c0.x = 0;
  c0.y = -ezMath::Tan(m_Angle * 0.5f) * m_fLength;
  c0.z = c0.y;

  c1.x = m_fLength;
  c1.y = ezMath::Tan(m_Angle * 0.5f) * m_fLength;
  c1.z = c1.y;

  msg.AddBounds(ezBoundingBox(c0, c1), ezWindVolumeComponent::SpatialDataCategory);
}
