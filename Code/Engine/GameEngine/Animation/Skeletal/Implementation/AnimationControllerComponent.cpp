#include <GameEnginePCH.h>

#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezAnimationControllerComponent, 1);
{
  //EZ_BEGIN_PROPERTIES
  //{
  //}
  //EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezAnimationControllerComponent::ezAnimationControllerComponent() = default;
ezAnimationControllerComponent::~ezAnimationControllerComponent() = default;


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimationControllerComponent);
