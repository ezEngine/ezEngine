#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeBoxComponent> ezPxShapeBoxComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeBoxComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeBoxComponent, ezPxShapeComponent, ezPxShapeBoxComponentManager);
  virtual void ezPhysXComponentIsAbstract() override {}

public:
  ezPxShapeBoxComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  ezVec3 m_vHalfExtents;

protected:


  // ************************************* FUNCTIONS *****************************

public:

  virtual void AddToActor(PxRigidActor* pActor, const ezTransform& ParentTransform) override;

protected:

};


