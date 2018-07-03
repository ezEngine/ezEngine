#include <PCH.h>
#include <RtsGamePlugin/Components/SelectableComponent.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

EZ_BEGIN_COMPONENT_TYPE(RtsSelectableComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SelectionRadius", m_fSelectionRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.1f, 10.0f)),
  }
  EZ_END_PROPERTIES;
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
    EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("RTS Sample"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE

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

void RtsSelectableComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  ezBoundingBoxSphere bounds;
  bounds.m_fSphereRadius = m_fSelectionRadius;
  bounds.m_vCenter.SetZero();
  bounds.m_vBoxHalfExtends.Set(m_fSelectionRadius);

  msg.AddBounds(bounds);
}
