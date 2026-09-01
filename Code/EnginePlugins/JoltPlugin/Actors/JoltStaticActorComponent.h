#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

struct ezMsgExtractGeometry;

struct ezMsgPhysicsMakeTemporarilyDynamic;

/// Manager for ezJoltStaticActorComponent.
///
/// Beyond the default component management it keeps track of the static actors that were temporarily turned into
/// dynamic ones, and copies their simulated transform back onto their owner objects.
class EZ_JOLTPLUGIN_DLL ezJoltStaticActorComponentManager : public ezComponentManager<class ezJoltStaticActorComponent, ezBlockStorageType::FreeList>
{
public:
  ezJoltStaticActorComponentManager(ezWorld* pWorld);
  ~ezJoltStaticActorComponentManager();

private:
  friend class ezJoltWorldModule;
  friend class ezJoltStaticActorComponent;

  void UpdateTemporarilyDynamicActors();

  ezDynamicArray<ezComponentHandle> m_TemporarilyDynamicActors;
};

/// Turns an object into an immovable obstacle in the physics simulation.
///
/// Dynamic actors collide with static actors. Static actors cannot be moved, not even programmatically.
/// If that is desired, use a dynamic actor instead and set it to be "kinematic".
///
/// Static actors are the only ones that can use concave collision meshes.
class EZ_JOLTPLUGIN_DLL ezJoltStaticActorComponent : public ezJoltActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltStaticActorComponent, ezJoltActorComponent, ezJoltStaticActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  void OnMsgPhysicsMakeTemporarilyDynamic(ezMsgPhysicsMakeTemporarilyDynamic& msg);

  //////////////////////////////////////////////////////////////////////////
  // ezJoltActorComponent
protected:
  virtual void CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltStaticActorComponent

public:
  ezJoltStaticActorComponent();
  ~ezJoltStaticActorComponent();

  /// Searches for a sibling ezMeshComponent and attempts to retrieve information about the ezJoltMaterial to use for each submesh from its ezMaterial information.
  void PullSurfacesFromGraphicsMesh(ezDynamicArray<const ezJoltMaterial*>& ref_materials);

  void SetMesh(const ezJoltMeshResourceHandle& hMesh);
  EZ_ALWAYS_INLINE const ezJoltMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

  void SetSurfaceFile(ezStringView sFile);      // [ property ]
  ezStringView GetSurfaceFile() const;          // [ property ]

  bool m_bPullSurfacesFromGraphicsMesh = false; // [ property ]
  ezSurfaceResourceHandle m_hSurface;           // [ property ]

protected:
  void OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const;
  const ezJoltMaterial* GetJoltMaterial() const;

  /// Whether the shapes of this actor could be used for a dynamic body. Triangle meshes can't.
  bool CanBeMadeDynamic();

  ezJoltMeshResourceHandle m_hCollisionMesh;

  // array to keep surfaces alive, in case they are pulled from the materials of the render mesh
  ezDynamicArray<ezSurfaceResourceHandle> m_UsedSurfaces;
};
