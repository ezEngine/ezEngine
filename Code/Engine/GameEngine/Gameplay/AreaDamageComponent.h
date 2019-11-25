#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

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

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezAreaDamageComponent

public:
  ezAreaDamageComponent();

  void ApplyAreaDamage(); // [ scriptable ]

  bool m_bTriggerOnCreation = true; // [ property ]
  ezUInt8 m_uiCollisionLayer = 0;   // [ property ]
  float m_fRadius = 5.0f;           // [ property ]
  float m_fDamage = 10.0f;          // [ property ]
  float m_fImpulse = 100.0f;        // [ property ]
};
