#pragma once

#include <PhysXPlugin/Components/PhysXComponent.h>

typedef ezComponentManagerSimple<class ezStaticMeshComponent, true> ezStaticMeshComponentManager;

class EZ_PHYSXPLUGIN_DLL ezStaticMeshComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezStaticMeshComponent, ezPhysXComponent, ezStaticMeshComponentManager);

public:
  ezStaticMeshComponent();

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


};
