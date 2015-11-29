#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeCapsuleComponent> ezPxShapeCapsuleComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeCapsuleComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeCapsuleComponent, ezPxShapeComponent, ezPxShapeCapsuleComponentManager);
  virtual void ezPhysXComponentIsAbstract() override {}

public:
  ezPxShapeCapsuleComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion) override;

  // ************************************* PROPERTIES ***********************************
public:

  float m_fRadius;
  float m_fHalfHeight;


protected:


  // ************************************* FUNCTIONS *****************************

public:

  virtual void AddToActor(PxRigidActor* pActor, const ezTransform& ParentTransform) override;

protected:

};


