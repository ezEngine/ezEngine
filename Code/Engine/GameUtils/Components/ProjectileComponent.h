#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>

class ezProjectileComponentManager : public ezComponentManagerSimple<class ezProjectileComponent, true>
{
public:
  ezProjectileComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;
};

class EZ_GAMEUTILS_DLL ezProjectileComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProjectileComponent, ezComponent, ezProjectileComponentManager);

public:
  ezProjectileComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  float m_fMetersPerSecond;
  ezUInt8 m_uiCollisionLayer;

private:


};
