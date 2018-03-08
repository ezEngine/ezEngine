#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeCapsuleComponent, ezBlockStorageType::FreeList> ezPxShapeCapsuleComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeCapsuleComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeCapsuleComponent, ezPxShapeComponent, ezPxShapeCapsuleComponentManager);

public:
  ezPxShapeCapsuleComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;

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

  virtual physx::PxShape* CreateShape(physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform) override;

protected:

};


