#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

struct ezMsgExtractGeometry;

using ezJoltStaticActorComponentManager = ezComponentManager<class ezJoltStaticActorComponent, ezBlockStorageType::FreeList>;

class EZ_JOLTPLUGIN_DLL ezJoltStaticActorComponent : public ezJoltActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltStaticActorComponent, ezJoltActorComponent, ezJoltStaticActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  void PullSurfacesFromGraphicsMesh(ezDynamicArray<const ezJoltMaterial*>& materials);

  //////////////////////////////////////////////////////////////////////////
  // ezJoltActorComponent
protected:
  virtual void CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltStaticActorComponent

public:
  ezJoltStaticActorComponent();
  ~ezJoltStaticActorComponent();

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

  void SetMesh(const ezJoltMeshResourceHandle& hMesh);
  EZ_ALWAYS_INLINE const ezJoltMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

  ezUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  bool m_bIncludeInNavmesh = true;              // [ property ]
  bool m_bPullSurfacesFromGraphicsMesh = false; // [ property ]

protected:
  void OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const;

  ezJoltMeshResourceHandle m_hCollisionMesh;

  // array to keep surfaces alive, in case they are pulled from the materials of the render mesh
  ezDynamicArray<ezSurfaceResourceHandle> m_UsedSurfaces;
};
