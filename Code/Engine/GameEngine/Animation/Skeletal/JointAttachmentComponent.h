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
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJointAttachmentComponent

public:
  ezJointAttachmentComponent();
  ~ezJointAttachmentComponent();

  void SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;      // [ property ]

protected:
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg); // [ msg handler ]

  ezHashedString m_sJointToAttachTo;
  ezUInt16 m_uiJointIndex = ezInvalidJointIndex;
};
