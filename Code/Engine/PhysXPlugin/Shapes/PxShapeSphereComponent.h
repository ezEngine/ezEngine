#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeSphereComponent> ezPxShapeSphereComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeSphereComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeSphereComponent, ezPxShapeComponent, ezPxShapeSphereComponentManager);
  virtual void ezPhysXComponentIsAbstract() override {}

public:
  ezPxShapeSphereComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  float m_fRadius;


protected:


  // ************************************* FUNCTIONS *****************************

public:

  virtual void AddToActor(PxRigidActor* pActor, const ezTransform& ParentTransform) override;

protected:

};


