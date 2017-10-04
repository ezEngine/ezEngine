#pragma once

#include <PhysXPlugin/Basics.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Meshes/MeshComponent.h>

class ezPxVisColMeshComponentManager : public ezComponentManager<class ezPxVisColMeshComponent, ezBlockStorageType::Compact>
{
public:
  typedef ezComponentManager<ezPxVisColMeshComponent, ezBlockStorageType::Compact> SUPER;

  ezPxVisColMeshComponentManager(ezWorld* pWorld) : SUPER(pWorld)
  {
  }

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);

  void EnqueueUpdate(ezComponentHandle hComponent);

private:
  ezDeque<ezComponentHandle> m_RequireUpdate;
};

class EZ_PHYSXPLUGIN_DLL ezPxVisColMeshComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxVisColMeshComponent, ezRenderComponent, ezPxVisColMeshComponentManager);

public:
  ezPxVisColMeshComponent();
  ~ezPxVisColMeshComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ezRenderComponent interface
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

protected:
  virtual ezMeshRenderData* CreateRenderData(ezUInt32 uiBatchId) const;
  void CreateCollisionRenderMesh();

  // ************************************* PROPERTIES ***********************************
public:

  void SetMeshFile(const char* szFile);
  const char* GetMeshFile() const;

protected:

  // ************************************* FUNCTIONS *****************************

public:
  void SetMesh(const ezPxMeshResourceHandle& hMesh);
  EZ_ALWAYS_INLINE const ezPxMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezPxMeshResourceHandle m_hCollisionMesh;
  mutable ezMeshResourceHandle m_hMesh;
};
