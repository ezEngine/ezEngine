#pragma once

#include <GameUtils/Basics.h>
#include <GameUtils/Components/TransformComponent.h>

class ezSliderComponent;
typedef ezComponentManagerSimple<ezSliderComponent, true> ezSliderComponentManager;

class EZ_GAMEUTILS_DLL ezSliderComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSliderComponent, ezTransformComponent, ezSliderComponentManager);

public:
  ezSliderComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion) override;

  // ************************************* PROPERTIES ***********************************

public:
  float m_fDistanceToTravel;
  float m_fAcceleration;
  float m_fDeceleration;
  ezEnum<ezBasisAxis> m_Axis;

private:
  float m_fLastDistance;
};


