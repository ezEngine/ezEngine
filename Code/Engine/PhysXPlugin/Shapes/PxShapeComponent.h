#pragma once

#include <PhysXPlugin/Components/PhysXComponent.h>
#include <GameUtils/Surfaces/SurfaceResource.h>

typedef ezComponentManagerAbstract<class ezPxShapeComponent> ezPxShapeComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeComponent, ezPhysXComponent, ezPxShapeComponentManager);

public:
  ezPxShapeComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  // physics material
  // collision flags ?
  // flags: scene query shape, simulate, trigger (?), vdb vis

  void SetSurfaceFile(const char* szFile);
  const char* GetSurfaceFile() const;

  ezUInt8 m_uiFilterGroup;

protected:
  ezSurfaceResourceHandle m_hSurface;

  // ************************************* FUNCTIONS *****************************

public:
  virtual void AddToActor(PxRigidActor* pActor, const ezTransform& ParentTransform) = 0;

protected:
  PxMaterial* GetPxMaterial();
};


