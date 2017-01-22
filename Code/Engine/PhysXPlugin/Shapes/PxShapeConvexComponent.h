#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>

typedef ezComponentManager<class ezPxShapeConvexComponent, true> ezPxShapeConvexComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeConvexComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeConvexComponent, ezPxShapeComponent, ezPxShapeConvexComponentManager);

public:
  ezPxShapeConvexComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  void SetMeshFile(const char* szFile);
  const char* GetMeshFile() const;


protected:
  ezPhysXMeshResourceHandle m_hCollisionMesh;


  // ************************************* FUNCTIONS *****************************

public:

  virtual PxShape* CreateShape(PxRigidActor* pActor, PxTransform& out_ShapeTransform) override;

protected:

};


