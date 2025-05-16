#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using ezAimIKComponentManager = ezComponentManager<class ezAimIKComponent, ezBlockStorageType::FreeList>;

struct ezIkJointEntry
{
  ezHashedString m_sJointName;
  float m_fWeight = 1.0f;
  mutable ezUInt16 m_uiJointIdx = 0;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezIkJointEntry);

/// \brief Adds inverse kinematics for a single joint of an animated mesh to point towards a target.
///
/// This can be used to make a creature look at something or to aim at a target.
/// The component has to be attached to a child object of an animated mesh.
/// The animated mesh needs to be driven by another component that generates an animation pose,
/// such as a ezAnimationControllerComponent or a ezSimpleAnimationComponent.
/// On those "EnableIK" must be set, then they will forward the pose to all their child objects and give them the
/// opportunity to override the pose using IK.
class EZ_GAMEENGINE_DLL ezAimIKComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAimIKComponent, ezComponent, ezAimIKComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezAimIKComponent

public:
  ezAimIKComponent();
  ~ezAimIKComponent();

  void SetPoleVectorReference(const char* szReference);         // [ property ]

  ezGameObjectHandle m_hPoleVector;                             // [ property ] An optional other object used as the pole vector for the joint to align with.
  ezEnum<ezBasisAxis> m_ForwardVector = ezBasisAxis::PositiveX; // [ property ] The local forward direction of the joint to orient towards the position of this object.
  ezEnum<ezBasisAxis> m_UpVector = ezBasisAxis::PositiveZ;      // [ property ] The local up direction of the joint to orient towards the pole vector.
  float m_fWeight = 1.0f;                                       // [ property ] Factor between 0 and 1 for how much to apply the IK.
  ezHybridArray<ezIkJointEntry, 2> m_Joints;                    // [ property ] A list of joints to apply the aim IK to. If multiple joints along a chain are used, set a weight of less than 1 for the first joints and a factor of 1 for the last joint, to distribute gradual aiming along the chain.

  bool m_bInversePoleVector = false;                            // [ property ] If true, the pole-vector will point away from the given position, not towards it.

  void SetDebugVisScale(float fScale);                          // [ property ] Scale for debug visualizations. 0 to disable.
  float GetDebugVisScale() const;

protected:
  void OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const; // [ msg handler ]

  ezUInt8 m_uiDebugVisScale = 0;

  const char* DummyGetter() const { return nullptr; }
};
