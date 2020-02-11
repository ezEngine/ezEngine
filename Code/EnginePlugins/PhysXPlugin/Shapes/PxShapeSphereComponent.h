#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeSphereComponent, ezBlockStorageType::FreeList> ezPxShapeSphereComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeSphereComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeSphereComponent, ezPxShapeComponent, ezPxShapeSphereComponentManager);

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
  // ezPxShapeSphereComponent

public:
  ezPxShapeSphereComponent();
  ~ezPxShapeSphereComponent();

  void SetRadius(float f);                      // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
};
