#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using ezJointOverrideComponentManager = ezComponentManager<class ezJointOverrideComponent, ezBlockStorageType::FreeList>;

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

  void SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;      // [ property ]

  bool m_bOverridePosition = false; // [ property ]
  bool m_bOverrideRotation = true;  // [ property ]
  bool m_bOverrideScale = false;    // [ property ]

protected:
  void OnAnimationPosePreparing(ezMsgAnimationPosePreparing& msg); // [ msg handler ]

  ezHashedString m_sJointToOverride;
  ezUInt16 m_uiJointIndex = ezInvalidJointIndex;
};
