#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeSphereComponent, true> ezPxShapeSphereComponentManager;

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

  virtual PxShape* CreateShape(PxRigidActor* pActor, PxTransform& out_ShapeTransform) override;

protected:

};


