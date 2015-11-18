#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>

typedef ezComponentManagerSimple<class ezPxStaticActorComponent, true> ezPxStaticActorComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxStaticActorComponent : public ezPxActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxStaticActorComponent, ezPxActorComponent, ezPxStaticActorComponentManager);
  virtual void ezPhysXComponentIsAbstract() override {}

public:
  ezPxStaticActorComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion) override;

  // ************************************* PROPERTIES ***********************************
public:


protected:


  // ************************************* FUNCTIONS *****************************

public:
  void Update() {}

  virtual void Initialize() override;

  virtual void Deinitialize() override;

private:

  PxRigidStatic* m_pActor;
};
