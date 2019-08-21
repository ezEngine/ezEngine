#include <ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>

ezSpatialData::Category s_ProcVolumeCategory = ezSpatialData::RegisterCategory("ProcVolume");

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezProcVolumeComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Value", GetValue, SetValue)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ENUM_ACCESSOR_PROPERTY("BlendMode", ezProcGenBlendMode, GetBlendMode, SetBlendMode)->AddAttributes(new ezDefaultValueAttribute(ezProcGenBlendMode::Set)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTransformChanged, OnTransformChanged)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezProcVolumeComponent::ezProcVolumeComponent() = default;
ezProcVolumeComponent::~ezProcVolumeComponent() = default;

void ezProcVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();

  GetOwner()->UpdateLocalBounds();

  if (GetUniqueID() != ezInvalidIndex)
  {
    // Only necessary in Editor
    InvalidateArea();
  }
}

void ezProcVolumeComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  // Don't disable notifications as other components attached to the owner game object might need them too.
  // GetOwner()->DisableStaticTransformChangesNotifications();

  GetOwner()->UpdateLocalBounds();

  if (GetUniqueID() != ezInvalidIndex)
  {
    // Only necessary in Editor
    InvalidateArea();
  }
}

void ezProcVolumeComponent::SetValue(float fValue)
{
  if (m_fValue != fValue)
  {
    m_fValue = fValue;

    if (IsActiveAndInitialized())
    {
      InvalidateArea();
    }
  }
}

void ezProcVolumeComponent::SetBlendMode(ezEnum<ezProcGenBlendMode> blendMode)
{
  if (m_BlendMode != blendMode)
  {
    m_BlendMode = blendMode;

    if (IsActiveAndInitialized())
    {
      InvalidateArea();
    }
  }
}

void ezProcVolumeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_fValue;
  s << m_BlendMode;
}

void ezProcVolumeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_fValue;
  s >> m_BlendMode;
}

void ezProcVolumeComponent::OnTransformChanged(ezMsgTransformChanged& msg)
{
  ezBoundingBoxSphere combined = GetOwner()->GetLocalBounds();
  combined.Transform(msg.m_OldGlobalTransform.GetAsMat4());

  combined.ExpandToInclude(GetOwner()->GetGlobalBounds());

  InvalidateArea(combined.GetBox());
}

void ezProcVolumeComponent::InvalidateArea()
{
  ezBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
  if (globalBounds.IsValid())
  {
    InvalidateArea(globalBounds.GetBox());
  }
}

void ezProcVolumeComponent::InvalidateArea(const ezBoundingBox& area)
{
  // TODO
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezProcVolumeSphereComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new ezDefaultValueAttribute(0.8f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Procedural Generation"),
    new ezSphereManipulatorAttribute("Radius"),
    new ezSphereVisualizerAttribute("Radius", nullptr, ezColor::LimeGreen),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezProcVolumeSphereComponent::ezProcVolumeSphereComponent() = default;
ezProcVolumeSphereComponent::~ezProcVolumeSphereComponent() = default;

void ezProcVolumeSphereComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();

      InvalidateArea();
    }
  }
}

void ezProcVolumeSphereComponent::SetFadeOutStart(float fFadeOutStart)
{
  if (m_fFadeOutStart != fFadeOutStart)
  {
    m_fFadeOutStart = fFadeOutStart;

    if (IsActiveAndInitialized())
    {
      InvalidateArea();
    }
  }
}

void ezProcVolumeSphereComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_fRadius;
  s << m_fFadeOutStart;
}

void ezProcVolumeSphereComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_fRadius;
  s >> m_fFadeOutStart;
}

void ezProcVolumeSphereComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(ezBoundingSphere(ezVec3::ZeroVector(), m_fRadius), s_ProcVolumeCategory);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezProcVolumeBoxComponent, 1, ezComponentMode::Static)
{
  /*EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new ezDefaultValueAttribute(0.8f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;*/
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Procedural Generation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezProcVolumeBoxComponent::ezProcVolumeBoxComponent() = default;
ezProcVolumeBoxComponent::~ezProcVolumeBoxComponent() = default;

void ezProcVolumeBoxComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  //s << m_fRadius;
  //s << m_fFadeOutStart;
}

void ezProcVolumeBoxComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  //s >> m_fRadius;
  //s >> m_fFadeOutStart;
}
