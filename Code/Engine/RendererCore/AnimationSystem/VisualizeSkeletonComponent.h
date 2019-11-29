#pragma once

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

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;


  //////////////////////////////////////////////////////////////////////////
  // ezVisualizeSkeletonComponent

public:
  ezVisualizeSkeletonComponent();
  ~ezVisualizeSkeletonComponent();

  void SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;      // [ property ]

  void SetSkeleton(const ezSkeletonResourceHandle& hMesh);
  EZ_ALWAYS_INLINE const ezSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

protected:
  void Render();
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  void CreateRenderMesh();
  void CreateSkeletonGeometry(const ezSkeleton* pSkeletonData, ezGeometry& geo);
  void CreateHitBoxGeometry(const ezSkeletonResourceDescriptor* pDescriptor, ezGeometry& geo);

  ezSkeletonResourceHandle m_hSkeleton;
  mutable ezMeshResourceHandle m_hMesh;
};
