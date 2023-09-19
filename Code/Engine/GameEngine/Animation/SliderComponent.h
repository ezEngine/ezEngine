#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

using ezSliderComponentManager = ezComponentManagerSimple<class ezSliderComponent, ezComponentUpdateType::WhenSimulating>;

class EZ_GAMEENGINE_DLL ezSliderComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSliderComponent, ezTransformComponent, ezSliderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

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
