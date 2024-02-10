#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Intersection.h>
#include <GameEngine/Effects/Wind/WindVolumeComponent.h>

ezSpatialData::Category ezWindVolumeComponent::SpatialDataCategory = ezSpatialData::RegisterCategory("WindVolumes", ezSpatialData::Flags::None);

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezWindVolumeComponent, 3)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Strength", ezWindStrength, m_Strength),
    EZ_MEMBER_PROPERTY("StrengthFactor", m_fStrengthFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(-10, 10)),
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
    new ezCategoryAttribute("Effects/Wind"),
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

void ezWindVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
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

void ezWindVolumeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_BurstDuration;
  s << m_OnFinishedAction;
  s << m_Strength;
  s << m_fStrengthFactor;
}

void ezWindVolumeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_BurstDuration;
  s >> m_OnFinishedAction;
  s >> m_Strength;

  if (uiVersion == 2)
  {
    bool bReverse = false;
    s >> bReverse;
    m_fStrengthFactor = bReverse ? -1 : 1;
  }

  if (uiVersion >= 3)
  {
    s >> m_fStrengthFactor;
  }
}

ezSimdVec4f ezWindVolumeComponent::ComputeForceAtGlobalPosition(const ezSimdVec4f& vGlobalPos) const
{
  const ezSimdTransform t = GetOwner()->GetGlobalTransformSimd();
  const ezSimdTransform tInv = t.GetInverse();
  const ezSimdVec4f localPos = tInv.TransformPosition(vGlobalPos);

  const ezSimdVec4f force = ComputeForceAtLocalPosition(localPos);

  return t.TransformDirection(force);
}

void ezWindVolumeComponent::OnTriggered(ezMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != ezTempHashedString("Suicide"))
    return;

  ezOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);

  SetActiveFlag(false);
}

void ezWindVolumeComponent::OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg)
{
  if (m_BurstDuration.IsPositive())
  {
    ezOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
  }
}

float ezWindVolumeComponent::GetWindInMetersPerSecond() const
{
  return ezWindStrength::GetInMetersPerSecond(m_Strength) * m_fStrengthFactor;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezWindVolumeComponentPatch_2_3 : public ezGraphPatch
{
public:
  ezWindVolumeComponentPatch_2_3()
    : ezGraphPatch("ezWindVolumeComponent", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto pReverseDirection =  pNode->FindProperty("ReverseDirection");
    if (pReverseDirection && pReverseDirection->m_Value.IsA<bool>())
    {
      float fFactor = pReverseDirection->m_Value.Get<bool>() ? -1 : 1;
      pNode->AddProperty("StrengthFactor", fFactor);
    }
  }
};

ezWindVolumeComponentPatch_2_3 g_ezWindVolumeComponentPatch_2_3;

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

void ezWindVolumeSphereComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
}

void ezWindVolumeSphereComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;
}

ezSimdVec4f ezWindVolumeSphereComponent::ComputeForceAtLocalPosition(const ezSimdVec4f& vLocalPos) const
{
  // TODO: could do this computation in global space

  ezSimdFloat lenScaled = vLocalPos.GetLength<3>() * m_fOneDivRadius;

  // inverse quadratic falloff to have sharper edges
  ezSimdFloat forceFactor = ezSimdFloat(1.0f) - (lenScaled * lenScaled);

  const ezSimdFloat force = GetWindInMetersPerSecond() * forceFactor.Max(0.0f);

  ezSimdVec4f dir = vLocalPos;
  dir.NormalizeIfNotZero<3>();

  return dir * force;
}

void ezWindVolumeSphereComponent::SetRadius(float fVal)
{
  m_fRadius = ezMath::Max(fVal, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezWindVolumeSphereComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), m_fRadius), ezWindVolumeComponent::SpatialDataCategory);
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

void ezWindVolumeCylinderComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fLength;
  s << m_Mode;
}

void ezWindVolumeCylinderComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;

  s >> m_fLength;
  s >> m_Mode;
}

ezSimdVec4f ezWindVolumeCylinderComponent::ComputeForceAtLocalPosition(const ezSimdVec4f& vLocalPos) const
{
  const ezSimdFloat fCylDist = vLocalPos.x();

  if (fCylDist <= -m_fLength * 0.5f || fCylDist >= m_fLength * 0.5f)
    return ezSimdVec4f::MakeZero();

  ezSimdVec4f orthoDir = vLocalPos;
  orthoDir.SetX(0.0f);

  if (orthoDir.GetLengthSquared<3>() >= ezMath::Square(m_fRadius))
    return ezSimdVec4f::MakeZero();

  if (m_Mode == ezWindVolumeCylinderMode::Vortex)
  {
    ezSimdVec4f forceDir = ezSimdVec4f(1, 0, 0, 0).CrossRH(orthoDir);
    forceDir.NormalizeIfNotZero<3>();
    return forceDir * GetWindInMetersPerSecond();
  }

  return ezSimdVec4f(GetWindInMetersPerSecond(), 0, 0);
}

void ezWindVolumeCylinderComponent::SetRadius(float fVal)
{
  m_fRadius = ezMath::Max(fVal, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezWindVolumeCylinderComponent::SetLength(float fVal)
{
  m_fLength = ezMath::Max(fVal, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezWindVolumeCylinderComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  const ezVec3 corner(m_fLength * 0.5f, m_fRadius, m_fRadius);

  msg.AddBounds(ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(-corner, corner)), ezWindVolumeComponent::SpatialDataCategory);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezWindVolumeConeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Angle", GetAngle, SetAngle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(45)), new ezClampValueAttribute(ezAngle::MakeFromDegree(1), ezAngle::MakeFromDegree(179))),
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

void ezWindVolumeConeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fLength;
  s << m_Angle;
}

void ezWindVolumeConeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fLength;
  s >> m_Angle;
}

ezSimdVec4f ezWindVolumeConeComponent::ComputeForceAtLocalPosition(const ezSimdVec4f& vLocalPos) const
{
  const ezSimdFloat fConeDist = vLocalPos.x();

  if (fConeDist <= ezSimdFloat::MakeZero() || fConeDist >= m_fLength)
    return ezSimdVec4f::MakeZero();

  // TODO: precompute base radius
  const float fBaseRadius = ezMath::Tan(m_Angle * 0.5f) * m_fLength;

  // TODO: precompute 1/length
  const ezSimdFloat fConeRadius = (fConeDist / ezSimdFloat(m_fLength)) * ezSimdFloat(fBaseRadius);

  ezSimdVec4f orthoDir = vLocalPos;
  orthoDir.SetX(0.0f);

  if (orthoDir.GetLengthSquared<3>() >= fConeRadius * fConeRadius)
    return ezSimdVec4f::MakeZero();

  return vLocalPos.GetNormalized<3>() * GetWindInMetersPerSecond();
}

void ezWindVolumeConeComponent::SetLength(float fVal)
{
  m_fLength = ezMath::Max(fVal, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezWindVolumeConeComponent::SetAngle(ezAngle val)
{
  m_Angle = ezMath::Max(val, ezAngle::MakeFromDegree(1.0f));

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

  msg.AddBounds(ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(c0, c1)), ezWindVolumeComponent::SpatialDataCategory);
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_WindVolumeComponent);
