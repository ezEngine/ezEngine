#pragma once

#include <Core/World/World.h>

class ShipComponent;
typedef ezComponentManagerSimple<ShipComponent, ezComponentUpdateType::WhenSimulating> ShipComponentManager;

class ShipComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ShipComponent, ezComponent, ShipComponentManager);

public:
  ShipComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& stream) override {}

  void Update();

  void SetVelocity(const ezVec3& vVel);

  void SetIsShooting(bool b);

  bool IsAlive() const { return m_fHealth > 0.0f; }

  float m_fHealth;
  ezInt32 m_iPlayerIndex;

private:
  ezVec3 m_vVelocity;
  bool m_bIsShooting;
  ezTime m_CurShootCooldown;
  float m_fAmmunition;
};


