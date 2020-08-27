#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>

class EZ_GAMEENGINE_DLL ezAnimationControllerComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezAnimationControllerComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent


  //////////////////////////////////////////////////////////////////////////
  // ezAnimationControllerComponent

public:
  ezAnimationControllerComponent();
  ~ezAnimationControllerComponent();
};
