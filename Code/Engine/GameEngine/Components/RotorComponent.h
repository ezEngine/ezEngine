#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/Components/TransformComponent.h>

typedef ezComponentManagerSimple<class ezRotorComponent, ezComponentUpdateType::WhenSimulating> ezRotorComponentManager;

class EZ_GAMEENGINE_DLL ezRotorComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRotorComponent, ezTransformComponent, ezRotorComponentManager);

public:
  ezRotorComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

public:
  ezInt32 m_iDegreeToRotate;
  float m_fAcceleration;
  float m_fDeceleration;
  ezEnum<ezBasisAxis> m_Axis;

private:
  ezQuat m_LastRotation;
};


