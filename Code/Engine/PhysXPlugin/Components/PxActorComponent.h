#pragma once

#include <PhysXPlugin/Components/PxComponent.h>

struct ezBuildNavMeshMessage;

class EZ_PHYSXPLUGIN_DLL ezPxActorComponent : public ezPxComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxActorComponent, ezPxComponent);

public:
  ezPxActorComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  void AddShapesFromObject(ezGameObject* pObject, physx::PxRigidActor* pActor, const ezSimdTransform& ParentTransform);
  void AddShapesToNavMesh(const ezGameObject* pObject, ezBuildNavMeshMessage& msg) const;
};


