#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using ezTwoBoneIKComponentManager = ezComponentManager<class ezTwoBoneIKComponent, ezBlockStorageType::FreeList>;

/// \brief Adds inverse kinematics for a chain of three joints (two-bones) of an animated mesh to reach a target position.
///
/// This can be used to make a creature grab something or to implement foot IK.
/// The component has to be attached to a child object of an animated mesh.
/// The animated mesh needs to be driven by another component that generates an animation pose,
/// such as a ezAnimationControllerComponent or a ezSimpleAnimationComponent.
/// On those "EnableIK" must be set, then they will forward the pose to all their child objects and give them the
/// opportunity to override the pose using IK.
class EZ_GAMEENGINE_DLL ezTwoBoneIKComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTwoBoneIKComponent, ezComponent, ezTwoBoneIKComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezTwoBoneIKComponent

public:
  ezTwoBoneIKComponent();
  ~ezTwoBoneIKComponent();

  void SetPoleVectorReference(const char* szReference); // [ property ]

  ezGameObjectHandle m_hPoleVector;                     ///< [ property ] An optional other object used as the pole vector for the middle joint to point towards.
  float m_fWeight = 1.0f;                               ///< [ property ] Factor between 0 and 1 for how much to apply the IK.
  ezHashedString m_sJointStart;                         ///< [ property ] First joint in the chain.
  ezHashedString m_sJointMiddle;                        ///< [ property ] Second joint in the chain.
  ezHashedString m_sJointEnd;                           ///< [ property ] Third joint in the chain. This one tries to reach the target position.
  ezEnum<ezBasisAxis> m_MidAxis;                        ///< [ property ] The axis of the middle joint around which to bend.

  ezUInt16 m_uiOrder = 0;                               ///< [ property ] At which point in the IK calculation to execute this.

  void SetDebugVisScale(float fScale);                  // [ property ] Scale for debug visualizations. 0 to disable.
  float GetDebugVisScale() const;

  // currently these are not exposed, to reduce the number of parameters to fiddle with
  // float m_fSoften = 1.0f;
  // ezAngle m_TwistAngle;

protected:
  void OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const; // [ msg handler ]

  ezUInt8 m_uiDebugVisScale = 0;

  mutable ezUInt16 m_uiJointIdxStart = 0;
  mutable ezUInt16 m_uiJointIdxMiddle = 0;
  mutable ezUInt16 m_uiJointIdxEnd = 0;

  const char* DummyGetter() const { return nullptr; }
};
