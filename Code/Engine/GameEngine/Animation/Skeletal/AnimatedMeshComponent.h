#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

typedef ezComponentManager<class ezAnimatedMeshComponent, ezBlockStorageType::FreeList> ezAnimatedMeshComponentManager;

class EZ_GAMEENGINE_DLL ezAnimatedMeshComponent : public ezSkinnedMeshComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAnimatedMeshComponent, ezSkinnedMeshComponent, ezAnimatedMeshComponentManager);


  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezMeshComponentBase

protected:
  virtual ezMeshRenderData* CreateRenderData() const override;

  //////////////////////////////////////////////////////////////////////////
  // ezAnimatedMeshComponent

public:
  ezAnimatedMeshComponent();
  ~ezAnimatedMeshComponent();

  bool m_bVisualizeBindPose = false;

protected:
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg);     // [ msg handler ]
  void OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg); // [ msg handler ]

  // ezAnimationPose m_AnimationPose;
  ezDynamicArray<ezMat4> m_AnimTransforms;
  ezSkeletonResourceHandle m_hSkeleton;
  ezDynamicArray<ezDebugRenderer::Line> m_Lines;
};
