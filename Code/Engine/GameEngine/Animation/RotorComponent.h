#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

typedef ezComponentManagerSimple<class ezRotorComponent, ezComponentUpdateType::WhenSimulating> ezRotorComponentManager;

class EZ_GAMEENGINE_DLL ezRotorComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRotorComponent, ezTransformComponent, ezRotorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRotorComponent

public:
  ezRotorComponent();
  ~ezRotorComponent();

  ezInt32 m_iDegreeToRotate = 0;
  float m_fAcceleration = 1.0f;
  float m_fDeceleration = 1.0f;
  ezEnum<ezBasisAxis> m_Axis = ezBasisAxis::PositiveZ;
  ezAngle m_AxisDeviation;

protected:
  void Update();

  ezVec3 m_vRotationAxis = ezVec3(0, 0, 1);
  ezQuat m_LastRotation = ezQuat::IdentityQuaternion();
};
