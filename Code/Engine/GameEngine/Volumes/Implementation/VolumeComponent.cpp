#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>
#include <GameEngine/Volumes/VolumeComponent.h>

ezSpatialData::Category s_VolumeCategory = ezSpatialData::RegisterCategory("GenericVolume", ezSpatialData::Flags::None);

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezVolumeComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Template", GetTemplateFile, SetTemplateFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    EZ_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new ezClampValueAttribute(-64.0f, 64.0f)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezVolumeComponent::ezVolumeComponent() = default;
ezVolumeComponent::~ezVolumeComponent() = default;

void ezVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void ezVolumeComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  GetOwner()->UpdateLocalBounds();
}

void ezVolumeComponent::SetTemplateFile(const char* szFile)
{
  ezBlackboardTemplateResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezBlackboardTemplateResource>(szFile);
  }

  SetTemplate(hResource);
}

const char* ezVolumeComponent::GetTemplateFile() const
{
  if (!m_hTemplateResource.IsValid())
    return "";

  return m_hTemplateResource.GetResourceID();
}

void ezVolumeComponent::SetTemplate(const ezBlackboardTemplateResourceHandle& hResource)
{
  m_hTemplateResource = hResource;
}

void ezVolumeComponent::SetSortOrder(float fOrder)
{
  fOrder = ezMath::Clamp(fOrder, -64.0f, 64.0f);
  m_fSortOrder = fOrder;
}

void ezVolumeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_hTemplateResource;
  s << m_fSortOrder;
}

void ezVolumeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_hTemplateResource;
  s >> m_fSortOrder;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVolumeSphereComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereManipulatorAttribute("Radius"),
    new ezSphereVisualizerAttribute("Radius", ezColorScheme::LightUI(ezColorScheme::Cyan)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezVolumeSphereComponent::ezVolumeSphereComponent() = default;
ezVolumeSphereComponent::~ezVolumeSphereComponent() = default;

void ezVolumeSphereComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void ezVolumeSphereComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = fFalloff;
}

void ezVolumeSphereComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
}

void ezVolumeSphereComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
}

void ezVolumeSphereComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(ezBoundingSphere(ezVec3::ZeroVector(), m_fRadius), s_VolumeCategory);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVolumeBoxComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(10.0f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
    EZ_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0.5f)), new ezClampValueAttribute(ezVec3(0.0f), ezVec3(1.0f))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezBoxManipulatorAttribute("Extents", 1.0f, true),
    new ezBoxVisualizerAttribute("Extents", 1.0f, ezColorScheme::LightUI(ezColorScheme::Cyan)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezVolumeBoxComponent::ezVolumeBoxComponent() = default;
ezVolumeBoxComponent::~ezVolumeBoxComponent() = default;

void ezVolumeBoxComponent::SetExtents(const ezVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void ezVolumeBoxComponent::SetFalloff(const ezVec3& vFalloff)
{
  m_vFalloff = vFalloff;
}

void ezVolumeBoxComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vFalloff;
}

void ezVolumeBoxComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vFalloff;
}

void ezVolumeBoxComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(ezBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), s_VolumeCategory);
}
