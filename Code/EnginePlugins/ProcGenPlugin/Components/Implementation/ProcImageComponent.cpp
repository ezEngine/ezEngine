#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ProcGenPlugin/Components/ProcImageComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>

namespace
{
  ezSpatialData::Category s_ProcImageCategory = ezSpatialData::RegisterCategory("ProcImage", ezSpatialData::Flags::None);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezProcImageComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new ezClampValueAttribute(-64.0f, 64.0f)),
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(10.0f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
    EZ_ACCESSOR_PROPERTY("Image", GetImageFile, SetImageFile)->AddAttributes(new ezAssetBrowserAttribute("Image Data")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractProcImages, OnExtractProcImages)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Procedural Generation"),
    new ezBoxManipulatorAttribute("Extents"),
    new ezBoxVisualizerAttribute("Extents", ezColor::LimeGreen),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezEvent<const ezProcGenInternal::InvalidatedArea&> ezProcImageComponent::s_AreaInvalidatedEvent;

ezProcImageComponent::ezProcImageComponent() = default;
ezProcImageComponent::~ezProcImageComponent() = default;

void ezProcImageComponent::OnActivated()
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

void ezProcImageComponent::OnDeactivated()
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

void ezProcImageComponent::SetExtents(const ezVec3& extents)
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

void ezProcImageComponent::SetSortOrder(float fOrder)
{
  if (m_fSortOrder != fOrder)
  {
    m_fSortOrder = fOrder;

    InvalidateArea();
  }
}

void ezProcImageComponent::SetImageFile(const char* szFile)
{
  ezImageDataResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezImageDataResource>(szFile);
  }

  SetImage(hResource);
}

const char* ezProcImageComponent::GetImageFile() const
{
  if (!m_Image.IsValid())
    return "";

  return m_Image.GetResourceID();
}

void ezProcImageComponent::SetImage(const ezImageDataResourceHandle& hResource)
{
  m_Image = hResource;
}

void ezProcImageComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_vExtents;
  s << m_fSortOrder;
  s << m_Image;
}

void ezProcImageComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_vExtents;
  s >> m_fSortOrder;
  s >> m_Image;
}

void ezProcImageComponent::OnTransformChanged(ezMsgTransformChanged& msg)
{
  ezBoundingBoxSphere combined = GetOwner()->GetLocalBounds();
  combined.Transform(msg.m_OldGlobalTransform.GetAsMat4());

  combined.ExpandToInclude(GetOwner()->GetGlobalBounds());

  InvalidateArea(combined.GetBox());
}

void ezProcImageComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), s_ProcImageCategory);
}

void ezProcImageComponent::OnExtractProcImages(ezMsgExtractProcImages& msg) const
{
  msg.m_pCollection->AddShape(GetOwner()->GetGlobalTransformSimd(), m_vExtents, m_fSortOrder, m_Image);
}

void ezProcImageComponent::InvalidateArea()
{
  if (!IsActiveAndInitialized())
    return;

  ezBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
  if (globalBounds.IsValid())
  {
    InvalidateArea(globalBounds.GetBox());
  }
}

void ezProcImageComponent::InvalidateArea(const ezBoundingBox& box)
{
  ezProcGenInternal::InvalidatedArea area;
  area.m_Box = box;
  area.m_pWorld = GetWorld();

  s_AreaInvalidatedEvent.Broadcast(area);
}
