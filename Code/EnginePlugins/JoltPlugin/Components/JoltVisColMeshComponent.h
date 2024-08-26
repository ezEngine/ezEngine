#pragma once

#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

class ezJoltVisColMeshComponentManager : public ezComponentManager<class ezJoltVisColMeshComponent, ezBlockStorageType::Compact>
{
public:
  using SUPER = ezComponentManager<ezJoltVisColMeshComponent, ezBlockStorageType::Compact>;

  ezJoltVisColMeshComponentManager(ezWorld* pWorld)
    : SUPER(pWorld)
  {
  }

  void Update(const ezWorldModule::UpdateContext& context);
  void EnqueueUpdate(ezComponentHandle hComponent);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  mutable ezMutex m_Mutex;
  ezDeque<ezComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

/// \brief Visualizes a Jolt collision mesh that is attached to the same game object.
///
/// When attached to a game object where a ezJoltStaticActorComponent or a ezJoltShapeConvexHullComponent is attached as well,
/// this component will retrieve the triangle mesh and turn it into a render mesh.
///
/// This is used for displaying the collision mesh of a single object.
/// It doesn't work for non-mesh shape types (sphere, box, capsule).
class EZ_JOLTPLUGIN_DLL ezJoltVisColMeshComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltVisColMeshComponent, ezRenderComponent, ezJoltVisColMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltVisColMeshComponent

public:
  ezJoltVisColMeshComponent();
  ~ezJoltVisColMeshComponent();

  /// \brief If this is set directly, the mesh is not taken from the sibling components.
  void SetMesh(const ezJoltMeshResourceHandle& hMesh);                                          // [ property ]
  EZ_ALWAYS_INLINE const ezJoltMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; } // [ property ]

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void CreateCollisionRenderMesh();

  ezJoltMeshResourceHandle m_hCollisionMesh;
  mutable ezMeshResourceHandle m_hMesh;
};
