#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeCapsuleComponent, ezBlockStorageType::FreeList> ezPxShapeCapsuleComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeCapsuleComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeCapsuleComponent, ezPxShapeComponent, ezPxShapeCapsuleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxShapeComponent

protected:
  virtual void CreateShapes(ezDynamicArray<physx::PxShape*>& out_Shapes, physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxShapeCapsuleComponent

public:
  ezPxShapeCapsuleComponent();
  ~ezPxShapeCapsuleComponent();

  void SetRadius(float value);                  // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

  void SetHeight(float value);                  // [ property ]
  float GetHeight() const { return m_fHeight; } // [ property ]


protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
  float m_fHeight = 0.5f;
};
