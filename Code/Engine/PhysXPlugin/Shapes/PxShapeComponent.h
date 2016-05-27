#pragma once

#include <PhysXPlugin/Components/PhysXComponent.h>
#include <GameUtils/Surfaces/SurfaceResource.h>

class EZ_PHYSXPLUGIN_DLL ezPxShapeComponent : public ezPhysXComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxShapeComponent, ezPhysXComponent);

public:
  ezPxShapeComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void Initialize() override;

  // ************************************* PROPERTIES ***********************************
public:

  // physics material
  // collision flags ?
  // flags: scene query shape, simulate, trigger (?), vdb vis

  void SetSurfaceFile(const char* szFile);
  const char* GetSurfaceFile() const;

  ezUInt8 m_uiCollisionLayer;

protected:
  ezSurfaceResourceHandle m_hSurface;

  // ************************************* FUNCTIONS *****************************

public:
  virtual void AddToActor(PxRigidActor* pActor, const ezTransform& ParentTransform) = 0;

protected:
  PxMaterial* GetPxMaterial();
};


