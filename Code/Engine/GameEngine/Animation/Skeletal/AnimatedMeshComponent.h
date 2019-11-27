#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationGraph/AnimationClipSampler.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>

struct ezSkeletonResourceDescriptor;
typedef ezTypedResourceHandle<class ezAnimationClipResource> ezAnimationClipResourceHandle;
typedef ezTypedResourceHandle<class ezSkeletonResource> ezSkeletonResourceHandle;

typedef ezComponentManagerSimple<class ezAnimatedMeshComponent, ezComponentUpdateType::WhenSimulating> ezAnimatedMeshComponentManager;

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
  // ezAnimatedMeshComponent

public:
  ezAnimatedMeshComponent();
  ~ezAnimatedMeshComponent();

  void SetAnimationClip(const ezAnimationClipResourceHandle& hResource);
  const ezAnimationClipResourceHandle& GetAnimationClip() const;

  void SetAnimationClipFile(const char* szFile); // [ property ]
  const char* GetAnimationClipFile() const;      // [ property ]

  bool GetLoopAnimation() const;    // [ property ]
  void SetLoopAnimation(bool loop); // [ property ]

  float GetAnimationSpeed() const;     // [ property ]
  void SetAnimationSpeed(float speed); // [ property ]


protected:
  void Update();
  void CreatePhysicsShapes(const ezSkeletonResourceDescriptor& skeleton, const ezAnimationPose& pose);

  void* m_pRagdoll = nullptr;

  bool m_bApplyRootMotion = false;
  bool m_bVisualizeSkeleton = false;
  ezAnimationPose m_AnimationPose;
  ezSkeletonResourceHandle m_hSkeleton;
  ezAnimationClipSampler m_AnimationClipSampler;
};
