#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/XR/StageSpaceComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezStageSpaceComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("StageSpace", ezXRStageSpace, GetStageSpace, SetStageSpace)->AddAttributes(new ezDefaultValueAttribute((ezInt32)ezXRStageSpace::Enum::Standing)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("XR"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezStageSpaceComponent::ezStageSpaceComponent() = default;
ezStageSpaceComponent::~ezStageSpaceComponent() = default;

ezEnum<ezXRStageSpace> ezStageSpaceComponent::GetStageSpace() const
{
  return m_space;
}

void ezStageSpaceComponent::SetStageSpace(ezEnum<ezXRStageSpace> space)
{
  m_space = space;
}

void ezStageSpaceComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_space;
}

void ezStageSpaceComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_space;
}

void ezStageSpaceComponent::OnActivated() {}

void ezStageSpaceComponent::OnDeactivated() {}

EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_StageSpaceComponent);
