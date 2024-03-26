#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using ezFootIKComponentManager = ezComponentManager<class ezFootIKComponent, ezBlockStorageType::FreeList>;

class EZ_GAMEENGINE_DLL ezFootIKComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFootIKComponent, ezComponent, ezFootIKComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezFootIKComponent

public:
  ezFootIKComponent();
  ~ezFootIKComponent();

  float m_fWeight = 1.0f;
  float m_fSoften = 1.0f;
  ezHashedString m_sJointStart;
  ezHashedString m_sJointMiddle;
  ezHashedString m_sJointEnd;
  ezEnum<ezBasisAxis> m_MidAxis;
  ezAngle m_TwistAngle;
  ezVec3 m_vPoleTarget = ezVec3::MakeZero();

protected:
  void OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const; // [ msg handler ]

  //ezUInt16 m_uiJointIdxStart = 0;
  //ezUInt16 m_uiJointIdxMiddle = 0;
  //ezUInt16 m_uiJointIdxEnd = 0;
};
