#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

typedef ezComponentManagerSimple<class ezSliderComponent, ezComponentUpdateType::WhenSimulating> ezSliderComponentManager;

class EZ_GAMEENGINE_DLL ezSliderComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSliderComponent, ezTransformComponent, ezSliderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezSliderComponent

public:
  ezSliderComponent();
  ~ezSliderComponent();

  float m_fDistanceToTravel = 1.0f;                    // [ property ]
  float m_fAcceleration = 0.0f;                        // [ property ]
  float m_fDeceleration = 0.0;                         // [ property ]
  ezEnum<ezBasisAxis> m_Axis = ezBasisAxis::PositiveZ; // [ property ]
  ezTime m_RandomStart;                                // [ property ]

protected:
  void Update();

  float m_fLastDistance = 0.0f;
};
