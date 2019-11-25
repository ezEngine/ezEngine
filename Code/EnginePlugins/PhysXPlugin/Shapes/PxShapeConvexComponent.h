#pragma once

#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeConvexComponent, ezBlockStorageType::FreeList> ezPxShapeConvexComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeConvexComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeConvexComponent, ezPxShapeComponent, ezPxShapeConvexComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxShapeComponent

protected:
  virtual physx::PxShape* CreateShape(physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxShapeConvexComponent

public:
  ezPxShapeConvexComponent();
  ~ezPxShapeConvexComponent();

  virtual void ExtractGeometry(ezMsgExtractGeometry& msg) const override;

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

protected:
  ezPxMeshResourceHandle m_hCollisionMesh;
};
