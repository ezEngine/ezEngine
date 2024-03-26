#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using ezAimIKComponentManager = ezComponentManager<class ezAimIKComponent, ezBlockStorageType::FreeList>;

struct ezIkJointEntry
{
  ezHashedString m_sJointName;
  float m_fWeight = 1.0f;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezIkJointEntry);

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

  ezEnum<ezBasisAxis> m_ForwardVector = ezBasisAxis::PositiveX;
  ezEnum<ezBasisAxis> m_UpVector = ezBasisAxis::PositiveZ;
  float m_fWeight = 1.0f;
  //ezVec3 m_vPoleTarget = ezVec3::MakeZero();
  ezHybridArray<ezIkJointEntry, 2> m_Joints;

protected:
  void OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const; // [ msg handler ]
};
