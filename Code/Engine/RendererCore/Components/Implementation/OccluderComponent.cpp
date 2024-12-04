#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/OccluderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezOccluderType, 1)
  EZ_ENUM_CONSTANTS(ezOccluderType::Box, ezOccluderType::QuadPosX)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezOccluderComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("Type", ezOccluderType, GetType, SetType),
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezClampValueAttribute(ezVec3(0.0f), {}), new ezDefaultValueAttribute(ezVec3(1.0f))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
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

void ezOccluderComponent::SetExtents(const ezVec3& vExtents)
{
  if (m_vExtents == vExtents)
    return;

  m_vExtents = vExtents;
  m_pOccluderObject.Clear();

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezOccluderComponent::SetType(ezEnum<ezOccluderType> type)
{
  if (m_Type == type)
    return;

  m_Type = type;
  m_pOccluderObject.Clear();

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezOccluderComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  if (GetOwner()->IsStatic())
    msg.AddBounds(ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), ezDefaultSpatialDataCategories::OcclusionStatic);
  else
    msg.AddBounds(ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), ezDefaultSpatialDataCategories::OcclusionDynamic);
}

void ezOccluderComponent::OnMsgExtractOccluderData(ezMsgExtractOccluderData& msg) const
{
  if (IsActiveAndInitialized())
  {
    if (m_Type == ezOccluderType::Box)
    {
      if (m_pOccluderObject == nullptr)
      {
        m_pOccluderObject = ezRasterizerObject::CreateBox(m_vExtents);
      }

      msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
    }
    else if (m_Type == ezOccluderType::QuadPosX)
    {
      if (m_pOccluderObject == nullptr)
      {
        m_pOccluderObject = ezRasterizerObject::CreateQuadX(ezVec2(m_vExtents.y, m_vExtents.z));
      }

      msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform() + GetOwner()->GetGlobalRotation() * ezVec3(m_vExtents.x * 0.5f, 0, 0));
    }
  }
}

void ezOccluderComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_Type;
}

void ezOccluderComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;

  if (uiVersion >= 2)
  {
    s >> m_Type;
  }
}

void ezOccluderComponent::OnActivated()
{
  m_pOccluderObject.Clear();
  GetOwner()->UpdateLocalBounds();
}

void ezOccluderComponent::OnDeactivated()
{
  m_pOccluderObject.Clear();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_OccluderComponent);
