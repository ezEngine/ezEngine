#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeSphereComponent, ezBlockStorageType::FreeList> ezPxShapeSphereComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeSphereComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeSphereComponent, ezPxShapeComponent, ezPxShapeSphereComponentManager);

public:
  ezPxShapeSphereComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const;

  // ************************************* PROPERTIES ***********************************
public:

  void SetRadius(float f);
  float GetRadius() const { return m_fRadius; }

protected:
  float m_fRadius;


  // ************************************* FUNCTIONS *****************************

public:

  virtual physx::PxShape* CreateShape(physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform) override;

protected:

};


