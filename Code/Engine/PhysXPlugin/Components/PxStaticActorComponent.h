#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>

typedef ezComponentManager<class ezPxStaticActorComponent> ezPxStaticActorComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxStaticActorComponent : public ezPxActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxStaticActorComponent, ezPxActorComponent, ezPxStaticActorComponentManager);
  virtual void ezPhysXComponentIsAbstract() override {}

public:
  ezPxStaticActorComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  void SetMeshFile(const char* szFile);
  const char* GetMeshFile() const;

  ezUInt8 m_uiCollisionLayer;

protected:
  ezPhysXMeshResourceHandle m_hCollisionMesh;

  // ************************************* FUNCTIONS *****************************

public:
  void SetMesh(const ezPhysXMeshResourceHandle& hMesh);
  EZ_FORCE_INLINE const ezPhysXMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

  virtual void OnSimulationStarted() override;

  virtual void Deinitialize() override;

private:

  PxRigidStatic* m_pActor;
};
