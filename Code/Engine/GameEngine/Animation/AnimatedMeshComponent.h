#pragma once

#include <GameEngine/Basics.h>
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
  EZ_ALWAYS_INLINE const ezAnimationClipResourceHandle& GetAnimationClip() const { return m_hAnimationClip; }

  void SetAnimationClipFile(const char* szFile);
  const char* GetAnimationClipFile() const;

  void Update();

protected:
  ezAnimationClipResourceHandle m_hAnimationClip;
  ezAnimationPose m_AnimationPose;
  ezSkeletonResourceHandle m_hSkeleton;
  ezTime m_AnimationTime;
};
