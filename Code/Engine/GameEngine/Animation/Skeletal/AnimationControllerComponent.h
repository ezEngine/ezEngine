#pragma once

#include <Core/World/Component.h>
#include <GameEngine/GameEngineDLL.h>
#include <Core/World/ComponentManager.h>

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
