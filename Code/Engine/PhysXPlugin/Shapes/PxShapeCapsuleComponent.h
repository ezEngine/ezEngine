#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeCapsuleComponent, true> ezPxShapeCapsuleComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeCapsuleComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeCapsuleComponent, ezPxShapeComponent, ezPxShapeCapsuleComponentManager);

public:
  ezPxShapeCapsuleComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const;

  // ************************************* PROPERTIES ***********************************
public:

  void SetRadius(float value);
  float GetRadius() const { return m_fRadius; }

  void SetHeight(float value);
  float GetHeight() const { return m_fHeight; }


protected:

  float m_fRadius;
  float m_fHeight;


  // ************************************* FUNCTIONS *****************************

public:

  virtual PxShape* CreateShape(PxRigidActor* pActor, PxTransform& out_ShapeTransform) override;

protected:

};


