#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Basics.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

typedef ezComponentManagerSimple<class ezJointAttachmentComponent, ezComponentUpdateType::WhenSimulating> ezJointAttachmentComponentManager;

class EZ_GAMEENGINE_DLL ezJointAttachmentComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJointAttachmentComponent, ezComponent, ezJointAttachmentComponentManager);

public:
  ezJointAttachmentComponent();
  ~ezJointAttachmentComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
  //
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // Properties
  //
  void SetJointName(const char* szName);
  const char* GetJointName() const;

  //////////////////////////////////////////////////////////////////////////
  //
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg);

  void Update();

protected:
  ezHashedString m_sJointToAttachTo;
  ezUInt16 m_uiJointIndex = ezInvalidJointIndex;
};
