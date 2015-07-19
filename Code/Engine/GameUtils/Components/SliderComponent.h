#pragma once

#include <GameUtils/Basics.h>
#include <GameUtils/Components/TransformComponent.h>

class ezSliderComponent;
typedef ezComponentManagerSimple<ezSliderComponent> ezSliderComponentManager;

class EZ_GAMEUTILS_DLL ezSliderComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSliderComponent, ezSliderComponentManager);

public:
  ezSliderComponent();

  void Update();

  // ************************************* PROPERTIES ***********************************

public:
  float m_fDistanceToTravel;
  float m_fAcceleration;
  float m_fDeceleration;
  ezEnum<ezBasisAxis> m_Axis;

private:
  float m_fLastDistance;
};


