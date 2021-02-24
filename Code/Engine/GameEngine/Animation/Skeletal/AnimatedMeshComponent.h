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
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezMeshComponentBase

protected:
  virtual ezMeshRenderData* CreateRenderData() const override;

  //////////////////////////////////////////////////////////////////////////
  // ezAnimatedMeshComponent

public:
  ezAnimatedMeshComponent();
  ~ezAnimatedMeshComponent();

protected:
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg);     // [ msg handler ]
  void OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg); // [ msg handler ]

  void InitializeAnimationPose();

  ezTransform m_RootTransform;
  ezSkeletonResourceHandle m_hSkeleton;
  ezSkinningSpaceAnimationPose m_SkinningSpacePose;
};
