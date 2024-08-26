#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using ezJointOverrideComponentManager = ezComponentManager<class ezJointOverrideComponent, ezBlockStorageType::FreeList>;

/// \brief Overrides the local transform of a bone in a skeletal animation.
///
/// Every time a new animation pose is prepared, this component replaces the transform of the chosen bone
/// to be the same as the local transform of the owner game object.
///
/// That allows you to do a simple kind of forward kinematics. For example it can be used to modify a targeting bone,
/// so that an animated object points into the right direction.
///
/// The global transform of the game object is irrelevant, but the local transform is used to copy over.
class EZ_GAMEENGINE_DLL ezJointOverrideComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJointOverrideComponent, ezComponent, ezJointOverrideComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJointOverrideComponent

public:
  ezJointOverrideComponent();
  ~ezJointOverrideComponent();

  /// \brief The name of the bone whose transform should be replaced with the transform of this game object.
  void SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;      // [ property ]

  /// \brief If true, the position of the bone will be overridden.
  bool m_bOverridePosition = false; // [ property ]

  /// \brief If true, the rotation of the bone will be overridden.
  bool m_bOverrideRotation = true; // [ property ]

  /// \brief If true, the scale of the bone will be overridden.
  bool m_bOverrideScale = false;                                         // [ property ]

protected:
  void OnAnimationPosePreparing(ezMsgAnimationPosePreparing& msg) const; // [ msg handler ]

  ezHashedString m_sJointToOverride;
  mutable ezUInt16 m_uiJointIndex = ezInvalidJointIndex;
};
