#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Components/ShapeIconComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezShapeIconComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Editing Utilities"),
    new ezColorAttribute(ezColorScheme::GetGroupColor(ezColorScheme::Utilities)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezShapeIconComponent::ezShapeIconComponent() = default;
ezShapeIconComponent::~ezShapeIconComponent() = default;
