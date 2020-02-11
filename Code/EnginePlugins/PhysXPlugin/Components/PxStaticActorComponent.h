#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

struct ezMsgExtractGeometry;
typedef ezComponentManager<class ezPxStaticActorComponent, ezBlockStorageType::FreeList> ezPxStaticActorComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxStaticActorComponent : public ezPxActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxStaticActorComponent, ezPxActorComponent, ezPxStaticActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxStaticActorComponent

public:
  ezPxStaticActorComponent();
  ~ezPxStaticActorComponent();

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

  void SetMesh(const ezPxMeshResourceHandle& hMesh);
  EZ_ALWAYS_INLINE const ezPxMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

  ezUInt32 GetShapeId() const { return m_uiShapeId; } // [ scriptable ]

  ezUInt8 m_uiCollisionLayer = 0;  // [ property ]
  bool m_bIncludeInNavmesh = true; // [ property ]

protected:
  void OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const;

  ezUInt32 m_uiShapeId = ezInvalidIndex;
  ezPxMeshResourceHandle m_hCollisionMesh;

  physx::PxRigidStatic* m_pActor = nullptr;

  ezPxUserData m_UserData;
};
