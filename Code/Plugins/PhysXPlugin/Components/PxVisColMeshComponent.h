#pragma once

#include <PhysXPlugin/PhysXPluginDLL.h>
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

class EZ_PHYSXPLUGIN_DLL ezPxVisColMeshComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxVisColMeshComponent, ezRenderComponent, ezPxVisColMeshComponentManager);

public:
  ezPxVisColMeshComponent();
  ~ezPxVisColMeshComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface

protected:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void Initialize() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent interface

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // ezPxVisColMeshComponent interface
  
protected:
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
  ezPxMeshResourceHandle m_hCollisionMesh;
  mutable ezMeshResourceHandle m_hMesh;
};
