#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Components/AiMarkerComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAiMarkerComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Priority", m_fPriority)->AddAttributes(new ezDefaultValueAttribute(0.5f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetSourceObject, In, "Source"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetSourceObject),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Components"),
    new ezSphereVisualizerAttribute("Radius", ezColor::Orange),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAiMarkerComponent::ezAiMarkerComponent() = default;
ezAiMarkerComponent::~ezAiMarkerComponent() = default;

void ezAiMarkerComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fPriority;
}

void ezAiMarkerComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_fPriority;
}

void ezAiMarkerComponent::SetSourceObject(ezGameObjectHandle hSource)
{
  m_hSource = hSource;
}

ezGameObjectHandle ezAiMarkerComponent::GetSourceObject() const
{
  const ezGameObject* pSource = nullptr;
  if (GetWorld()->TryGetObject(m_hSource, pSource))
    return m_hSource;

  pSource = GetOwner()->GetParent();
  return pSource ? pSource->GetHandle() : GetOwner()->GetHandle();
}
