#pragma once

#include <Core/World/ComponentManager.h>

using ezCameraShakeComponentManager = ezComponentManagerSimple<class ezCameraShakeComponent, ezComponentUpdateType::WhenSimulating>;

class EZ_GAMECOMPONENTS_DLL ezCameraShakeComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCameraShakeComponent, ezComponent, ezCameraShakeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezCameraShakeComponent

  ezAngle m_MinShake;
  ezAngle m_MaxShake;

public:
  ezCameraShakeComponent();
  ~ezCameraShakeComponent();

protected:
  void Update();

  void GenerateKeyframe();
  float GetStrengthAtPosition() const;

  float m_fLastStrength = 0.0f;
  ezTime m_ReferenceTime;
  ezAngle m_Rotation;
  ezQuat m_qPrevTarget = ezQuat::IdentityQuaternion();
  ezQuat m_qNextTarget = ezQuat::IdentityQuaternion();
};
