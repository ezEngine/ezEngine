#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>

namespace
{
  ezSpatialData::Category s_ProcVolumeCategory = ezSpatialData::RegisterCategory("ProcVolume", ezSpatialData::Flags::None);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezProcVolumeComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Value", GetValue, SetValue)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new ezClampValueAttribute(-64.0f, 64.0f)),
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

ezEvent<const ezProcGenInternal::InvalidatedArea&> ezProcVolumeComponent::s_AreaInvalidatedEvent;

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

  if (GetUniqueID() != ezInvalidIndex)
  {
    // Only necessary in Editor
    ezBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
    if (globalBounds.IsValid())
    {
      InvalidateArea(globalBounds.GetBox());
    }
  }

  // Don't disable notifications as other components attached to the owner game object might need them too.
  // GetOwner()->DisableStaticTransformChangesNotifications();

  GetOwner()->UpdateLocalBounds();
}

void ezProcVolumeComponent::SetValue(float fValue)
{
  if (m_fValue != fValue)
  {
    m_fValue = fValue;

    InvalidateArea();
  }
}

void ezProcVolumeComponent::SetSortOrder(float fOrder)
{
  if (m_fSortOrder != fOrder)
  {
    m_fSortOrder = fOrder;

    InvalidateArea();
  }
}

void ezProcVolumeComponent::SetBlendMode(ezEnum<ezProcGenBlendMode> blendMode)
{
  if (m_BlendMode != blendMode)
  {
    m_BlendMode = blendMode;

    InvalidateArea();
  }
}

void ezProcVolumeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_fValue;
  s << m_fSortOrder;
  s << m_BlendMode;
}

void ezProcVolumeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_fValue;
  s >> m_fSortOrder;
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
  if (!IsActiveAndInitialized())
    return;

  ezBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
  if (globalBounds.IsValid())
  {
    InvalidateArea(globalBounds.GetBox());
  }
}

void ezProcVolumeComponent::InvalidateArea(const ezBoundingBox& box)
{
  ezProcGenInternal::InvalidatedArea area;
  area.m_Box = box;
  area.m_pWorld = GetWorld();

  s_AreaInvalidatedEvent.Broadcast(area);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezProcVolumeSphereComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractVolumes, OnExtractVolumes)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Procedural Generation"),
    new ezSphereManipulatorAttribute("Radius"),
    new ezSphereVisualizerAttribute("Radius", ezColor::LimeGreen),
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
    }

    InvalidateArea();
  }
}

void ezProcVolumeSphereComponent::SetFadeOutStart(float fFadeOutStart)
{
  if (m_fFadeOutStart != fFadeOutStart)
  {
    m_fFadeOutStart = fFadeOutStart;

    InvalidateArea();
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

void ezProcVolumeSphereComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingSphere(ezVec3::ZeroVector(), m_fRadius), s_ProcVolumeCategory);
}

void ezProcVolumeSphereComponent::OnExtractVolumes(ezMsgExtractVolumes& msg) const
{
  msg.m_pCollection->AddSphere(GetOwner()->GetGlobalTransformSimd(), m_fRadius, m_BlendMode, m_fSortOrder, m_fValue, m_fFadeOutStart);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezProcVolumeBoxComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(10.0f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
    EZ_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0.5f)), new ezClampValueAttribute(ezVec3(0.0f), ezVec3(1.0f))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractVolumes, OnExtractVolumes)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Procedural Generation"),
    new ezBoxManipulatorAttribute("Extents"),
    new ezBoxVisualizerAttribute("Extents", 1.0f, ezColor::LimeGreen),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezProcVolumeBoxComponent::ezProcVolumeBoxComponent() = default;
ezProcVolumeBoxComponent::~ezProcVolumeBoxComponent() = default;

void ezProcVolumeBoxComponent::SetExtents(const ezVec3& extents)
{
  if (m_vExtents != extents)
  {
    m_vExtents = extents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }

    InvalidateArea();
  }
}

void ezProcVolumeBoxComponent::SetFadeOutStart(const ezVec3& fadeOutStart)
{
  if (m_vFadeOutStart != fadeOutStart)
  {
    m_vFadeOutStart = fadeOutStart;

    InvalidateArea();
  }
}

void ezProcVolumeBoxComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_vExtents;
  s << m_vFadeOutStart;
}

void ezProcVolumeBoxComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_vExtents;
  s >> m_vFadeOutStart;
}

void ezProcVolumeBoxComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), s_ProcVolumeCategory);
}

void ezProcVolumeBoxComponent::OnExtractVolumes(ezMsgExtractVolumes& msg) const
{
  msg.m_pCollection->AddBox(GetOwner()->GetGlobalTransformSimd(), m_vExtents, m_BlendMode, m_fSortOrder, m_fValue, m_vFadeOutStart);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezProcVolumeImageComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Image", GetImageFile, SetImageFile)->AddAttributes(new ezAssetBrowserAttribute("Image Data")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractVolumes, OnExtractVolumes)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezProcVolumeImageComponent::ezProcVolumeImageComponent() = default;
ezProcVolumeImageComponent::~ezProcVolumeImageComponent() = default;

void ezProcVolumeImageComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_Image;
}

void ezProcVolumeImageComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_Image;
}

void ezProcVolumeImageComponent::OnExtractVolumes(ezMsgExtractVolumes& msg) const
{
  msg.m_pCollection->AddImage(GetOwner()->GetGlobalTransformSimd(), m_vExtents, m_BlendMode, m_fSortOrder, m_fValue, m_vFadeOutStart, m_Image);
}

void ezProcVolumeImageComponent::SetImageFile(const char* szFile)
{
  ezImageDataResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezImageDataResource>(szFile);
  }

  SetImage(hResource);
}

const char* ezProcVolumeImageComponent::GetImageFile() const
{
  if (!m_Image.IsValid())
    return "";

  return m_Image.GetResourceID();
}

void ezProcVolumeImageComponent::SetImage(const ezImageDataResourceHandle& hResource)
{
  m_Image = hResource;
}
