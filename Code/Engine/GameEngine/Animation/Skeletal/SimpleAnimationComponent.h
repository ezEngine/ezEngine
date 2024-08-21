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
struct ezMsgGenericEvent;

using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;
using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

using ezSimpleAnimationComponentManager = ezComponentManagerSimple<class ezSimpleAnimationComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::FreeList>;

#define EZ_ADD_RESOURCEHANDLE_PROPERTIES(name, member)                                  \
  void Set##name##File(ezStringView sFile)                                              \
  {                                                                                     \
    if (!sFile.IsEmpty())                                                               \
    {                                                                                   \
      member = ezResourceManager::LoadResource<decltype(member)::RESOURCE_TYPE>(sFile); \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
      member = {};                                                                      \
    }                                                                                   \
  }                                                                                     \
                                                                                        \
  ezStringView Get##name##File() const                                                  \
  {                                                                                     \
    return member.GetResourceID();                                                      \
  }

/// \brief Plays a single animation clip on an animated mesh.
///
/// \see ezAnimatedMeshComponent
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


  ezAnimationClipResourceHandle m_hAnimationClip;

  void SetAnimationClipFile(ezStringView sFile)
  {
    if (!sFile.IsEmpty())
    {
      m_hAnimationClip = ezResourceManager::LoadResource<ezAnimationClipResource>(sFile);
    }
    else
    {
      m_hAnimationClip = {};
    }
  }

  ezStringView GetAnimationClipFile() const
  {
    return m_hAnimationClip.GetResourceID();
  }

  /// \brief How to play the animation.
  ezEnum<ezPropertyAnimMode> m_AnimationMode; // [ property ]

  /// \brief How quickly or slowly to play the animation.
  float m_fSpeed = 1.0f; // [ property ]

  /// \brief Sets the current sample position of the animation clip in 0 (start) to 1 (end) range.
  void SetNormalizedPlaybackPosition(float fPosition);

  /// \brief Returns the normalized [0;1] sample position of the animation clip.
  float GetNormalizedPlaybackPosition() const { return m_fNormalizedPlaybackPosition; }

  /// \brief How often to update the animation while the animated mesh is invisible.
  ezEnum<ezAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

protected:
  void Update();
  bool UpdatePlaybackTime(ezTime tDiff, const ezEventTrack& eventTrack, ezAnimPoseEventTrackSampleMode& out_trackSampling);

  ezEnum<ezRootMotionMode> m_RootMotionMode;
  float m_fNormalizedPlaybackPosition = 0.0f;
  ezTime m_Duration;
  ezSkeletonResourceHandle m_hSkeleton;
  ezTime m_ElapsedTimeSinceUpdate = ezTime::MakeZero();
  bool m_bEnableIK = false;

  ozz::vector<ozz::math::SoaTransform> m_OzzLocalTransforms; // TODO: could be frame allocated
};
