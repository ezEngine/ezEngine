#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeBoxComponent, ezBlockStorageType::FreeList> ezPxShapeBoxComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeBoxComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeBoxComponent, ezPxShapeComponent, ezPxShapeBoxComponentManager);

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
  // ezPxShapeBoxComponent

public:
  ezPxShapeBoxComponent();
  ~ezPxShapeBoxComponent();

  void SetExtents(const ezVec3& value);                   // [ property ]
  const ezVec3& GetExtents() const { return m_vExtents; } // [ property ]

  virtual void ExtractGeometry(ezMsgExtractGeometry& ref_msg) const override;

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;

  ezVec3 m_vExtents = ezVec3(1.0f);
};
