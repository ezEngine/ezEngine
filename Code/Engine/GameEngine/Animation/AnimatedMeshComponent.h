#pragma once

#include <GameEngine/Basics.h>
#include <RendererCore/AnimationSystem/AnimationGraph/AnimationClipSampler.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/Meshes/RenderMeshComponent.h>

typedef ezTypedResourceHandle<class ezAnimationClipResource> ezAnimationClipResourceHandle;
typedef ezTypedResourceHandle<class ezSkeletonResource> ezSkeletonResourceHandle;

typedef ezComponentManagerSimple<class ezAnimatedMeshComponent, ezComponentUpdateType::WhenSimulating> ezSimpleAnimationComponentManager;

class EZ_GAMEENGINE_DLL ezAnimatedMeshComponent : public ezRenderMeshComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAnimatedMeshComponent, ezRenderMeshComponent, ezSimpleAnimationComponentManager);

public:
  ezAnimatedMeshComponent();
  ~ezAnimatedMeshComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
  //
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnActivated() override;

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
  ezAnimationPose m_AnimationPose;
  ezSkeletonResourceHandle m_hSkeleton;
  ezAnimationClipSampler m_AnimationClipSampler;
};
