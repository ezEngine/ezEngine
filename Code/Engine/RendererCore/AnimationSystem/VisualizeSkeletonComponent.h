#pragma once

#include <PhysXPlugin/Basics.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

class EZ_RENDERERCORE_DLL ezVisualizeSkeletonComponentManager
    : public ezComponentManager<class ezVisualizeSkeletonComponent, ezBlockStorageType::Compact>
{
public:
  typedef ezComponentManager<ezVisualizeSkeletonComponent, ezBlockStorageType::Compact> SUPER;

  ezVisualizeSkeletonComponentManager(ezWorld* pWorld);

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

class EZ_RENDERERCORE_DLL ezVisualizeSkeletonComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVisualizeSkeletonComponent, ezRenderComponent, ezVisualizeSkeletonComponentManager);

public:
  ezVisualizeSkeletonComponent();
  ~ezVisualizeSkeletonComponent();

  void Render();

protected:
  // ezComponent interface
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnSimulationStarted() override;

  // ezRenderComponent interface
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

protected:
  virtual ezMeshRenderData* CreateRenderData(ezUInt32 uiBatchId) const;
  void CreateRenderMesh();

  void CreateSkeletonGeometry(const ezSkeleton* pSkeletonData, ezGeometry& geo);
  void CreateHitBoxGeometry(const ezSkeletonResourceDescriptor* pDescriptor, ezGeometry& geo);



  // ************************************* PROPERTIES ***********************************
public:
  void SetSkeletonFile(const char* szFile);
  const char* GetSkeletonFile() const;

protected:
  // ************************************* FUNCTIONS *****************************

public:
  void SetSkeleton(const ezSkeletonResourceHandle& hMesh);
  EZ_ALWAYS_INLINE const ezSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

private:
  ezSkeletonResourceHandle m_hSkeleton;
  mutable ezMeshResourceHandle m_hMesh;
};

