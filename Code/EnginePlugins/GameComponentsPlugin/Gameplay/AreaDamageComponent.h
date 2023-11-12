#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

class ezPhysicsWorldModuleInterface;
struct ezPhysicsOverlapResult;

class EZ_GAMECOMPONENTS_DLL ezAreaDamageComponentManager : public ezComponentManager<class ezAreaDamageComponent, ezBlockStorageType::FreeList>
{
  using SUPER = ezComponentManager<ezAreaDamageComponent, ezBlockStorageType::FreeList>;

public:
  ezAreaDamageComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;

private:
  friend class ezAreaDamageComponent;
  ezPhysicsWorldModuleInterface* m_pPhysicsInterface = nullptr;
};

class EZ_GAMECOMPONENTS_DLL ezAreaDamageComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAreaDamageComponent, ezComponent, ezAreaDamageComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezAreaDamageComponent

public:
  ezAreaDamageComponent();
  ~ezAreaDamageComponent();

  void ApplyAreaDamage(); // [ scriptable ]

  bool m_bTriggerOnCreation = true; // [ property ]
  ezUInt8 m_uiCollisionLayer = 0;   // [ property ]
  float m_fRadius = 5.0f;           // [ property ]
  float m_fDamage = 10.0f;          // [ property ]
  float m_fImpulse = 100.0f;        // [ property ]
};
