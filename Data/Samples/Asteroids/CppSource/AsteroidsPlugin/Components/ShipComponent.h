#pragma once

#include <Core/World/World.h>

class ShipComponent;
using ShipComponentManager = ezComponentManagerSimple<ShipComponent, ezComponentUpdateType::WhenSimulating>;

class ShipComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ShipComponent, ezComponent, ShipComponentManager);

public:
  ShipComponent();

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override {}

  void Update();

  void AddExternalForce(const ezVec3& vVel);

  void SetIsShooting(bool b);

  bool IsAlive() const { return m_fHealth > 0.0f; }

  float m_fHealth = 0;
  ezInt32 m_iPlayerIndex = -1;

private:
  void Explode();

  ezVec3 m_vExternalForce = ezVec3::MakeZero();
  ezVec3 m_vVelocity = ezVec3::MakeZero();
  bool m_bIsShooting = false;
  ezTime m_CurShootCooldown;
  float m_fAmmunition;
};
