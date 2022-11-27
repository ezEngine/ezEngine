#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

class ezEventTrack;
struct ezMsgAnimationReachedEnd;
struct ezMsgGenericEvent;

using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;
using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

using ezSimpleAnimationComponentManager = ezComponentManagerSimple<class ezSimpleAnimationComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::FreeList>;

class EZ_GAMEENGINE_DLL ezSimpleAnimationComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSimpleAnimationComponent, ezComponent, ezSimpleAnimationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJointAttachmentComponent

public:
  ezSimpleAnimationComponent();
  ~ezSimpleAnimationComponent();

  void SetAnimationClip(const ezAnimationClipResourceHandle& hResource);
  const ezAnimationClipResourceHandle& GetAnimationClip() const;

  void SetAnimationClipFile(const char* szFile); // [ property ]
  const char* GetAnimationClipFile() const;      // [ property ]

  ezEnum<ezPropertyAnimMode> m_AnimationMode; // [ property ]
  float m_fSpeed = 1.0f;                      // [ property ]

  void SetNormalizedPlaybackPosition(float fPosition);
  float GetNormalizedPlaybackPosition() const { return m_fNormalizedPlaybackPosition; }

protected:
  void Update();
  bool UpdatePlaybackTime(ezTime tDiff, const ezEventTrack& eventTrack, ezAnimPoseEventTrackSampleMode& out_trackSampling);

  ezEnum<ezRootMotionMode> m_RootMotionMode;
  float m_fNormalizedPlaybackPosition = 0.0f;
  ezTime m_Duration;
  ezAnimationClipResourceHandle m_hAnimationClip;
  ezSkeletonResourceHandle m_hSkeleton;

  ozz::vector<ozz::math::SoaTransform> m_OzzLocalTransforms; // TODO: could be frame allocated
};
