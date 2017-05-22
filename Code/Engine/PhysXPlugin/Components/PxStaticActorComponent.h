#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

struct ezBuildNavMeshMessage;
typedef ezComponentManager<class ezPxStaticActorComponent, ezBlockStorageType::FreeList> ezPxStaticActorComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxStaticActorComponent : public ezPxActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxStaticActorComponent, ezPxActorComponent, ezPxStaticActorComponentManager);

public:
  ezPxStaticActorComponent();
  ~ezPxStaticActorComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void Deinitialize() override;

  virtual void OnSimulationStarted() override;

  void OnBuildNavMesh(ezBuildNavMeshMessage& msg) const;

  // ************************************* PROPERTIES ***********************************
public:

  void SetMeshFile(const char* szFile);
  const char* GetMeshFile() const;

  ezUInt32 GetShapeId() const { return m_uiShapeId; }

  ezUInt8 m_uiCollisionLayer;

protected:
  ezUInt32 m_uiShapeId;

  ezPhysXMeshResourceHandle m_hCollisionMesh;

  // ************************************* FUNCTIONS *****************************

public:
  void SetMesh(const ezPhysXMeshResourceHandle& hMesh);
  EZ_FORCE_INLINE const ezPhysXMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

private:

  physx::PxRigidStatic* m_pActor;

  ezPxUserData m_UserData;
};
