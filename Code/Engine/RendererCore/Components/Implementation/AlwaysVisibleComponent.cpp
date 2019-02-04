#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/AlwaysVisibleComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAlwaysVisibleComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezAlwaysVisibleComponent::ezAlwaysVisibleComponent() = default;
ezAlwaysVisibleComponent::~ezAlwaysVisibleComponent() = default;

void ezAlwaysVisibleComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  //ezStreamWriter& s = stream.GetStream();
}

void ezAlwaysVisibleComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  //ezStreamReader& s = stream.GetStream();
}

ezResult ezAlwaysVisibleComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  bAlwaysVisible = true;
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_AlwaysVisibleComponent);

