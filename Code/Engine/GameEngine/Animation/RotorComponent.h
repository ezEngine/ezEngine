#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

typedef ezComponentManagerSimple<class ezRotorComponent, ezComponentUpdateType::WhenSimulating> ezRotorComponentManager;

class EZ_GAMEENGINE_DLL ezRotorComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRotorComponent, ezTransformComponent, ezRotorComponentManager);

public:
  ezRotorComponent();

  void Update();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface

protected:
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

public:
  ezInt32 m_iDegreeToRotate;
  float m_fAcceleration;
  float m_fDeceleration;
  ezEnum<ezBasisAxis> m_Axis;
  ezAngle m_AxisDeviation;

private:
  ezVec3 m_vRotationAxis;
  ezQuat m_LastRotation;
};
