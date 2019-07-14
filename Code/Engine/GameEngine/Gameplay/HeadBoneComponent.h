#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/Animation/TransformComponent.h>

typedef ezComponentManagerSimple<class ezHeadBoneComponent, ezComponentUpdateType::WhenSimulating> ezHeadBoneComponentManager;

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezMsgSetVerticalHeadBoneRotation : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetVerticalHeadBoneRotation, ezMessage);

  double m_Angle; // in radians
};

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezMsgChangeVerticalHeadBoneRotation : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgChangeVerticalHeadBoneRotation, ezMessage);

  double m_Angle; // in radians
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezHeadBoneComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezHeadBoneComponent, ezComponent, ezHeadBoneComponentManager);

public:
  ezHeadBoneComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnSetVerticalRotation(ezMsgSetVerticalHeadBoneRotation& msg);
  void OnChangeVerticalRotation(ezMsgChangeVerticalHeadBoneRotation& msg);

public:
  ezAngle m_MaxVerticalRotation;

private:
  ezAngle m_CurVerticalRotation;
  ezAngle m_NewVerticalRotation;
};


