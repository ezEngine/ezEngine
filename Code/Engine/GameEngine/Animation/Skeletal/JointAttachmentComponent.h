#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using ezJointAttachmentComponentManager = ezComponentManager<class ezJointAttachmentComponent, ezBlockStorageType::FreeList>;

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

  void SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;      // [ property ]

  ezVec3 m_vLocalPositionOffset = ezVec3::ZeroVector();         // [ property ]
  ezQuat m_vLocalRotationOffset = ezQuat::MakeIdentity(); // [ property ]

protected:
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg); // [ msg handler ]

  ezHashedString m_sJointToAttachTo;
  ezUInt16 m_uiJointIndex = ezInvalidJointIndex;
};
