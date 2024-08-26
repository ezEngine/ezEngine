#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

class EZ_GAMEENGINE_DLL ezAnimatedMeshComponentManager : public ezComponentManager<class ezAnimatedMeshComponent, ezBlockStorageType::FreeList>
{
public:
  ezAnimatedMeshComponentManager(ezWorld* pWorld);
  ~ezAnimatedMeshComponentManager();

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);
  void AddToUpdateList(ezAnimatedMeshComponent* pComponent);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezDeque<ezComponentHandle> m_ComponentsToUpdate;
};

/// \brief Instantiates a mesh that can be animated through skeletal animation.
///
/// The referenced mesh has to contain skinning information.
///
/// This component only creates an animated mesh for rendering. It does not animate the mesh in any way.
/// The component handles messages of type ezMsgAnimationPoseUpdated. Using this message other systems can set a new pose
/// for the animated mesh.
///
/// For example the ezSkeletonPoseComponent, ezSimpleAnimationComponent and ezAnimationControllerComponent do this
/// to change the pose of the animated mesh.
class EZ_GAMEENGINE_DLL ezAnimatedMeshComponent : public ezMeshComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezAnimatedMeshComponent, ezMeshComponentBase, ezAnimatedMeshComponentManager);


  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezMeshComponentBase

protected:
  virtual ezMeshRenderData* CreateRenderData() const override;
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezAnimatedMeshComponent

public:
  ezAnimatedMeshComponent();
  ~ezAnimatedMeshComponent();

  void RetrievePose(ezDynamicArray<ezMat4>& out_modelTransforms, ezTransform& out_rootTransform, const ezSkeleton& skeleton);

protected:
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg);     // [ msg handler ]
  void OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg); // [ msg handler ]

  void InitializeAnimationPose();

  void MapModelSpacePoseToSkinningSpace(const ezHashTable<ezHashedString, ezMeshResourceDescriptor::BoneData>& bones, const ezSkeleton& skeleton, ezArrayPtr<const ezMat4> modelSpaceTransforms, ezBoundingBox* bounds);

  ezTransform m_RootTransform = ezTransform::MakeIdentity();
  ezBoundingBox m_MaxBounds;
  ezSkinningState m_SkinningState;
  ezSkeletonResourceHandle m_hDefaultSkeleton;
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

  EZ_GAMEENGINE_DLL static void Apply(ezRootMotionMode::Enum mode, ezGameObject* pObject, const ezVec3& vTranslation, ezAngle rotationX, ezAngle rotationY, ezAngle rotationZ);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezRootMotionMode);
