#pragma once

#include <PhysXPlugin/Components/PhysXComponent.h>

typedef ezComponentManagerAbstract<class ezPxActorComponent> ezPxActorComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxActorComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxActorComponent, ezPhysXComponent, ezPxActorComponentManager);

public:
  ezPxActorComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion) override;

  // ************************************* PROPERTIES ***********************************
public:


protected:


  // ************************************* FUNCTIONS *****************************

public:


protected:
  void AddShapesFromObject(ezGameObject* pObject, PxRigidActor* pActor, const ezTransform& ParentTransform);
  void AddShapesFromChildren(ezGameObject* pRoot, PxRigidActor* pActor, const ezTransform& ParentTransform);

};


