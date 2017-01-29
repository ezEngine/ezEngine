#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>

typedef ezComponentManager<class ezPxCenterOfMassComponent, ezBlockStorageType::Compact> ezPxCenterOfMassComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxCenterOfMassComponent : public ezPxComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxCenterOfMassComponent, ezPxComponent, ezPxCenterOfMassComponentManager);

public:
  ezPxCenterOfMassComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:


protected:

  // ************************************* FUNCTIONS *****************************

public:

private:

};
