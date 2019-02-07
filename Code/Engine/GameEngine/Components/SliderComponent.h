#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/Components/TransformComponent.h>

typedef ezComponentManagerSimple<class ezSliderComponent, ezComponentUpdateType::WhenSimulating> ezSliderComponentManager;

class EZ_GAMEENGINE_DLL ezSliderComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSliderComponent, ezTransformComponent, ezSliderComponentManager);

public:
  ezSliderComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface
  // 
public:

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:

  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezSliderComponent interface
  // 

public:
  void Update();

  float m_fDistanceToTravel;
  float m_fAcceleration;
  float m_fDeceleration;
  ezEnum<ezBasisAxis> m_Axis;
  ezTime m_RandomStart;

private:
  float m_fLastDistance;
};


