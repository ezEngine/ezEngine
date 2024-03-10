#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

struct ezMsgExtractGeometry;
using ezPxStaticActorComponentManager = ezComponentManager<class ezPxStaticActorComponent, ezBlockStorageType::FreeList>;

class EZ_PHYSXPLUGIN_DLL ezPxStaticActorComponent : public ezPxActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxStaticActorComponent, ezPxActorComponent, ezPxStaticActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  void PullSurfacesFromGraphicsMesh(ezHybridArray<physx::PxMaterial*, 32>& ref_materials);

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

  ezUInt8 m_uiCollisionLayer = 0;                     // [ property ]
  bool m_bIncludeInNavmesh = true;                    // [ property ]
  bool m_bPullSurfacesFromGraphicsMesh = true;        // [ property ]

protected:
  void OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const;

  ezUInt32 m_uiShapeId = ezInvalidIndex;
  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
  ezPxMeshResourceHandle m_hCollisionMesh;

  physx::PxRigidStatic* m_pActor = nullptr;

  // array to keep surfaces alive, in case they are pulled from the materials of the render mesh
  ezDynamicArray<ezSurfaceResourceHandle> m_UsedSurfaces;
};
