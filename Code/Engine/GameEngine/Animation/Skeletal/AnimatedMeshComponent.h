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
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  //////////////////////////////////////////////////////////////////////////
  // ezAnimatedMeshComponent

public:
  ezAnimatedMeshComponent();
  ~ezAnimatedMeshComponent();

protected:
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg);     // [ msg handler ]
  void OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg); // [ msg handler ]

  void InitializeAnimationPose();

  ezTransform m_RootTransform = ezTransform::IdentityTransform();
  ezSkinningSpaceAnimationPose m_SkinningSpacePose;
};


struct ezRootMotionMode
{
  using StorageType = ezInt8;

  enum Enum
  {
    Ignore,
    ApplyToOwner,
    SendMoveCharacterMsg,

    Default = Ignore
  };

  EZ_GAMEENGINE_DLL static void Apply(ezRootMotionMode::Enum mode, ezGameObject* pObject, const ezVec3& translation, ezAngle rotationX, ezAngle rotationY, ezAngle rotationZ);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezRootMotionMode);
