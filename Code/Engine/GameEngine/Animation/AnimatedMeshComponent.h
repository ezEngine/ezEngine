#pragma once

#include <GameEngine/Basics.h>
#include <RendererCore/AnimationSystem/AnimationGraph/AnimationClipSampler.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

struct ezSkeletonResourceDescriptor;
typedef ezTypedResourceHandle<class ezAnimationClipResource> ezAnimationClipResourceHandle;
typedef ezTypedResourceHandle<class ezSkeletonResource> ezSkeletonResourceHandle;

typedef ezComponentManagerSimple<class ezAnimatedMeshComponent, ezComponentUpdateType::WhenSimulating> ezAnimatedMeshComponentManager;

class EZ_GAMEENGINE_DLL ezAnimatedMeshComponent : public ezMeshComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezAnimatedMeshComponent, ezMeshComponentBase, ezAnimatedMeshComponentManager);

public:
  ezAnimatedMeshComponent();
  ~ezAnimatedMeshComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
  //
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  //

  //////////////////////////////////////////////////////////////////////////
  // Properties
  //

  void SetAnimationClip(const ezAnimationClipResourceHandle& hResource);
  const ezAnimationClipResourceHandle& GetAnimationClip() const;

  void SetAnimationClipFile(const char* szFile);
  const char* GetAnimationClipFile() const;

  bool GetLoopAnimation() const;
  void SetLoopAnimation(bool loop);

  float GetAnimationSpeed() const;
  void SetAnimationSpeed(float speed);

  void Update();

protected:
  void CreatePhysicsShapes(const ezSkeletonResourceDescriptor& skeleton, const ezAnimationPose& pose);
  void* m_pRagdoll = nullptr;

  bool m_bApplyRootMotion = false;
  bool m_bVisualizeSkeleton = false;
  ezAnimationPose m_AnimationPose;
  ezSkeletonResourceHandle m_hSkeleton;
  ezAnimationClipSampler m_AnimationClipSampler;
};
