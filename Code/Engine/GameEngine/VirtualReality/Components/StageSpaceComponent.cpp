#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/VirtualReality/Components/StageSpaceComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezStageSpaceComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("StageSpace", ezVRStageSpace, GetStageSpace, SetStageSpace)//->AddAttributes(new ezDefaultValueAttribute((ezInt32)ezVRStageSpace::Enum::Standing)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Virtual Reality"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezStageSpaceComponent::ezStageSpaceComponent()
{
}

ezStageSpaceComponent::~ezStageSpaceComponent() {}


ezEnum<ezVRStageSpace> ezStageSpaceComponent::GetStageSpace() const
{
  return m_space;
}


void ezStageSpaceComponent::SetStageSpace(ezEnum<ezVRStageSpace> space)
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

void ezStageSpaceComponent::OnActivated()
{
}

void ezStageSpaceComponent::OnDeactivated()
{
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_VirtualReality_Components_StageSpaceComponent);

