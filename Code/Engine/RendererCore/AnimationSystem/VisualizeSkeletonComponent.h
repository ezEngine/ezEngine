#pragma once

#include <Foundation/Math/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

using ezVisualizeSkeletonComponentManager = ezComponentManagerSimple<class ezVisualizeSkeletonComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact>;

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

  void SetSkeleton(const ezSkeletonResourceHandle& hResource);
  const ezSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

protected:
  void Update();
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg); // [ msg handler ]
  void UpdateSkeletonVis();

  ezSkeletonResourceHandle m_hSkeleton;
  ezBoundingBoxSphere m_LocalBounds;
  ezDynamicArray<ezDebugRenderer::Line> m_LinesSkeleton;
};
