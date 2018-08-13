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

  void SetSkeleton(const ezSkeletonResourceHandle& hResource);
  EZ_ALWAYS_INLINE const ezSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

  void SetSkeletonFile(const char* szFile);
  const char* GetSkeletonFile() const;


  void Update();

protected:
  ezAnimationClipResourceHandle m_hAnimationClip;
  ezSkeletonResourceHandle m_hSkeleton;

  ezAnimationPose m_AnimationPose;

  ezTime m_AnimationTime;
};
