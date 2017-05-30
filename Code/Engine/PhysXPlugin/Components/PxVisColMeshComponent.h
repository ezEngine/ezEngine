#pragma once

#include <PhysXPlugin/Basics.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Meshes/MeshComponent.h>

typedef ezComponentManager<class ezPxVisColMeshComponent, ezBlockStorageType::Compact> ezPxVisColMeshComponentManager;

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
  void CreateCollisionRenderMesh() const;

  // ************************************* PROPERTIES ***********************************
public:

  void SetMeshFile(const char* szFile);
  const char* GetMeshFile() const;

protected:

  // ************************************* FUNCTIONS *****************************

public:
  void SetMesh(const ezPhysXMeshResourceHandle& hMesh);
  EZ_ALWAYS_INLINE const ezPhysXMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

private:

  ezPhysXMeshResourceHandle m_hCollisionMesh;
  mutable ezMeshResourceHandle m_hMesh;
};
