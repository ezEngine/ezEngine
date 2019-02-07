#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>

class ezPhysicsWorldModuleInterface;
struct ezPhysicsOverlapResult;

class EZ_GAMEENGINE_DLL ezAreaDamageComponentManager : public ezComponentManager<class ezAreaDamageComponent, ezBlockStorageType::FreeList>
{
  typedef ezComponentManager<ezAreaDamageComponent, ezBlockStorageType::FreeList> SUPER;

public:
  ezAreaDamageComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);

private:
  friend class ezAreaDamageComponent;
  ezPhysicsWorldModuleInterface* m_pPhysicsInterface;
};

class EZ_GAMEENGINE_DLL ezAreaDamageComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAreaDamageComponent, ezComponent, ezAreaDamageComponentManager);

public:
  ezAreaDamageComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void ApplyAreaDamage();

  // ************************************* PROPERTIES ***********************************

  bool m_bTriggerOnCreation = true;
  ezUInt8 m_uiCollisionLayer = 0;
  float m_fRadius = 5.0f;
  float m_fDamage = 10.0f;
  float m_fImpulse = 100.0f;

protected:
  virtual void OnSimulationStarted() override;
};


