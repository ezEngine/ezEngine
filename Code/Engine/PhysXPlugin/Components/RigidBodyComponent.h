#pragma once

#include <PhysXPlugin/Components/PhysXComponent.h>

typedef ezComponentManagerSimple<class ezRigidBodyComponent, true> ezRigidBodyComponentManager;

class EZ_PHYSXPLUGIN_DLL ezRigidBodyComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRigidBodyComponent, ezPhysXComponent, ezRigidBodyComponentManager);

public:
  ezRigidBodyComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion) override;

  // ************************************* PROPERTIES ***********************************
public:


protected:


  // ************************************* FUNCTIONS *****************************

public:
  void Update() {}


};
