#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>

typedef ezComponentManager<class ezPxShapeConvexComponent, ezBlockStorageType::FreeList> ezPxShapeConvexComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeConvexComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeConvexComponent, ezPxShapeComponent, ezPxShapeConvexComponentManager);

public:
  ezPxShapeConvexComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  void SetMeshFile(const char* szFile);
  const char* GetMeshFile() const;


protected:
  ezPxMeshResourceHandle m_hCollisionMesh;


  // ************************************* FUNCTIONS *****************************

public:

  virtual physx::PxShape* CreateShape(physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform) override;

  virtual void AddToNavMesh(ezBuildNavMeshMessage& msg) const override;

protected:

};


