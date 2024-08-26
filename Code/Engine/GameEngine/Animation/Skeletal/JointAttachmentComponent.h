#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using ezJointAttachmentComponentManager = ezComponentManager<class ezJointAttachmentComponent, ezBlockStorageType::FreeList>;

/// \brief Used to expose an animated mesh's bone as a game object, such that objects can be attached to it to move along.
///
/// The animation system deals with bone animations internally.
/// Sometimes it is desirable to move certain objects along with a bone,
/// for example when a character should hold something in their hand.
///
/// This component references a bone by name, and takes care to position the owner object at the same location as the bone
/// whenever the animation pose changes.
/// Thus it is possible to attach other objects as child objects to this one, so that they move along as well.
class EZ_GAMEENGINE_DLL ezJointAttachmentComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJointAttachmentComponent, ezComponent, ezJointAttachmentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJointAttachmentComponent

public:
  ezJointAttachmentComponent();
  ~ezJointAttachmentComponent();

  /// \brief Sets the bone name whose transform should be copied into this game object.
  void SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;      // [ property ]

  /// \brief An additional local offset to be added to the transform.
  ezVec3 m_vLocalPositionOffset = ezVec3::MakeZero(); // [ property ]

  /// \brief An additional local offset to be added to the transform.
  ezQuat m_vLocalRotationOffset = ezQuat::MakeIdentity();      // [ property ]

protected:
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg); // [ msg handler ]

  ezHashedString m_sJointToAttachTo;
  ezUInt16 m_uiJointIndex = ezInvalidJointIndex;
};
