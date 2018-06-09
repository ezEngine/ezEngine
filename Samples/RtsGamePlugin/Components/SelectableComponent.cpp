#include <PCH.h>
#include <RtsGamePlugin/Components/SelectableComponent.h>

EZ_BEGIN_COMPONENT_TYPE(RtsSelectableComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SelectionRadius", m_fSelectionRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.1f, 10.0f)),
  }
  EZ_END_PROPERTIES

    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("RTS Sample"),
  }
  EZ_END_ATTRIBUTES
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

