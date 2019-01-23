#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeBoxComponent, ezBlockStorageType::FreeList> ezPxShapeBoxComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeBoxComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeBoxComponent, ezPxShapeComponent, ezPxShapeBoxComponentManager);

public:
  ezPxShapeBoxComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;
  

  // ************************************* PROPERTIES ***********************************
public:

  void SetExtents(const ezVec3& value);
  const ezVec3& GetExtents() const { return m_vExtents; }

protected:
  ezVec3 m_vExtents;


  // ************************************* FUNCTIONS *****************************

public:

  virtual physx::PxShape* CreateShape(physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform) override;
  virtual void ExtractGeometry(ezMsgExtractGeometry& msg) const override;

protected:

};


