#pragma once

#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>

using ezPxShapeConvexComponentManager = ezComponentManager<class ezPxShapeConvexComponent, ezBlockStorageType::FreeList>;

class EZ_PHYSXPLUGIN_DLL ezPxShapeConvexComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeConvexComponent, ezPxShapeComponent, ezPxShapeConvexComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxShapeComponent

protected:
  virtual void CreateShapes(ezDynamicArray<physx::PxShape*>& out_Shapes, physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxShapeConvexComponent

public:
  ezPxShapeConvexComponent();
  ~ezPxShapeConvexComponent();

  virtual void ExtractGeometry(ezMsgExtractGeometry& ref_msg) const override;

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

protected:
  ezPxMeshResourceHandle m_hCollisionMesh;
};
