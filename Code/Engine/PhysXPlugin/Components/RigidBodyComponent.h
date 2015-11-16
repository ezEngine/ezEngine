#pragma once

#include <PhysXPlugin/Components/PhysXComponent.h>

typedef ezComponentManagerSimple<class ezPxRigidBodyComponent, true> ezPxRigidBodyComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxRigidBodyComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxRigidBodyComponent, ezPhysXComponent, ezPxRigidBodyComponentManager);

public:
  ezPxRigidBodyComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion) override;

  // ************************************* PROPERTIES ***********************************
public:


protected:


  // ************************************* FUNCTIONS *****************************

public:
  void Update();

  virtual ezResult Initialize() override;

  virtual ezResult Deinitialize() override;

protected:
  PxRigidDynamic* m_pActor;
};
