#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RecastPlugin/Components/NpcComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezNpcComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Experimental"),
    new ezInDevelopmentAttribute(ezInDevelopmentAttribute::Phase::Alpha),
    new ezColorAttribute(ezColorScheme::GetGroupColor(ezColorScheme::Ai)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezNpcComponent::ezNpcComponent() = default;
ezNpcComponent::~ezNpcComponent() = default;

void ezNpcComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
}

void ezNpcComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_NpcComponent);
