#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/Components/TransformComponent.h>

typedef ezComponentManagerSimple<class ezSliderComponent, ezComponentUpdateType::WhenSimulating> ezSliderComponentManager;

class EZ_GAMEENGINE_DLL ezSliderComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSliderComponent, ezTransformComponent, ezSliderComponentManager);

public:
  ezSliderComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

public:
  float m_fDistanceToTravel;
  float m_fAcceleration;
  float m_fDeceleration;
  ezEnum<ezBasisAxis> m_Axis;

private:
  float m_fLastDistance;
};


