#pragma once

#include <PhysXPlugin/Components/PhysXComponent.h>

typedef ezComponentManagerSimple<class ezPxStaticMeshComponent, true> ezPxStaticMeshComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxStaticMeshComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxStaticMeshComponent, ezPhysXComponent, ezPxStaticMeshComponentManager);

public:
  ezPxStaticMeshComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion) override;

  // ************************************* PROPERTIES ***********************************
public:


protected:


  // ************************************* FUNCTIONS *****************************

public:
  void Update() {}

  virtual ezResult Initialize() override;

  virtual ezResult Deinitialize() override;

private:

  PxRigidStatic* m_pActor;
};
