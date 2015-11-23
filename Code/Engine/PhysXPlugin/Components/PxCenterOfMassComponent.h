#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Resources/PhysXMeshResource.h>

typedef ezComponentManager<class ezPxCenterOfMassComponent> ezPxCenterOfMassComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxCenterOfMassComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxCenterOfMassComponent, ezPhysXComponent, ezPxCenterOfMassComponentManager);
  virtual void ezPhysXComponentIsAbstract() override {}

public:
  ezPxCenterOfMassComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion) override;

  // ************************************* PROPERTIES ***********************************
public:


protected:

  // ************************************* FUNCTIONS *****************************

public:

private:

};
