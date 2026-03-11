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

/// \brief Used to apply damage to objects in the vicinity and push physical objects away.
///
/// The component queries for dynamic physics shapes within a given radius.
/// For all objects found it sends the messages ezMsgPhysicsAddImpulse and ezMsgDamage.
/// The former is used to apply a physical impulse, to push the objects away from the center of the explosion.
/// The second message is used to apply damage to the objects. This only has an effect, if those objects
/// handle that message type.///
///
/// This component is mainly meant as an example how to make gameplay functionality, such as explosions.
/// If its functionality is insufficient for your use-case, write your own and take its code as inspiration.
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

  void ApplyAreaDamage();           // [ scriptable ]

  bool m_bTriggerOnCreation = true; // [ property ]
  ezUInt8 m_uiCollisionLayer = 0;   // [ property ]
  float m_fRadius = 5.0f;           // [ property ]
  float m_fDamage = 10.0f;          // [ property ]
  ezUInt8 m_uiImpulseType = 0;      // [ property ]
  float m_fImpulse = 100.0f;        // [ property ]
};
