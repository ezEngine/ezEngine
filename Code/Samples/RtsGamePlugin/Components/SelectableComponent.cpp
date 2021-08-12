#include <RtsGamePlugin/RtsGamePluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <RtsGamePlugin/Components/SelectableComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(RtsSelectableComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SelectionRadius", m_fSelectionRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.1f, 10.0f)),
  }
  EZ_END_PROPERTIES;
  // BEGIN-DOCS-CODE-SNIPPET: spatial-bounds-handler
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  EZ_END_MESSAGEHANDLERS;
  // END-DOCS-CODE-SNIPPET
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("RTS Sample"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

// BEGIN-DOCS-CODE-SNIPPET: spatial-category-registration
ezSpatialData::Category RtsSelectableComponent::s_SelectableCategory = ezSpatialData::RegisterCategory("Selectable", ezSpatialData::Flags::None);
// END-DOCS-CODE-SNIPPET

RtsSelectableComponent::RtsSelectableComponent() = default;
RtsSelectableComponent::~RtsSelectableComponent() = default;

void RtsSelectableComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fSelectionRadius;
}

void RtsSelectableComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fSelectionRadius;
}


void RtsSelectableComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

// BEGIN-DOCS-CODE-SNIPPET: spatial-bounds-update
void RtsSelectableComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  ezBoundingBoxSphere bounds;
  bounds.m_fSphereRadius = m_fSelectionRadius;
  bounds.m_vCenter.SetZero();
  bounds.m_vBoxHalfExtends.Set(m_fSelectionRadius);

  msg.AddBounds(bounds, s_SelectableCategory);
}
// END-DOCS-CODE-SNIPPET
