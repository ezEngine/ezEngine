#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/OccluderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezOccluderComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezClampValueAttribute(ezVec3(0.0f), {}), new ezDefaultValueAttribute(ezVec3(1.0f))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTransformChanged, OnMsgTransformChanged),
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractOccluderData, OnMsgExtractOccluderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
    new ezBoxVisualizerAttribute("Extents", 1.0f, ezColorScheme::LightUI(ezColorScheme::Blue)),
    new ezBoxManipulatorAttribute("Extents", 1.0f, true),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezOccluderComponentManager::ezOccluderComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezOccluderComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

ezOccluderComponent::ezOccluderComponent() = default;
ezOccluderComponent::~ezOccluderComponent() = default;

void ezOccluderComponent::SetExtents(const ezVec3& extents)
{
  m_vExtents = extents;
  m_bColliderValid = false;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezOccluderComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  if (GetOwner()->IsStatic())
    msg.AddBounds(ezBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), ezDefaultSpatialDataCategories::OcclusionStatic);
  else
    msg.AddBounds(ezBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), ezDefaultSpatialDataCategories::OcclusionDynamic);
}

void ezOccluderComponent::OnMsgTransformChanged(ezMsgTransformChanged& msg)
{
  m_bColliderValid = false;
}

void ezOccluderComponent::OnMsgExtractOccluderData(ezMsgExtractOccluderData& msg) const
{
  if (IsActiveAndInitialized())
  {
    // if this object is dynamic, we have to check the last transform with which we generated the collider mesh
    if (GetOwner()->IsDynamic() && GetOwner()->GetGlobalTransformSimd() != m_LastGlobalTransform)
    {
      m_bColliderValid = false;
      m_LastGlobalTransform = GetOwner()->GetGlobalTransformSimd();
    }

    if (!m_bColliderValid)
    {
      m_bColliderValid = true;

      m_Object.CreateBox(m_vExtents, GetOwner()->GetGlobalTransform().GetAsMat4());
    }

    msg.AddOccluder(&m_Object, ezTransform::IdentityTransform());
  }
}

void ezOccluderComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_vExtents;
}

void ezOccluderComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_vExtents;
}

void ezOccluderComponent::OnActivated()
{
  // if this is a static object (not usually moving), inform us if the object moves anyway (editor use case)
  GetOwner()->EnableStaticTransformChangesNotifications();

  m_bColliderValid = false;
  GetOwner()->UpdateLocalBounds();
}

void ezOccluderComponent::OnDeactivated()
{
  m_bColliderValid = false;
  m_Object.Clear();
}
